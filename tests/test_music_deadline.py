#!/usr/bin/env python3
"""Exercise the production music starvation guard with delayed input.

Only PCM copying and sound-RAM writes are substituted. This establishes the
buffer-response deadline, not the latency of a physical Saturn CD drive.
"""
from pathlib import Path
import subprocess
import tempfile
from test_music_stream import extract_function

ROOT=Path(__file__).resolve().parents[1]
PREFIX=r'''
#include <assert.h>
#include <stdio.h>
#include <string.h>
#define JO_MIN(a,b) ((a)<(b)?(a):(b))
#define JO_AUDIO_MUSIC_BLOCK_SAMPLES 4096u
#define JoAudioMusicImaAdpcm 2u
static struct {
 unsigned written,staged,format,in,out,frame_size,played_blocks;
 unsigned end_of_data,ima_left,end_written,underruns;
} __jo_audio_music;
static unsigned silence_frames;
static unsigned jo_audio_music_decode_ima(unsigned n){(void)n;assert(0);return 0;}
static void jo_audio_music_decode_pcm(unsigned n){
 __jo_audio_music.out+=n*2u;__jo_audio_music.written+=n;
}
static void jo_audio_music_flush(void){}
static void jo_audio_music_silence(unsigned limit){
 assert(limit>=__jo_audio_music.written);
 silence_frames+=limit-__jo_audio_music.written;__jo_audio_music.written=limit;
}
'''
SUFFIX=r'''
static unsigned run(unsigned prefix,unsigned first_data_ms,unsigned *first_under){
 memset(&__jo_audio_music,0,sizeof __jo_audio_music);silence_frames=0;
 __jo_audio_music.frame_size=2;__jo_audio_music.in=prefix-44u;
 jo_audio_music_decode(12288u,~0u);assert(!__jo_audio_music.underruns);
 *first_under=0;
 for(unsigned frame=1;frame<=120;++frame){
  unsigned now=frame*1000u/60u;
  if(now>=first_data_ms)__jo_audio_music.in=1000000u;
  unsigned played=frame*735u;
  __jo_audio_music.played_blocks=played/4096u;
  unsigned old=__jo_audio_music.underruns;
  jo_audio_music_decode(played+12288u,1472u);
  if(!old&&__jo_audio_music.underruns)*first_under=now;
 }
 return __jo_audio_music.underruns;
}
int main(void){
 unsigned first,small,large;
 small=run(32768u,300u,&first);
 assert(small>0&&first==200u);
 printf("PASS:32KiB prefix, first new data300ms -> %u underruns, first at%ums\n",small,first);
 large=run(65536u,550u,&first);
 assert(!large&&!silence_frames);
 puts("PASS:64KiB prefix, first new data550ms -> zero underruns or inserted silence");
 assert(run(65536u,650u,&first)>0&&first==566u);
 printf("PASS:64KiB remains finite: first new data650ms -> starvation at%ums\n",first);
}
'''

def main():
    source=(ROOT/'sdk/jo_engine/audio.c').read_text()
    with tempfile.TemporaryDirectory(prefix='wf-music-deadline-') as folder:
        work=Path(folder);c=work/'test.c';binary=work/'test'
        c.write_text(PREFIX+extract_function(source,'jo_audio_music_decode')+SUFFIX)
        subprocess.run(['cc','-std=c11','-O2','-Wall','-Wextra','-Werror',str(c),'-o',str(binary)],check=True)
        subprocess.run([str(binary)],check=True)

if __name__=='__main__':main()
