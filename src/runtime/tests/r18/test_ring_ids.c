#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "wf_r18_ring.h"
static uint8_t blob[114918],bad[114918];
static unsigned u16(const uint8_t*p){return (p[0]<<8)|p[1];}
static unsigned u32(const uint8_t*p){return (u16(p)<<16)|u16(p+2);}
int main(int argc,char**argv){
 assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f&&fread(blob,1,sizeof blob,f)==sizeof blob);fclose(f);
 unsigned n=u16(blob+10),table=u32(blob+16)+n*128;assert(n>2);
 assert(!wf_r18_ring_bind(blob,sizeof blob));
 /* Binary tile lookup requires strict source-ID ordering, validated at bind.
  * Corruption must fail explicitly rather than selecting unrelated pixels. */
 memcpy(bad,blob,sizeof bad);memcpy(bad+table+2,bad+table,2);assert(wf_r18_ring_bind(bad,sizeof bad)==-1);
 memcpy(bad,blob,sizeof bad);uint8_t tmp[2];memcpy(tmp,bad+table,2);memcpy(bad+table,bad+table+2,2);memcpy(bad+table+2,tmp,2);assert(wf_r18_ring_bind(bad,sizeof bad)==-1);
 assert(!wf_r18_ring_bind(blob,sizeof blob));
 for(unsigned i=0;i<n;i++)assert(!wf_r18_ring_tile_write(0,0,0,(uint16_t)u16(blob+table+i*2)));
 puts("PASS ring source-ID index: rejects duplicates/unsorted IDs; every canonical source tile resolves");return 0;
}
