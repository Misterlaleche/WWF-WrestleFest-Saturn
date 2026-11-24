#!/usr/bin/env python3
"""Compile the actual Jo refill routine against a deterministic CD boundary.

The extracted function is production code, not a second implementation. The
fake drive has a 200-sector buffer and honors the requested physical read span.
This catches repeated seeks caused by confusing the RAM buffer with the drive
read extent, and verifies every byte through RAM-buffer wraps and final EOF.
"""
from pathlib import Path
import re
import subprocess
import tempfile

ROOT=Path(__file__).resolve().parents[1]

def extract_function(source,name):
    match=re.search(r'(?:static )?void\s+'+re.escape(name)+r'\([^;]+?\)\s*\{',source)
    if not match:raise RuntimeError('production function not found: '+name)
    depth=1;end=match.end()
    while depth:
        depth+=(source[end]=='{')-(source[end]=='}');end+=1
    return source[match.start():end]

PREFIX=r'''
#include <assert.h>
#include <stdbool.h>
static volatile bool __jo_audio_emergency_muted;
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#define JO_ZERO(x) ((x)=0)
#define JO_NULL ((void*)0)
#define JO_MIN(a,b) ((a)<(b)?(a):(b))
#define JO_AUDIO_MUSIC_BUFFER_SIZE 65536u
#define JO_AUDIO_MUSIC_BUFFER_MASK 65535u
#define JO_AUDIO_MUSIC_SECTOR_SIZE 2048u
#define JO_AUDIO_MUSIC_URGENT_TIMEOUT 100u
#define JO_AUDIO_MUSIC_AHEAD 12288u
#define JO_AUDIO_MUSIC_NO_QUOTA (~0u)
#define BASE_FAD 1000u
#define FILE_BYTES (44100u*2u*20u+44u)
static unsigned char storage[65536];
static unsigned int __jo_audio_music_sector[512];
static struct {
 unsigned cursor,data_end,data_start,in,out,reading_sector,fad,frame_size,written;
 unsigned char *buffer;
 bool loop,end_of_data,ended,playing;
} __jo_audio_music;
static bool __jo_audio_music_pending,__jo_audio_music_ready,__jo_audio_music_autostart,__jo_audio_music_priming;
static bool __jo_audio_music_async;
static unsigned char *__jo_audio_music_reserved_buffer=storage;
static void (*__jo_core_music_vblank)(void);
static void (*__jo_core_music_update)(void),(*__jo_fs_stream_service)(void);
static unsigned keyons,keyoffs;
static void jo_audio_music_vblank(void){}
static void jo_audio_music_key_on(void){++keyons;}
static void jo_audio_music_key_off(void){++keyoffs;}
static void jo_free(void *p){(void)p;assert(0);}
void jo_audio_stop_music(void);
static void jo_audio_music_decode(unsigned frames,unsigned quota){
 (void)quota;assert(__jo_audio_music.in-__jo_audio_music.out>=frames*2u);
 __jo_audio_music.out+=frames*2u;__jo_audio_music.written+=frames;
}
static unsigned requests,requested_count,stream_sector,remaining;
static bool reading;
static unsigned async_requests,async_stops,sync_stops;
static bool stop_pending;
static unsigned char byte_at(unsigned at){return (unsigned char)(at*17u+3u);}
static bool __jo_fs_stream_is_reading(void){return reading;}
static int __jo_fs_stream_read(unsigned fad,unsigned count){
 ++requests;requested_count=count;stream_sector=fad-BASE_FAD;remaining=count;reading=true;return 0;
}
static int __attribute__((unused)) __jo_fs_stream_read_async(unsigned fad,unsigned count){
 ++async_requests;return __jo_fs_stream_read(fad,count);
}
static int __attribute__((unused)) __jo_fs_stream_poll(void){
 if(stop_pending){stop_pending=false;reading=false;}return 1;
}
static bool __attribute__((unused)) __jo_fs_stream_async_pending(void){return stop_pending;}
static void __attribute__((unused)) __jo_fs_stream_stop_async(void){++async_stops;stop_pending=true;}
static void __attribute__((unused)) __jo_fs_stream_stop(void){++sync_stops;reading=false;}
static int __jo_fs_stream_take(void *target,int maximum){
 if(!reading)return -1;
 unsigned n=JO_MIN(200u,remaining);n=JO_MIN(n,(unsigned)maximum);
 unsigned char *out=target;
 if(out>=storage&&out<storage+sizeof storage)assert(n*2048u<=(unsigned)(storage+sizeof storage-out));
 else assert(target==__jo_audio_music_sector&&n==1);
 for(unsigned i=0;i<n*2048u;++i)out[i]=byte_at(stream_sector*2048u+i);
 stream_sector+=n;remaining-=n;if(!remaining)reading=false;
 return (int)n;
}
static void jo_memcpy(void *to,const void *from,unsigned bytes){
 unsigned char*p=to;assert(p>=storage&&p<=storage+sizeof storage&&bytes<=(unsigned)(storage+sizeof storage-p));
 memcpy(to,from,bytes);
}
'''
SUFFIX=r'''
int main(void){
 __jo_audio_music.cursor=__jo_audio_music.data_start=44;
 __jo_audio_music.data_end=FILE_BYTES;__jo_audio_music.fad=BASE_FAD;
 __jo_audio_music.reading_sector=~0u;__jo_audio_music.buffer=storage;
 jo_audio_music_fill(false);
 assert(requests==1);
 assert(requested_count==(FILE_BYTES+2047u)/2048u); /* Physical stream, not one RAM-buffer-sized seek. */
 unsigned consumed=0;
 while(!__jo_audio_music.end_of_data||__jo_audio_music.in!=__jo_audio_music.out){
  unsigned n=JO_MIN(1024u,__jo_audio_music.in-__jo_audio_music.out);
  for(unsigned i=0;i<n;++i)assert(storage[(__jo_audio_music.out+i)&65535u]==byte_at(44u+consumed+i));
  consumed+=n;__jo_audio_music.out+=n;jo_audio_music_fill(false);
  assert(__jo_audio_music.in-__jo_audio_music.out<=65536u);
 }
 assert(consumed==FILE_BYTES-44u&&requests==1);
 printf("PASS: one continuous CD read, %u exact PCM bytes, RAM wraps and EOF\n",consumed);
 memset(&__jo_audio_music,0,sizeof __jo_audio_music);requests=0;keyons=0;
 __jo_audio_music.cursor=__jo_audio_music.data_start=44;__jo_audio_music.data_end=FILE_BYTES;
 __jo_audio_music.frame_size=2;__jo_audio_music.fad=BASE_FAD;__jo_audio_music.reading_sector=~0u;
 __jo_audio_music.buffer=storage;__jo_audio_music_pending=true;__jo_audio_music_priming=true;
 __jo_audio_music_async=true;
 jo_audio_music_update();
 assert(keyons==0 && reading);
 assert(__jo_audio_music.in-__jo_audio_music.out>=65536u-2048u);
 __jo_audio_music_autostart=true;jo_audio_music_update();
 assert(keyons==1 && !__jo_audio_music_pending && requests==1 && reading);
 assert(async_requests==1);
 puts("PASS: prime retains the reader, fills ring plus main buffer, and key-on causes no seek");
 jo_audio_stop_music();
 assert(async_stops==1 && sync_stops==0 && stop_pending && __jo_core_music_update);
 jo_audio_music_update();
 assert(!stop_pending && !__jo_core_music_update && keyoffs==1);
 puts("PASS: prepared stop returns before CD acknowledgement; normal updates retire the request");
 for(unsigned prefix=32768u;prefix<=65536u;prefix*=2u){
  memset(&__jo_audio_music,0,sizeof __jo_audio_music);requests=0;keyons=0;reading=false;
  __jo_audio_music.cursor=prefix;__jo_audio_music.data_start=44;__jo_audio_music.data_end=FILE_BYTES;
  __jo_audio_music.frame_size=2;__jo_audio_music.fad=BASE_FAD;__jo_audio_music.reading_sector=~0u;
  __jo_audio_music.buffer=storage;__jo_audio_music.in=prefix-44;
  for(unsigned i=0;i<prefix-44u;++i)storage[i]=byte_at(44u+i);
  __jo_audio_music_pending=true;__jo_audio_music_ready=false;__jo_audio_music_autostart=true;
  __jo_audio_music_priming=false;
  jo_audio_music_update();assert(keyons==1 && requests==0);
  consumed=__jo_audio_music.out;
  while(!__jo_audio_music.end_of_data||__jo_audio_music.in!=__jo_audio_music.out){
   unsigned n=JO_MIN(1024u,__jo_audio_music.in-__jo_audio_music.out);
   for(unsigned i=0;i<n;++i)assert(storage[(__jo_audio_music.out+i)&65535u]==byte_at(44u+consumed+i));
   consumed+=n;__jo_audio_music.out+=n;jo_audio_music_fill(false);
   assert(__jo_audio_music.in-__jo_audio_music.out<=65536u);
  }
  assert(consumed==FILE_BYTES-44u && requests==1);
  assert(requested_count==(FILE_BYTES+2047u)/2048u-prefix/2048u);
  printf("PASS: %u-byte prefix, seek-free key-on, exact CD handoff and one continuous read\n",prefix);
 }
}
'''
def main():
    source=(ROOT/'sdk/jo_engine/audio.c').read_text()
    with tempfile.TemporaryDirectory(prefix='wf-music-stream-') as folder:
        work=Path(folder);c=work/'test.c';binary=work/'test'
        c.write_text(PREFIX+extract_function(source,'jo_audio_music_fill')+
                     extract_function(source,'jo_audio_music_update')+
                     extract_function(source,'jo_audio_stop_music')+SUFFIX)
        subprocess.run(['cc','-std=c11','-O2','-Wall','-Wextra','-Werror',str(c),'-o',str(binary)],check=True)
        subprocess.run([str(binary)],check=True)

if __name__=='__main__':main()
