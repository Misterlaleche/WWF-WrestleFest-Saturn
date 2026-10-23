#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "wf_r18_ring.h"
/* New transport contract: VDP2 16x16 characters and one-word pattern names. */
int wf_r18_ring_characters(volatile uint8_t *,size_t);
int wf_r18_ring_cells_update(unsigned,int,int,volatile uint16_t *,size_t *);
static uint8_t blob[114918],chars[131072];static uint16_t map[2][1024];
static unsigned u16(const uint8_t*p){return (p[0]<<8)|p[1];}
static unsigned u32(const uint8_t*p){return (u16(p)<<16)|u16(p+2);}
static unsigned pixel(unsigned layer,unsigned x,unsigned y){
 uint16_t pn=map[layer][((y>>4)&31u)*32u+((x>>4)&31u)];
 unsigned px=x&15u,py=y&15u;if(pn&0x400u)px=15u-px;if(pn&0x800u)py=15u-py;
 unsigned q=(py/8u)*2u+px/8u;uint8_t b=chars[(pn&1023u)*128u+q*32u+(py&7u)*4u+(px&7u)/2u];
 return ((pn>>12)<<4)|((px&1u)?b&15u:b>>4);
}
static void check(unsigned l,int x,int y){
 size_t writes;assert(!wf_r18_ring_cells_update(l,x,y,map[l],&writes));
 int bx=x%2560,by=y%2048;if(bx<0)bx+=2560;if(by<0)by+=2048;
 for(unsigned py=0;py<240;py++)for(unsigned px=0;px<320;px++){
  uint8_t want;assert(!wf_r18_ring_pixel(l,x+(int)px,y+(int)py,&want));
  assert(pixel(l,(unsigned)(bx+(int)px)%2560u,(unsigned)(by+(int)py)%2048u)==want);
 }
}
int main(int argc,char**argv){
 assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f&&fread(blob,1,sizeof blob,f)==sizeof blob);fclose(f);assert(!wf_r18_ring_bind(blob,sizeof blob));
 unsigned n=u16(blob+10),base=u32(blob+16),ids=base+n*128u;assert(n<=1024);
 assert(wf_r18_ring_characters(chars,n*128u-1u)==-1);assert(!wf_r18_ring_characters(chars,sizeof chars));
 /* Every original packed nibble survives its VDP2 quadrant rearrangement. */
 for(unsigned t=0;t<n;t++)for(unsigned y=0;y<16;y++)for(unsigned x=0;x<16;x++){
  unsigned q=(y/8)*2+x/8;uint8_t b=chars[t*128+q*32+(y&7)*4+(x&7)/2];unsigned got=(x&1)?b&15:b>>4;
  uint8_t a=blob[base+t*128+y*8+x/2];assert(got==((x&1)?a&15:a>>4));
 }
 uint32_t seed=0x26e66;
 for(unsigned l=0;l<2;l++){
  check(l,480,1488);size_t writes;assert(!wf_r18_ring_cells_update(l,480,1488,map[l],&writes)&&writes==0);
  assert(!wf_r18_ring_cells_update(l,481,1488,map[l],&writes)&&writes<=32);
  assert(!wf_r18_ring_cells_update(l,482,1489,map[l],&writes)&&writes<=32);
  for(unsigned frame=0;frame<96;frame++){
   seed=seed*1664525u+1013904223u;int x=(int)(seed%3400u)-400;
   seed=seed*1664525u+1013904223u;int y=(int)(seed%2800u)-400;
   unsigned id=(frame*37)%n,source=u16(blob+ids+id*2u),cell=frame*67u%1024u;
   assert(!wf_r18_ring_tile_write(l,cell,(uint16_t)((frame&15u)|((frame&3u)<<6)),(uint16_t)source));
   check(l,x,y);wf_r18_ring_tile_restore(l,cell);check(l,x,y);
  }
 }
 puts("PASS native VDP2 cells: all803characters byte-exact, visible pixels, flips/palette/pen0, wrap, scroll cache and overlay restore");return 0;
}
