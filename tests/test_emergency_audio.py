#!/usr/bin/env python3
"""Fault-path tests of production SDK functions and the complete audio adapter.

MMIO/CD are instrumented, never emulated Saturn timing. The WFSX fixture is
synthetic; it proves lifecycle/queue contracts, NOT waveforms or arcade fidelity.
"""
from pathlib import Path
import argparse, hashlib, json, os, re, subprocess

ROOT = Path(__file__).resolve().parents[1]

def function(source, name, fallback=None):
    m = re.search(r'(?:static\s+)?(?:void|bool)\s+' + re.escape(name) + r'\([^;]*?\)\s*\{', source)
    if not m:
        if fallback is not None:
            return fallback
        raise RuntimeError('Missing production function: ' + name)
    end, depth = m.end(), 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[m.start():end]

SDK_PREFIX = r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define JO_COMPILE_WITH_AUDIO_SUPPORT
#define JO_COMPILE_WITH_FS_SUPPORT
#include "jo/audio.h"
#include "jo/sega_saturn.h"
#define JO_ZERO(x) ((x)=0)
#define JO_NULL NULL
#define JO_MIN(a,b) ((a)<(b)?(a):(b))
#define JO_AUDIO_MUSIC_LEFT_SLOT 18u
#define JO_AUDIO_MUSIC_BLOCK_SAMPLES 4096u
#define JO_AUDIO_MUSIC_AHEAD 12288u
#define JO_AUDIO_MUSIC_BUFFER_SIZE 65536u
#define JO_AUDIO_MUSIC_NO_QUOTA (~0u)
#undef JO_SCSP_COMMON
static volatile unsigned short registers[128];
#define JO_SCSP_COMMON(offset) registers[(offset)/2u]
static volatile bool __jo_audio_emergency_muted;
static unsigned __jo_audio_pending_count, __jo_audio_music_sound_ram;
static unsigned __jo_core_vblank_count;
static struct { bool active; } __jo_audio_channels[JO_SOUND_MAX_CHANNEL];
static void (*__jo_core_music_vblank)(void), (*__jo_core_music_update)(void), (*__jo_fs_stream_service)(void);
static unsigned io, decode, keys, frees;
static void __jo_fs_stream_poll(void) { ++io; }
static bool __jo_fs_stream_async_pending(void) { return true; }
static void __jo_fs_stream_stop_async(void) { ++io; }
static void __jo_fs_stream_stop(void) { ++io; }
static void jo_audio_music_fill(bool urgent) { (void)urgent; ++io; }
static void jo_audio_music_decode(unsigned a,unsigned b) { (void)a;(void)b; ++decode; }
static void jo_audio_music_key_on(void) { ++keys; }
static void jo_audio_music_key_off(void) { ++keys; }
static void jo_free(void *p) { (void)p; ++frees; }
static unsigned jo_core_get_refresh_rate(void) { return 60; }
static void jo_memcpy(void *to,const void *from,unsigned n) { memcpy(to,from,n); }
static void jo_audio_music_update(void);
static void jo_audio_music_service(void);
static void jo_audio_music_vblank(void);
'''
SDK_SUFFIX = r'''
static unsigned char buffer[65536];
static void setup(unsigned kind) {
    __jo_audio_emergency_muted=false;
    memset((void*)registers,0xa5,sizeof registers);
    jo_audio_set_volume(127);
    assert(((volatile unsigned char*)&registers[0])[1]==15);
    memset(&__jo_audio_music,0,sizeof __jo_audio_music);
    __jo_audio_music.volume=127;
    __jo_audio_music.buffer=kind==0?NULL:buffer;
    __jo_audio_music_reserved_buffer=buffer;
    __jo_audio_music_sound_ram=0x10000;
    __jo_audio_music_pending=kind==2;
    __jo_audio_music_ready=kind==2;
    __jo_audio_music_autostart=kind==2;
    __jo_audio_music_priming=kind==2;
    __jo_audio_music_async=kind!=0;
    __jo_audio_music.playing=kind==1;
    __jo_audio_music.frame_size=2;
    __jo_audio_music.frame_samples=735;
    __jo_audio_music_start_ticks=123;
    __jo_audio_pending_count=2;
    for(unsigned i=0;i<JO_SOUND_MAX_CHANNEL;++i)__jo_audio_channels[i].active=true;
    __jo_core_music_update=jo_audio_music_update;
    __jo_core_music_vblank=jo_audio_music_vblank;
    __jo_fs_stream_service=jo_audio_music_service;
    memset(buffer,0x5a,sizeof buffer);
    io=decode=keys=frees=0;
}
int main(void) {
    for(unsigned kind=0;kind<3;++kind){
        setup(kind);
        void (*old_update)(void)=__jo_core_music_update;
        void (*old_irq)(void)=__jo_core_music_vblank;
        void (*old_file_service)(void)=__jo_fs_stream_service;
        unsigned char *old_buffer=__jo_audio_music.buffer;
        jo_audio_emergency_mute();
        assert(((volatile unsigned char*)&registers[0])[0]==2);
        assert(((volatile unsigned char*)&registers[0])[1]==0 && "master output still audible");
        assert(__jo_audio_emergency_muted);
        assert(!__jo_core_music_update&&!__jo_core_music_vblank&&!__jo_fs_stream_service);
        assert(!__jo_audio_music_pending&&!__jo_audio_music_ready&&!__jo_audio_music_autostart&&!__jo_audio_music_priming);
        assert(!__jo_audio_music.playing&&!__jo_audio_pending_count);
        assert(__jo_audio_music.buffer==old_buffer && "borrowed stream memory must survive");
        for(unsigned i=0;i<JO_SOUND_MAX_CHANNEL;++i)assert(!__jo_audio_channels[i].active);
        for(unsigned i=0;i<sizeof buffer;++i)assert(buffer[i]==0x5a);
        assert(!io&&!decode&&!keys&&!frees);
        old_update();old_irq();old_file_service();
        jo_audio_stop_music();jo_audio_set_volume(127);
        assert(((volatile unsigned char*)&registers[0])[1]==0 && "ordinary volume setter reopened master output");
        jo_audio_emergency_mute();
        jo_audio_prepared_music p={.fad=50,.data_start=44,.data_end=10000,.rate=44100,
          .channels=1,.format=JoAudioMusicPcm16,.frame_size=2,.block_align=2};
        assert(!jo_audio_stage_music(&p));
        assert(!io&&!decode&&!keys&&!frees && "fault path must never wait on hardware or free a live buffer");
        assert(((volatile unsigned char*)&registers[0])[1]==0);
        assert(!__jo_core_music_update&&!__jo_core_music_vblank&&!__jo_fs_stream_service);
    }
    puts("PASS SDK: cold/playing/primed, all channels, master latch, saved callbacks, repeat, no IO/free, stage refused");
}
'''
ADAPTER = r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define WF_JO_AUDIO_HOST
#include "wf_jo_audio.c"
extern void wf_jo_audio_emergency_stop(void) __attribute__((weak));
static unsigned now, starts, music_calls, writes, prepare_calls, mute_calls, volume;
static bool hw_muted, hw_playing;
static unsigned char sound_ram[524288];
static unsigned top=sizeof sound_ram;
unsigned jo_get_ticks(void){return now;}
void *jo_malloc(unsigned n){return malloc(n);}
void jo_free(void *p){free(p);}
bool jo_audio_reserve_music(void){return true;}
bool jo_audio_reserve_resident(jo_sound *s,unsigned n){assert(n<=top);top-=n; s->resident=top+1;s->data_length=n;return true;}
bool jo_audio_write_resident(const jo_sound *s,unsigned at,const void *p,unsigned n){assert(s->resident+at+n<=sizeof sound_ram+1);memcpy(sound_ram+s->resident-1+at,p,n);++writes;return true;}
bool jo_audio_sync(void){return true;}
void jo_audio_stop_channel(unsigned char c){(void)c;}
bool jo_audio_is_channel_playing(unsigned char c){(void)c;return hw_playing;}
void jo_audio_play_sound_on_channel(jo_sound*s,unsigned char c){(void)s;(void)c;++starts;}
bool jo_audio_prepare_music_from_header(const char*n,const unsigned char*h,unsigned z,unsigned size,jo_audio_prepared_music*p){(void)n;assert(h&&z==44&&size>44);memset(p,0,sizeof *p);p->fad=1;p->rate=44100;++prepare_calls;return true;}
bool jo_audio_cache_music_prefix(const char*n,jo_audio_prepared_music*p,void*b,unsigned z){(void)n;(void)p;(void)b;(void)z;return false;}
bool jo_audio_prime_prepared_music(const jo_audio_prepared_music*p){(void)p;return true;}
bool jo_audio_is_music_ready(void){return true;}
bool jo_audio_play_prepared_music(const jo_audio_prepared_music*p,bool loop){(void)p;(void)loop;++starts;hw_playing=true;return true;}
void jo_audio_service_music(void){++music_calls;}
bool jo_audio_is_music_pending(void){return false;}
bool jo_audio_is_music_playing(void){return hw_playing;}
void jo_audio_stop_music(void){++music_calls;hw_playing=false;}
void jo_audio_set_music_volume(unsigned char v){volume=v;}
unsigned jo_audio_get_music_underruns(void){return 0;}
unsigned jo_audio_get_music_start_ticks(void){return now;}
unsigned jo_audio_get_music_io_errors(void){return 0;}
void jo_audio_emergency_mute(void){++mute_calls;hw_muted=true;volume=0;hw_playing=false;}
static void put16(uint8_t*p,unsigned v){p[0]=(uint8_t)(v>>8);p[1]=(uint8_t)v;}
static void put32(uint8_t*p,uint32_t v){put16(p,v>>16);put16(p+2,v);}
static unsigned char synthetic_pack[1550];
static void fixture(void){
 uint8_t*p=synthetic_pack;memcpy(p,"WFSX",4);put16(p+4,1);put16(p+6,116);put32(p+8,sizeof synthetic_pack);
 put32(p+12,32);put32(p+16,960);put32(p+20,1548);put32(p+24,1548);
 for(unsigned r=1;r<116;++r)if(r!=74){put32(p+32+r*8,1548);put16(p+36+r*8,1);put16(p+38+r*8,1);}
 put32(p+28,crc32(p+32,sizeof synthetic_pack-32));
 assert(wf_jo_audio_pack_valid(p,sizeof synthetic_pack));
}
int main(int argc,char **argv){
 assert(argc==2);unsigned kind=(unsigned)atoi(argv[1]);fixture();
 void (*volatile emergency_entry)(void)=wf_jo_audio_emergency_stop;
 assert(emergency_entry && "missing emergency stop API");
 if(kind){assert(wf_jo_audio_init(synthetic_pack,sizeof synthetic_pack));assert(prepare_calls==18);}
 if(kind==2){
  assert(wf_r18_scsp_audio(NULL,0x3106,NULL)==WF_OK);assert(wf_r18_scsp_service());assert(hw_playing);
  for(unsigned i=0;i<64;++i)assert(wf_r18_scsp_audio(NULL,0x3120,NULL)==WF_OK);
  for(unsigned i=0;i<3;++i){pcm[i].pending=pcm[i].playing=pcm[i].filling=1;sequence[i].active=1;}
  assert(wf_r18_scsp_audio(NULL,0x3121,NULL)==WF_BAD_STATE);
 }
 if(kind==3)wf_jo_audio_diagnostic.fault=5;
 unsigned before_music=music_calls,before_starts=starts,before_writes=writes,before_prepare=prepare_calls;
 uint32_t old_fault=wf_jo_audio_diagnostic.fault;
 wf_jo_audio_emergency_stop();
 assert(hw_muted&&volume==0&&!hw_playing&&mute_calls);
 assert(!initialized&&!wf_jo_audio_diagnostic.initialized&&!wf_jo_audio_diagnostic.queue_depth);
 assert(queue_write==queue_read);
 for(unsigned i=0;i<3;++i)assert(!pcm[i].pending&&!pcm[i].playing&&!pcm[i].filling&&!sequence[i].active);
 assert(wf_jo_audio_diagnostic.fault==old_fault);
 assert(wf_r18_scsp_audio(NULL,0x3106,NULL)==WF_BAD_STATE);
 assert(!wf_r18_scsp_service()&&!wf_jo_audio_service_cooperate());
 assert(!wf_jo_audio_prime_music(6));
 assert(!wf_jo_audio_init(synthetic_pack,sizeof synthetic_pack));
 wf_jo_audio_emergency_stop();
 assert(hw_muted&&volume==0&&music_calls==before_music&&starts==before_starts&&writes==before_writes&&prepare_calls==before_prepare);
 puts("PASS adapter: lifecycle, full queue, 3 pending voices, rejected restart, retained fault, no post-fault work");
}
'''

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--source',type=Path,default=ROOT)
    p.add_argument('--output',type=Path,default=ROOT/'build/emergency-audio')
    args=p.parse_args();root=args.source.resolve();out=args.output.resolve();out.mkdir(parents=True,exist_ok=True)
    s=(root/'sdk/jo_engine/audio.c').read_text()
    a=s.index('typedef enum\n{\n    JoAudioMusicPcm8');b=s.index('\nstatic unsigned int                 __jo_audio_music_sector',a)
    body=SDK_PREFIX+s[a:b]
    for name in ('jo_audio_set_memory_and_volume','jo_audio_set_volume','jo_audio_music_vblank','jo_audio_music_update','jo_audio_music_service','jo_audio_stage_music','jo_audio_stop_music'):
        body+='\n'+function(s,name)+'\n'
    body+='\n'+function(s,'jo_audio_emergency_mute','void jo_audio_emergency_mute(void){}')+'\n'+SDK_SUFFIX
    report={'scope':'Host contracts; fake MMIO/CD and synthetic WFSX. No target timing or acoustic verification.', 'runs':[], 'source_hashes':{f:hashlib.sha256((root/f).read_bytes()).hexdigest() for f in ('sdk/jo_engine/audio.c','port/wf_jo_audio.c')}}
    for label,code,cases in [('sdk',body,[[]]),('adapter',ADAPTER,[[str(i)] for i in range(4)])]:
        path=out/(label+'.c');path.write_text(code)
        cmd=[os.environ.get('CC','cc'),'-std=c11','-O1','-g','-Wall','-Wextra','-Werror','-Wno-unused-variable',
             '-fsanitize=address,undefined','-fno-omit-frame-pointer','-fno-pie','-no-pie',
             '-I'+str(root/'port'),'-I'+str(root/'core/include'),'-I'+str(root/'sdk/jo_engine'),str(path),'-o',str(out/label)]
        c=subprocess.run(cmd,capture_output=True,text=True);(out/(label+'-compile.log')).write_text(c.stdout+c.stderr)
        if c.returncode:
            print(c.stderr);report['runs'].append({'name':label,'compile_returncode':c.returncode});continue
        for i,arguments in enumerate(cases):
            r=subprocess.run([str(out/label),*arguments],capture_output=True,text=True,timeout=10,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0',UBSAN_OPTIONS='halt_on_error=1'))
            log=f'{label}-{i}.log';(out/log).write_text(r.stdout+r.stderr)
            report['runs'].append({'name':label,'case':i,'returncode':r.returncode,'log':log,'command':cmd})
            print(label,i,'PASS' if r.returncode==0 else 'FAIL',r.stdout+r.stderr)
    report['status']='PASS' if len(report['runs'])==5 and all(r.get('returncode')==0 for r in report['runs']) else 'FAIL'
    (out/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    return report['status']!='PASS'
if __name__=='__main__':raise SystemExit(main())
