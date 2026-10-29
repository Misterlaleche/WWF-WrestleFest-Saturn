#include "wf_r18_ring.h"
#include "wf_saturn_memory.h"
#include "wf_r18_video.h"
#include <string.h>
#ifdef __sh__
#include <yaul.h>
#endif
static const uint8_t *g;
static uint32_t tiles,palettes;
static uint16_t tiles_n;
/* The board's FG/BG IDs fit in13 bits. Keep this immutable translation in
 * HWRAM: animation writes otherwise binary-search the same slow LWRAM IDs
 * hundreds of times on one tick. Larger public API IDs retain the general
 * sorted-table fallback, so the accepted ring-pack format is unchanged. */
#define SOURCE_TILE_DIRECT_COUNT 8192u
static uint16_t source_tile_direct[SOURCE_TILE_DIRECT_COUNT];
static uint16_t overlay[2][1024][2];
static uint8_t overlay_valid[2][1024];
static uint8_t overlay_dirty[2][1024];
static uint16_t dirty_cells[2][1024],dirty_count[2];
static int last[5];
static int16_t surface_x[2][512],surface_y[2][256];
static uint32_t surface_generation[2],ring_generation=1u;
static uint8_t surface_transparent[2];
static uint16_t cell_x[2][32],cell_y[2][32];
static uint32_t cell_generation[2];
static uint16_t u16(const uint8_t *p){return (uint16_t)((p[0]<<8)|p[1]);}
static uint32_t u32(const uint8_t *p){return ((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|(p[2]<<8)|p[3];}
int wf_r18_ring_bind(const void *blob,size_t n){
 const uint8_t *p=blob;
 g=0;
 if(!p||n<544||memcmp(p,"R18R",4)||u16(p+4)!=2||u32(p+12)!=32||u32(p+24)!=160||u32(p+28)!=128)return -1;
 uint32_t t=u32(p+16),c=u32(p+20);uint16_t count=u16(p+10);
 if(t<544||c<t||count>1024u||(size_t)t+count*130u!=c||(size_t)c+1024!=n)return -1;
 for(unsigned page=0;page<128;page++){
  uint32_t off=u32(p+32+page*4);
  if(off<544||(size_t)off+1280>t)return -1;
  for(unsigned i=0;i<320;i++)if(u16(p+off+i*4+2)>=count)return -1;
 }
 const uint8_t *ids=p+t+count*128u;
 for(unsigned i=1;i<count;i++)if(u16(ids+(i-1u)*2u)>=u16(ids+i*2u))return -1;
 memset(source_tile_direct,0xff,sizeof source_tile_direct);
 for(unsigned i=0;i<count;i++){
  unsigned source=u16(ids+i*2u);
  if(source<SOURCE_TILE_DIRECT_COUNT)source_tile_direct[source]=(uint16_t)i;
 }
 g=p;tiles=t;palettes=c;tiles_n=count;memset(overlay_valid,0,sizeof overlay_valid);
 memset(overlay_dirty,0,sizeof overlay_dirty);dirty_count[0]=dirty_count[1]=0;
 ++ring_generation;surface_generation[0]=0;surface_generation[1]=0;
 cell_generation[0]=0;cell_generation[1]=0;
 for(unsigned i=0;i<5;i++)last[i]=0x10000;
#ifdef __sh__
 vdp2_cram_mode_set(2);
 for(unsigned layer=0;layer<2;layer++){
  vdp2_scrn_cell_format_t f={.scroll_screen=layer?VDP2_SCRN_NBG1:VDP2_SCRN_NBG0,.ccc=VDP2_SCRN_CCC_PALETTE_16,.char_size=VDP2_SCRN_CHAR_SIZE_2X2,.pnd_size=1,.aux_mode=VDP2_SCRN_AUX_MODE_0,.plane_size=VDP2_SCRN_PLANE_SIZE_1X1,.palette_base=VDP2_CRAM_MODE_2_OFFSET((layer?WF_SATURN_CRAM_BG_FIRST:WF_SATURN_CRAM_FG_FIRST)/256u,0,0),.cpd_base=VDP2_VRAM_ADDR(layer?2:0,0)};
  vdp2_vram_t names=VDP2_VRAM_ADDR(1,layer?WF_SATURN_VDP2_BG_NAMES:WF_SATURN_VDP2_FG_NAMES);
  vdp2_scrn_normal_map_t map={.plane_a=names,.plane_b=names,.plane_c=names,.plane_d=names};
  vdp2_scrn_cell_format_set(&f,&map);
  if(wf_r18_ring_characters((volatile uint8_t *)f.cpd_base,0x20000u))return -1;
  vdp2_scrn_scroll_x_set(f.scroll_screen,0);vdp2_scrn_scroll_y_set(f.scroll_screen,0);
  for(unsigned color=0;color<256;color++)wf_saturn_cram_write((uint16_t)((layer?WF_SATURN_CRAM_BG_FIRST:WF_SATURN_CRAM_FG_FIRST)+color),u16(g+palettes+layer*512+color*2));
 }
 vdp2_vram_cycp_t *cy=vdp2_vram_cycp_get();
 /* Manual3.3: one name read per layer; 16 colors require one character
  * access at a permitted later timing. Paired CPU access T7 follows idle T6. */
 for(unsigned bank=0;bank<4;bank++){cy->pt[bank].raw=0xffffffffu;cy->pt[bank].t7=0xe;}
 cy->pt[1].t0=VDP2_VRAM_CYCP_PNDR_NBG0;cy->pt[1].t1=VDP2_VRAM_CYCP_PNDR_NBG1;
 cy->pt[1].t2=VDP2_VRAM_CYCP_PNDR_NBG2;
 cy->pt[0].t4=VDP2_VRAM_CYCP_CHPNDR_NBG0;cy->pt[2].t5=VDP2_VRAM_CYCP_CHPNDR_NBG1;
 cy->pt[3].t3=VDP2_VRAM_CYCP_CHPNDR_NBG2;
 vdp2_vram_cycp_set(cy);
#endif
 return 0;
}
int wf_r18_ring_pixel(unsigned layer,int wx,int wy,uint8_t *index){
 if(!g||!index||layer>1)return -1;
 wx%=2560;wy%=2048;if(wx<0)wx+=2560;if(wy<0)wy+=2048;
 unsigned tx=(unsigned)wx>>4,ty=(unsigned)wy>>4;
 unsigned page=(ty/16)*8+tx/20;
 uint32_t po=u32(g+32+(layer*64+page)*4);
 const uint8_t *cell=g+po+((ty%16)*20+tx%20)*4;
 unsigned attr=u16(cell),tile=u16(cell+2),x=(unsigned)wx&15,y=(unsigned)wy&15;
 unsigned physical=(ty&31)*32+(tx&31);
 if(overlay_valid[layer][physical]){attr=overlay[layer][physical][0];tile=overlay[layer][physical][1];}
 if(attr&64)x=15-x;
 if(attr&128)y=15-y;
 uint8_t v=g[tiles+tile*128+y*8+x/2];v=(x&1)?v&15:v>>4;
 *index=(uint8_t)(((attr&15)<<4)|v);return 0;
}
int wf_r18_ring_scanline(unsigned layer,int wx,int wy,unsigned transparent,uint8_t out[320]){
 if(!g||!out||layer>1)return -1;
 wx%=2560;wy%=2048;if(wx<0)wx+=2560;if(wy<0)wy+=2048;
 unsigned ty=(unsigned)wy>>4;
 unsigned row=(unsigned)wy&15u;
 unsigned screen=0;
 while(screen<320u){
  unsigned tx=(unsigned)wx>>4;
  unsigned po=u32(g+32+((ty/16u)*8u+tx/20u+layer*64u)*4u);
  const uint8_t *cell=g+po+((ty%16u)*20u+tx%20u)*4u;
  unsigned attr=u16(cell),tile=u16(cell+2);
  unsigned physical=(ty&31u)*32u+(tx&31u);
  if(overlay_valid[layer][physical]){
   attr=overlay[layer][physical][0];tile=overlay[layer][physical][1];
  }
  unsigned local=(unsigned)wx&15u;
  unsigned run=16u-local;
  if(run>320u-screen)run=320u-screen;
  unsigned sample_row=(attr&128u)?15u-row:row;
  const uint8_t *pixels=g+tiles+tile*128u+sample_row*8u;
  unsigned bank=(attr&15u)<<4;
  for(unsigned i=0;i<run;i++){
   unsigned px=local+i;
   if(attr&64u)px=15u-px;
   uint8_t packed=pixels[px>>1];
   unsigned nibble=(px&1u)?packed&15u:packed>>4;
   out[screen+i]=(uint8_t)((transparent&&!nibble)?0u:bank|nibble);
  }
  screen+=run;wx+=(int)run;if(wx>=2560)wx-=2560;
 }
 return 0;
}
/* Surface coordinates are already wrapped. Reuse a decoded map cell while
 * sampling its column/dirty block; do not repeat page divisions and LWRAM
 * map reads for every one of its 256 pixels. The public pixel lookup remains
 * an independent slow path for reference comparisons. */
typedef struct ring_sample {unsigned tx,ty,attr,tile;int valid;} ring_sample_t;
static uint8_t surface_pixel(unsigned layer,unsigned wx,unsigned wy,ring_sample_t*c){
 unsigned tx=wx>>4,ty=wy>>4;
 if(!c->valid||c->tx!=tx||c->ty!=ty){
  unsigned physical=(ty&31u)*32u+(tx&31u);
  if(overlay_valid[layer][physical]){
   c->attr=overlay[layer][physical][0];c->tile=overlay[layer][physical][1];
  }else{
   unsigned po=u32(g+32+(layer*64u+(ty/16u)*8u+tx/20u)*4u);
   const uint8_t *cell=g+po+((ty%16u)*20u+tx%20u)*4u;
   c->attr=u16(cell);c->tile=u16(cell+2);
  }
  c->tx=tx;c->ty=ty;c->valid=1;
 }
 unsigned x=wx&15u,y=wy&15u;
 if(c->attr&64u)x=15u-x;
 if(c->attr&128u)y=15u-y;
 uint8_t packed=g[tiles+c->tile*128u+y*8u+x/2u];
 return (uint8_t)(((c->attr&15u)<<4)|((x&1u)?packed&15u:packed>>4));
}
int wf_r18_ring_surface_update(unsigned layer,int wx,int wy,unsigned transparent,
                               volatile uint8_t *bitmap,size_t *writes){
 if(!g||!bitmap||layer>1u)return -1;
 transparent=transparent?1u:0u;
 if(surface_generation[layer]!=ring_generation||
    surface_transparent[layer]!=transparent){
  memset(surface_x[layer],0xff,sizeof surface_x[layer]);
  memset(surface_y[layer],0xff,sizeof surface_y[layer]);
  surface_generation[layer]=ring_generation;
  surface_transparent[layer]=(uint8_t)transparent;
 }
 uint8_t dirty_x[512]={0},dirty_y[256]={0},visible_x[512]={0};
 for(unsigned x=0;x<320u;x++){
  int world=(wx+(int)x)%2560;if(world<0)world+=2560;
  unsigned slot=(unsigned)world&511u;
  visible_x[slot]=1;
  if(surface_x[layer][slot]!=world){surface_x[layer][slot]=(int16_t)world;dirty_x[slot]=1;}
 }
 for(unsigned y=0;y<240u;y++){
  int world=(wy+(int)y)%2048;if(world<0)world+=2048;
  unsigned slot=(unsigned)world&255u;
  if(surface_y[layer][slot]!=world){surface_y[layer][slot]=(int16_t)world;dirty_y[slot]=1;}
 }
 size_t count=0;
 for(unsigned x=0;x<512u;x++)if(dirty_x[x]){
  ring_sample_t sample={0};
  for(unsigned y=0;y<256u;y++)if(surface_y[layer][y]>=0&&!dirty_y[y]){
   uint8_t pixel=surface_pixel(layer,(unsigned)surface_x[layer][x],(unsigned)surface_y[layer][y],&sample);
   bitmap[y*512u+x]=(uint8_t)((transparent&&!(pixel&15u))?0u:pixel);
   ++count;
  }
 }
 uint8_t line[320];
 for(unsigned y=0;y<256u;y++)if(dirty_y[y]){
  if(wf_r18_ring_scanline(layer,wx,surface_y[layer][y],transparent,line)!=0)
   return -1;
  for(unsigned x=0;x<320u;x++){
   unsigned slot=(uint32_t)(wx+(int)x)&511u;
   bitmap[y*512u+slot]=line[x];
   ++count;
  }
  ring_sample_t sample={0};
  for(unsigned x=0;x<512u;x++)if(surface_x[layer][x]>=0&&!visible_x[x]){
   uint8_t pixel=surface_pixel(layer,(unsigned)surface_x[layer][x],(unsigned)surface_y[layer][y],&sample);
   bitmap[y*512u+x]=(uint8_t)((transparent&&!(pixel&15u))?0u:pixel);
   ++count;
  }
 }
 for(unsigned i=0;i<dirty_count[layer];i++){
  unsigned cell=dirty_cells[layer][i];
  unsigned cx=cell&31u,cy=cell>>5;
  ring_sample_t sample={0};
  for(unsigned py=0;py<16u;py++){
   unsigned y=(cy*16u+py)&255u;
   int world_y=surface_y[layer][y];
   if(world_y<0||((unsigned)world_y>>4&31u)!=cy)continue;
   for(unsigned px=0;px<16u;px++){
    unsigned x=cx*16u+px;
    int world_x=surface_x[layer][x];
    if(world_x<0||((unsigned)world_x>>4&31u)!=cx)continue;
    uint8_t pixel=surface_pixel(layer,(unsigned)world_x,(unsigned)world_y,&sample);
    bitmap[y*512u+x]=(uint8_t)((transparent&&!(pixel&15u))?0u:pixel);
    ++count;
   }
  }
  overlay_dirty[layer][cell]=0;
 }
 dirty_count[layer]=0;
 if(writes)*writes=count;
 return 0;
}
int wf_r18_ring_characters(volatile uint8_t *dest,size_t capacity){
 if(!g||!dest||capacity<(size_t)tiles_n*128u)return -1;
 for(unsigned t=0;t<tiles_n;t++)for(unsigned q=0;q<4;q++)
  for(unsigned y=0;y<8;y++)for(unsigned x=0;x<4;x++)
   dest[t*128u+q*32u+y*4u+x]=g[tiles+t*128u+((q/2u)*8u+y)*8u+(q%2u)*4u+x];
 return 0;
}
int wf_r18_ring_cells_update(unsigned layer,int wx,int wy,volatile uint16_t *map,size_t *writes){
 if(!g||layer>1||!map)return -1;
 if(cell_generation[layer]!=ring_generation){
  memset(cell_x[layer],0xff,sizeof cell_x[layer]);memset(cell_y[layer],0xff,sizeof cell_y[layer]);cell_generation[layer]=ring_generation;
 }
 wx%=2560;wy%=2048;if(wx<0)wx+=2560;if(wy<0)wy+=2048;
 uint8_t changed_x[32]={0},changed_y[32]={0};unsigned changed=0;
 for(unsigned i=0;i<=((unsigned)(wx&15)+319u)/16u;i++){
  unsigned tile=((unsigned)wx/16u+i)%160u,slot=tile&31u;
  if(cell_x[layer][slot]!=tile){cell_x[layer][slot]=(uint16_t)tile;changed_x[slot]=1;changed=1;}
 }
 for(unsigned i=0;i<=((unsigned)(wy&15)+239u)/16u;i++){
  unsigned tile=((unsigned)wy/16u+i)%128u,slot=tile&31u;
  if(cell_y[layer][slot]!=tile){cell_y[layer][slot]=(uint16_t)tile;changed_y[slot]=1;changed=1;}
 }
 /* Pixel scrolling is done by VDP2. If the resident tile coordinates and
  * overlays did not change, all1024 pattern names are already correct.
  * Keep updating the scroll registers in ring_update, without rescanning
  * the cached row/column cross-product on every one-pixel camera step. */
 if(!changed&&!dirty_count[layer]){if(writes)*writes=0;return 0;}
 size_t count=0;ring_sample_t sample={0};
 /* Keep the cached X/Y cross-product coherent, including offscreen cells.
  * A returning row must never reuse an old column's pattern name. */
 for(unsigned y=0;y<32;y++)if(cell_y[layer][y]!=0xffffu)
  for(unsigned x=0;x<32;x++)if(cell_x[layer][x]!=0xffffu){
   unsigned cell=y*32u+x;
   if(!changed_x[x]&&!changed_y[y]&&!overlay_dirty[layer][cell])continue;
   (void)surface_pixel(layer,(unsigned)cell_x[layer][x]*16u,(unsigned)cell_y[layer][y]*16u,&sample);
   map[cell]=(uint16_t)(((sample.attr&15u)<<12)|((sample.attr&0xc0u)<<4)|sample.tile);count++;
  }
 for(unsigned i=0;i<dirty_count[layer];i++)overlay_dirty[layer][dirty_cells[layer][i]]=0;
 dirty_count[layer]=0;if(writes)*writes=count;return 0;
}
#ifdef __sh__
static void ring_show(uint8_t pri){
 /* No screen or sprite priority is exposed until all pattern names have
  * reached VRAM. The frontend uses its own gated commit instead. */
 for(unsigned i=0;i<8u;i++)vdp2_sprite_priority_set(i,pri==0x7c?2:3);
 vdp2_scrn_display_set(VDP2_SCRN_DISPTP_NBG2|(pri==0x78?(VDP2_SCRN_DISPTP_NBG0|VDP2_SCRN_DISP_NBG1):(VDP2_SCRN_DISP_NBG0|VDP2_SCRN_DISPTP_NBG1)));
}
#endif
static int ring_update(int16_t sx0,int16_t sy0,int16_t sx1,int16_t sy1,uint8_t pri,unsigned show){
 if(!g||(pri!=0x78&&pri!=0x7b&&pri!=0x7c))return -1;
 if(last[0]==sx0&&last[1]==sy0&&last[2]==sx1&&last[3]==sy1&&last[4]==pri){
#ifdef __sh__
  if(show)ring_show(pri);
#else
  (void)show;
#endif
  return 0;
 }
#ifdef __sh__
 for(unsigned layer=0;layer<2;layer++){
  int first=(pri==0x78)?layer==0:layer==1;
  int sx=first?sx0:sx1,sy=first?sy0:sy1;
  int wy=2048-sy;
  vdp2_scrn_t screen=layer?VDP2_SCRN_NBG1:VDP2_SCRN_NBG0;
  volatile uint16_t *vram=(volatile uint16_t *)VDP2_VRAM_ADDR(1,layer?WF_SATURN_VDP2_BG_NAMES:WF_SATURN_VDP2_FG_NAMES);
  if(wf_r18_ring_cells_update(layer,sx,wy,vram,NULL)!=0)
   return -1;
  vdp2_scrn_scroll_x_set(screen,fix16_int32_from((uint32_t)sx&511u));
  vdp2_scrn_scroll_y_set(screen,fix16_int32_from((uint32_t)wy&511u));
 }
 vdp2_scrn_priority_set(VDP2_SCRN_NBG0,pri==0x78?2:1);
 vdp2_scrn_priority_set(VDP2_SCRN_NBG1,pri==0x78?1:pri==0x7b?2:3);
 if(show)ring_show(pri);
#endif
 last[0]=sx0;last[1]=sy0;last[2]=sx1;last[3]=sy1;last[4]=pri;return 0;
}
int wf_r18_ring_update(int16_t sx0,int16_t sy0,int16_t sx1,int16_t sy1,uint8_t pri){
 return ring_update(sx0,sy0,sx1,sy1,pri,1u);
}
int wf_r18_ring_update_deferred(int16_t sx0,int16_t sy0,int16_t sx1,int16_t sy1,uint8_t pri){
 return ring_update(sx0,sy0,sx1,sy1,pri,0u);
}

static void mark_overlay_dirty(unsigned layer,unsigned cell){
 if(!overlay_dirty[layer][cell]){
  overlay_dirty[layer][cell]=1;
  dirty_cells[layer][dirty_count[layer]++]=(uint16_t)cell;
 }
 last[4]=0x10000;
}
static int source_tile_index(uint16_t source_tile,unsigned *tile){
 if(!g)return -1;
 if(source_tile<SOURCE_TILE_DIRECT_COUNT){
  unsigned index=source_tile_direct[source_tile];
  if(index==0xffffu)return -1;
  *tile=index;return 0;
 }
 /* The pack's strictly sorted source-ID table is validated at bind. */
 const uint8_t *ids=g+tiles+tiles_n*128u;unsigned lo=0,hi=tiles_n;
 while(lo<hi){unsigned mid=lo+(hi-lo)/2u;if(u16(ids+mid*2u)<source_tile)lo=mid+1u;else hi=mid;}
 if(lo==tiles_n||u16(ids+lo*2u)!=source_tile)return -1;
 *tile=lo;return 0;
}
int wf_r18_ring_tile_write(unsigned layer,unsigned cell,uint16_t attr,uint16_t source_tile){
 unsigned tile;if(layer>1||cell>=1024||source_tile_index(source_tile,&tile))return -1;
 if(!overlay_valid[layer][cell]||overlay[layer][cell][0]!=attr||
    overlay[layer][cell][1]!=tile)mark_overlay_dirty(layer,cell);
 overlay[layer][cell][0]=attr;overlay[layer][cell][1]=(uint16_t)tile;
 overlay_valid[layer][cell]=1;return 0;
}
static int video_cell(unsigned layer,unsigned cell,uint8_t *bytes,wf_r18_background_op_t op){
 if(!g||layer>1||cell>=1024||!bytes)return -1;
 if(op==WF_R18_BACKGROUND_READ){
  unsigned attr,tile;
  if(overlay_valid[layer][cell]){attr=overlay[layer][cell][0];tile=overlay[layer][cell][1];}
  else{
   unsigned x=cell&31u,y=cell>>5;
   /* An untouched physical cell has no decoded resident coordinate yet;
    * keep its board shadow until the camera first loads that coordinate. */
   if(cell_generation[layer]!=ring_generation||cell_x[layer][x]==0xffffu||cell_y[layer][y]==0xffffu)return 0;
   ring_sample_t sample={0};
   (void)surface_pixel(layer,cell_x[layer][x]*16u,cell_y[layer][y]*16u,&sample);
   attr=sample.attr;tile=sample.tile;
  }
  uint16_t source=u16(g+tiles+tiles_n*128u+tile*2u);
  if(layer){uint16_t word=(uint16_t)((attr<<12)|source);bytes[0]=(uint8_t)(word>>8);bytes[1]=(uint8_t)word;}
  else{bytes[0]=(uint8_t)(attr>>8);bytes[1]=(uint8_t)attr;bytes[2]=(uint8_t)(source>>8);bytes[3]=(uint8_t)source;}
  return 0;
 }
 uint16_t attr=u16(bytes),source;unsigned tile;
 if(layer){source=attr&4095u;attr>>=12;}else source=u16(bytes+2)&8191u;
 if(op==WF_R18_BACKGROUND_VALIDATE)return source_tile_index(source,&tile);
 return op==WF_R18_BACKGROUND_WRITE?wf_r18_ring_tile_write(layer,cell,attr,source):-1;
}
void wf_r18_video_ring_bind(wf_r18_video_t *video){if(video)video->ring_cell=video_cell;}
void wf_r18_ring_tile_restore(unsigned layer,unsigned cell){
 if(layer<2&&cell<1024&&overlay_valid[layer][cell]){
  overlay_valid[layer][cell]=0;mark_overlay_dirty(layer,cell);
 }
}
int wf_r18_ring_palette_set(unsigned layer,const uint8_t *strided,size_t n){
 if(layer>1||!strided||n<0x800)return -1;
#ifdef __sh__
 for(unsigned bank=0;bank<16;bank++)wf_saturn_cram_write_bank((uint16_t)((layer?WF_SATURN_CRAM_BG_FIRST:WF_SATURN_CRAM_FG_FIRST)+bank*16u),strided+bank*128u);
#endif
 return 0;
}
