/* Immutable ROM words must not be copied repeatedly out of slow LWRAM.
 * Instrument the real reader; file I/O remains a byte-exact ROM fixture. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "wf_r18_present.h"
static uint8_t source[524288], cache[524288], tiles[128];
static size_t slow_bytes;
static unsigned reads, fail_read;
static void *counted_copy(void *,const void *,size_t);
#undef memcpy
#define memcpy counted_copy
#include "../../src/wf_r18_present.c"
#undef memcpy
static void *counted_copy(void *d,const void *s,size_t n){
 uintptr_t a=(uintptr_t)s,b=(uintptr_t)cache;
 if(a>=b&&a+n<=b+sizeof cache)slow_bytes+=n;
 return memcpy(d,s,n);
}
static int file_read(const char *name,uint32_t off,void *d,size_t n){
 assert(!strcmp(name,"ROM68K.BIN")&&off+n<=sizeof source);
 ++reads;if(fail_read)return -1;memcpy(d,source+off,n);return 0;
}
static void check(uint32_t at,size_t n){
 uint8_t got[1050];assert(n<=sizeof got);
 assert(wf_r18_rom_read(NULL,at,got,n)==WF_OK);
 assert(!memcmp(got,source+at,n));
}
int main(void){
 for(unsigned i=0;i<sizeof source;i++)source[i]=(uint8_t)(i*37u+(i>>8));
 wf_r18_present_init(file_read,cache,262144u,tiles,sizeof tiles);
 const uint16_t pages[]={0,1,2,511};
 assert(wf_r18_rom_pages_preload(pages,4)==WF_OK);slow_bytes=0;
 unsigned initial=reads;
 for(unsigned i=0;i<1000;i++)check(34u,2u);
 assert(reads==initial);
 if(slow_bytes>32u){fprintf(stderr,"Repeated immutable word copied %zu slow-RAM bytes (budget32)\n",slow_bytes);return 1;}
 /* Word/long ends, unaligned accesses, colliding lines, page crossings and
  * large reads must retain every ROM byte, including the final word. */
 for(unsigned round=0;round<3;round++)for(unsigned at=0;at<2048;at++){
  check(at,1);check(at,2);check(at,4);
 }
 check(1000,1050);check(524286,2);check(524288,0);
 uint8_t got[4]={1,2,3,4};
 assert(wf_r18_rom_read(NULL,524287,got,2)==WF_BLOCKED);
 assert(wf_r18_rom_read(NULL,524289,got,0)==WF_BLOCKED);
 assert(got[0]==1&&got[1]==2);
 /* Cache identity is the full ROM offset, never the low slot bits. */
 check(32,2);check(1056,2);check(524256,2);check(32,2);
 fail_read=1;
 assert(wf_r18_rom_read(NULL,3500,got,2)==WF_BLOCKED);
 fail_read=0;check(3500,2);
 /* Init and sparse reload invalidate words from the preceding image. */
 source[34]^=0xffu;
 assert(wf_r18_rom_pages_preload(pages,4)==WF_OK);check(34,2);
 source[34]^=0x55u;
 wf_r18_present_init(file_read,cache,262144u,tiles,sizeof tiles);check(34,2);
 /* Full preload has the same immutable-word contract as sparse pages. */
 wf_r18_present_init(file_read,cache,sizeof cache,tiles,sizeof tiles);
 assert(wf_r18_rom_preload()==WF_OK);initial=reads;
 for(unsigned page=0;page<512;page++)for(unsigned n=1;n<=4;n++){
  check(page*1024u+31u,n);check(page*1024u+995u,n);
 }
 assert(reads==initial);
 source[34]^=0x23u;
 assert(wf_r18_rom_preload()==WF_OK);check(34,2);
 printf("PASS exact ROM words, bounded slow-RAM copies, cross-lines/pages, invalid ranges, I/O failure and reload invalidation\n");
 return 0;
}
