#!/usr/bin/env python3
"""Couple the production music startup/refill/PCM/guard code to a delayed CD.

Only hardware I/O and sound-RAM writes are substituted. The WAV is consumed
through its exact final partial sector; every PCM sample and inserted zero is
checked. The 300 ms startup delay is the existing test_music_deadline contract,
not an emulator field selected to make this candidate pass.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import wave

ROOT = Path(__file__).resolve().parents[1]


def extract_function(source, name):
    match = re.search(r'(?:static )?(?:void|bool)\s+' + re.escape(name) + r'\([^;]+?\)\s*\{', source)
    if not match:
        raise RuntimeError('production function not found: ' + name)
    depth, end = 1, match.end()
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[match.start():end]

PREFIX = r'''
#include <assert.h>
#include <stdbool.h>
static volatile bool __jo_audio_emergency_muted;
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wf_jo_audio.h"
#define JO_MIN(a,b) ((a)<(b)?(a):(b))
#define JO_ZERO(a) ((a)=0)
#define JO_NULL NULL
#define JO_DEFAULT_AUDIO_VOLUME 127
'''

HARDWARE = r'''
static unsigned char file_bytes[800000], storage[65536];
static unsigned file_size, now_ms, first_data_ms, first_take_ms;
static unsigned requests, requested_count, stream_sector, remaining;
static unsigned keyons, keyoffs, pcm_samples, padded_samples, startup_zeros;
static unsigned pcm_mode, __jo_core_vblank_count, monitor, scsp_samples;
static unsigned first_under_frame, first_under_written;
static bool reading, stop_pending;
static unsigned __jo_audio_music_sound_ram=1;
static void (*__jo_core_music_vblank)(void),(*__jo_core_music_update)(void);
static void (*__jo_fs_stream_service)(void);

#define JO_SCSP_COMMON_MONITOR 0
#define JO_SCSP_MONITOR_SLOT(slot) 0
#define JO_SCSP_COMMON(reg) monitor
#define JO_SCSP_MONITOR_BLOCK(value) (scsp_samples/4096u)
#define JO_AUDIO_MUSIC_LEFT_SLOT 30

static unsigned jo_get_ticks(void){return now_ms;}
static unsigned jo_core_get_refresh_rate(void){return 60;}
static void jo_free(void *p){(void)p;assert(0);}
static void jo_memcpy(void *to,const void *from,unsigned n){memcpy(to,from,n);}
static void jo_audio_music_key_off(void){++keyoffs;}
static void jo_audio_music_key_on(void){
 ++keyons;
 __jo_audio_music.played_blocks=__jo_audio_music.block=0;
 __jo_audio_music.block_vblank=__jo_core_vblank_count;
 __jo_audio_music_start_ticks=jo_get_ticks();
}
static void jo_audio_music_flush(void){
 for(unsigned i=0;i<__jo_audio_music.staged;++i){
  short sample=__jo_audio_music_stage[0][i];
  if(pcm_mode){
   unsigned at=44u+pcm_samples*2u;
   assert(at+1u<file_size);
   short expected=(short)(file_bytes[at]|(file_bytes[at+1u]<<8));
   assert(sample==expected);++pcm_samples;
  }else{
   assert(sample==0);++padded_samples;
   if(!__jo_audio_music.end_of_data)++startup_zeros;
  }
 }
 __jo_audio_music.written+=__jo_audio_music.staged;
 __jo_audio_music.staged=0;
}
static unsigned jo_audio_music_decode_ima(unsigned n){(void)n;assert(0);return 0;}
static bool __jo_fs_stream_is_reading(void){return reading;}
static int __jo_fs_stream_read(unsigned fad,unsigned count){
 ++requests;requested_count=count;stream_sector=fad-1000u;
 remaining=count;reading=true;return 0;
}
static int __jo_fs_stream_read_async(unsigned fad,unsigned count){
 return __jo_fs_stream_read(fad,count);
}
static int __jo_fs_stream_poll(void){
 if(stop_pending){stop_pending=false;reading=false;}return 1;
}
static bool __jo_fs_stream_async_pending(void){return stop_pending;}
static void __jo_fs_stream_stop_async(void){stop_pending=true;}
static void __jo_fs_stream_stop(void){reading=false;}
static int __jo_fs_stream_take(void *target,int maximum){
 if(!reading)return -1;
 if(now_ms<first_data_ms)return 0;
 if(!first_take_ms)first_take_ms=now_ms;
 unsigned n=JO_MIN(32u,remaining);n=JO_MIN(n,(unsigned)maximum);
 unsigned char *out=target;
 for(unsigned i=0;i<n*2048u;++i){
  unsigned at=stream_sector*2048u+i;
  out[i]=at<file_size?file_bytes[at]:0xa5;
 }
 stream_sector+=n;remaining-=n;if(!remaining)reading=false;
 return (int)n;
}
static void jo_audio_music_decode_pcm(unsigned);
static void jo_audio_music_update(void);
static void jo_audio_music_service(void);
void jo_audio_stop_music(void);
'''

PCM_WRAPPER = r'''
static void jo_audio_music_decode_pcm(unsigned n){
 pcm_mode=1;jo_audio_music_decode_pcm_body(n);
}
'''

SILENCE_WRAPPER = r'''
static void jo_audio_music_silence(unsigned end){
 pcm_mode=0;jo_audio_music_silence_body(end);
}
'''

SUFFIX = r'''
typedef struct {unsigned underruns,zeros,first_frame,first_written,first_take;} result_t;
static result_t run(unsigned prefix,unsigned delay){
 memset(&__jo_audio_music,0,sizeof __jo_audio_music);
 memset(storage,0xcc,sizeof storage);
 __jo_audio_music_reserved_buffer=storage;
 __jo_audio_music_pending=__jo_audio_music_ready=false;
 __jo_audio_music_autostart=__jo_audio_music_priming=false;
 __jo_audio_music_async=false;
 reading=stop_pending=false;
 now_ms=__jo_core_vblank_count=scsp_samples=0;
 requests=requested_count=keyons=keyoffs=pcm_samples=padded_samples=startup_zeros=0;
 first_under_frame=first_under_written=first_take_ms=0;
 first_data_ms=delay;
 jo_audio_prepared_music prepared={1000,44,file_size,44100,1,
  JoAudioMusicPcm16,2,0,2,file_bytes,prefix};
 assert(jo_audio_play_prepared_music(&prepared,false));
 assert(keyons==1&&requests==0&&__jo_audio_music_start_ticks==0);
 assert(!__jo_audio_music_pending&&__jo_audio_music.playing);
 for(unsigned frame=1;frame<600u;++frame){
  __jo_core_vblank_count=frame;now_ms=frame*1000u/60u;
  scsp_samples=frame*735u;
  jo_audio_music_update();
  unsigned before=__jo_audio_music.underruns;
  jo_audio_music_vblank();
  if(__jo_audio_music.underruns!=before&&!first_under_frame){
   first_under_frame=frame;first_under_written=__jo_audio_music.written;
  }
  jo_audio_music_update();
  assert(__jo_audio_music.in-__jo_audio_music.out<=65536u);
  if(__jo_audio_music.ended)break;
 }
 assert(__jo_audio_music.ended&&!__jo_audio_music.playing);
 assert(__jo_audio_music.cursor==file_size);
 assert(pcm_samples==(file_size-44u)/2u);
 assert(__jo_audio_music.end_written==pcm_samples+startup_zeros);
 assert(requests==1&&requested_count==(file_size+2047u)/2048u-prefix/2048u);
 result_t r={__jo_audio_music.underruns,startup_zeros,first_under_frame,
  first_under_written,first_take_ms};
 printf("{\"prefix_bytes\":%u,\"first_data_ms\":%u,\"first_take_ms\":%u,"
  "\"underruns\":%u,\"startup_zeros\":%u,\"first_underrun_frame\":%u,"
  "\"first_underrun_written\":%u,\"exact_pcm_samples\":%u,"
  "\"eof_cursor\":%u,\"physical_reads\":%u,\"key_on_ms\":0}\n",
  prefix,delay,r.first_take,r.underruns,r.zeros,r.first_frame,r.first_written,
  pcm_samples,__jo_audio_music.cursor,requests);
 return r;
}
int main(int argc,char **argv){
 (void)__jo_audio_music_block;
 assert(argc==2);FILE *f=fopen(argv[1],"rb");assert(f);
 file_size=(unsigned)fread(file_bytes,1,sizeof file_bytes,f);assert(!ferror(f));
 assert(feof(f));fclose(f);assert(file_size==705644u);
 result_t r=run(32768u,0u);assert(!r.underruns&&!r.zeros);
 r=run(32768u,283u);assert(r.underruns==2u&&r.zeros==2646u&&r.first_frame==12u);
 r=run(32768u,300u);assert(r.underruns==2u&&r.zeros==4118u&&r.first_frame==12u);
 r=run(49152u,300u);assert(!r.underruns&&!r.zeros);
 r=run(49152u,350u);assert(!r.underruns&&!r.zeros);
 r=run(49152u,400u);assert(r.underruns==1u&&r.zeros==22u&&r.first_frame==23u);
 r=run(65536u,550u);assert(!r.underruns&&!r.zeros);
 r=run(WF_JO_AUDIO_CART_PREFIX_BYTES-65536u,300u);
 if(r.underruns||r.zeros){fputs("Configured cue 08 prefix fails the existing 300 ms deadline\n",stderr);return 1;}
 puts("PASS: configured prefix, immediate key-on, exact full WAV, final partial sector and finite startup deadline");
 return 0;
}
'''


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--wav', type=Path, required=True)
    parser.add_argument('--build-dir', type=Path, default=Path('build/music-startup'))
    parser.add_argument('--sanitize', action='store_true')
    args = parser.parse_args()
    args.wav = args.wav.resolve()
    work = args.build_dir.resolve()
    work.mkdir(parents=True, exist_ok=True)
    with wave.open(str(args.wav), 'rb') as wav:
        assert (wav.getnchannels(), wav.getsampwidth(), wav.getframerate(),
                wav.getnframes()) == (1, 2, 44100, 352800)
    assert args.wav.read_bytes()[36:40] == b'data'
    source = (ROOT/'sdk/jo_engine/audio.c').read_text()
    header = (ROOT/'sdk/jo_engine/jo/audio.h').read_text()
    prepared = re.search(r'typedef struct\s*\{[^{}]*\}\s*jo_audio_prepared_music;', header).group()
    declarations = source[source.index('# define JO_AUDIO_MUSIC_BLOCK_SAMPLES'):
                          source.index('extern void', source.index('# define JO_AUDIO_MUSIC_BLOCK_SAMPLES'))]
    pcm = extract_function(source, 'jo_audio_music_decode_pcm').replace(
        'jo_audio_music_decode_pcm(', 'jo_audio_music_decode_pcm_body(', 1)
    silence = extract_function(source, 'jo_audio_music_silence').replace(
        'jo_audio_music_silence(', 'jo_audio_music_silence_body(', 1)
    program = (PREFIX + prepared + '\n' + declarations + HARDWARE + pcm + PCM_WRAPPER
               + silence + SILENCE_WRAPPER)
    for name in ('jo_audio_music_decode',
                 'jo_audio_music_vblank', 'jo_audio_music_fill',
                 'jo_audio_music_update', 'jo_audio_music_service',
                 'jo_audio_stop_music', 'jo_audio_stage_music',
                 'jo_audio_play_prepared_music'):
        program += extract_function(source, name)
    program += SUFFIX
    cfile, binary = work/'music_startup.c', work/'music_startup'
    cfile.write_text(program)
    command = ['cc', '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror',
               '-I'+str(ROOT/'port'), '-I'+str(ROOT/'core/include')]
    if args.sanitize:
        command += ['-O1', '-g', '-fsanitize=address,undefined', '-fno-omit-frame-pointer', '-no-pie']
    command += [str(cfile), '-o', str(binary)]
    subprocess.run(command, check=True)
    env = os.environ.copy()
    env.setdefault('ASAN_OPTIONS', 'detect_leaks=0')
    result = subprocess.run([str(binary), str(args.wav)], capture_output=True, text=True, env=env)
    (work/'run.log').write_text(result.stdout + result.stderr)
    files = [ROOT/'sdk/jo_engine/audio.c', ROOT/'sdk/jo_engine/jo/audio.h',
             ROOT/'port/wf_jo_audio.h', Path(__file__),
             args.wav, cfile, binary]
    report = {'command': command, 'run_command': [str(binary), str(args.wav)],
              'returncode': result.returncode, 'cases': [json.loads(line) for line in result.stdout.splitlines()
                                                       if line.startswith('{')],
              'sha256': {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in files}}
    (work/'report.json').write_text(json.dumps(report, indent=2)+'\n')
    print(result.stdout, end='')
    print(result.stderr, end='')
    raise SystemExit(result.returncode)


if __name__ == '__main__':
    main()
