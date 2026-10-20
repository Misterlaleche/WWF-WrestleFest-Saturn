#include "wf_r18_ring.h"
#include "wf_saturn_memory.h"
#include <string.h>
#ifdef __sh__
#include <yaul.h>
#endif
static const uint8_t *g;
static uint32_t tiles,palettes;
static uint16_t tiles_n;
static uint16_t overlay[2][1024][2];
static uint8_t overlay_valid[2][1024];
static uint8_t overlay_dirty[2][1024];
static uint16_t dirty_cells[2][1024],dirty_count[2];
static int last[5];
static int16_t surface_x[2][512],surface_y[2][256];
static uint32_t surface_generation[2],ring_generation=1u;
static uint8_t surface_transparent[2];
static uint16_t u16(const uint8_t *p){return (uint16_t)((p[0]<<8)|p[1]);}
static uint32_t u32(const uint8_t *p){return ((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|(p[2]<<8)|p[3];}
int wf_r18_ring_bind(const void *blob,size_t n){
 const uint8_t *p=blob;
 g=0;
 if(!p||n<544||memcmp(p,"R18R",4)||u16(p+4)!=2||u32(p+12)!=32||u32(p+24)!=160||u32(p+28)!=128)return -1;
 uint32_t t=u32(p+16),c=u32(p+20);uint16_t count=u16(p+10);
 if(t<544||c<t||(size_t)t+count*130u!=c||(size_t)c+1024!=n)return -1;
 for(unsigned page=0;page<128;page++){
  uint32_t off=u32(p+32+page*4);
  if(off<544||(size_t)off+1280>t)return -1;
  for(unsigned i=0;i<320;i++)if(u16(p+off+i*4+2)>=count)return -1;
 }
 g=p;tiles=t;palettes=c;tiles_n=count;memset(overlay_valid,0,sizeof overlay_valid);
 memset(overlay_dirty,0,sizeof overlay_dirty);dirty_count[0]=dirty_count[1]=0;
 ++ring_generation;surface_generation[0]=0;surface_generation[1]=0;
 for(unsigned i=0;i<5;i++)last[i]=0x10000;
#ifdef __sh__
 for(unsigned layer=0;layer<2;layer++){
  vdp2_scrn_bitmap_format_t f={.scroll_screen=layer?VDP2_SCRN_NBG1:VDP2_SCRN_NBG0,.ccc=VDP2_SCRN_CCC_PALETTE_256,.bitmap_size=VDP2_SCRN_BITMAP_SIZE_512X256,.palette_base=VDP2_CRAM_MODE_2_OFFSET((layer?WF_SATURN_CRAM_BG_FIRST:WF_SATURN_CRAM_FG_FIRST)/256u,0,0),.bitmap_base=VDP2_VRAM_ADDR(layer?2:0,0)};
  vdp2_scrn_bitmap_format_set(&f);
  vdp2_scrn_scroll_x_set(f.scroll_screen,0);vdp2_scrn_scroll_y_set(f.scroll_screen,0);
  volatile uint32_t *cram=(volatile uint32_t *)f.palette_base;
  for(unsigned color=0;color<256;color++){uint16_t v=u16(g+palettes+layer*512+color*2);unsigned r=(v&15)*17,green=((v>>4)&15)*17,b=((v>>8)&15)*17;cram[color]=(b<<16)|(green<<8)|r;}
 }
 vdp2_cram_mode_set(2);
 vdp2_vram_cycp_t *cy=vdp2_vram_cycp_get();
 cy->pt[0].raw=0x4444eeee;cy->pt[2].raw=0x5555eeee;vdp2_vram_cycp_set(cy);
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
 for(unsigned x=0;x<512u;x++)if(dirty_x[x])
  for(unsigned y=0;y<256u;y++)if(surface_y[layer][y]>=0&&!dirty_y[y]){
   uint8_t pixel;
   if(wf_r18_ring_pixel(layer,surface_x[layer][x],surface_y[layer][y],
                        &pixel)!=0)return -1;
   bitmap[y*512u+x]=(uint8_t)((transparent&&!(pixel&15u))?0u:pixel);
   ++count;
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
  for(unsigned x=0;x<512u;x++)if(surface_x[layer][x]>=0&&!visible_x[x]){
   uint8_t pixel;
   if(wf_r18_ring_pixel(layer,surface_x[layer][x],surface_y[layer][y],
                        &pixel)!=0)return -1;
   bitmap[y*512u+x]=(uint8_t)((transparent&&!(pixel&15u))?0u:pixel);
   ++count;
  }
 }
 for(unsigned i=0;i<dirty_count[layer];i++){
  unsigned cell=dirty_cells[layer][i];
  unsigned cx=cell&31u,cy=cell>>5;
  for(unsigned py=0;py<16u;py++){
   unsigned y=(cy*16u+py)&255u;
   int world_y=surface_y[layer][y];
   if(world_y<0||((unsigned)world_y>>4&31u)!=cy)continue;
   for(unsigned px=0;px<16u;px++){
    unsigned x=cx*16u+px;
    int world_x=surface_x[layer][x];
    if(world_x<0||((unsigned)world_x>>4&31u)!=cx)continue;
    uint8_t pixel;
    if(wf_r18_ring_pixel(layer,world_x,world_y,&pixel)!=0)return -1;
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
int wf_r18_ring_update(int16_t sx0,int16_t sy0,int16_t sx1,int16_t sy1,uint8_t pri){
 if(!g||(pri!=0x78&&pri!=0x7b&&pri!=0x7c))return -1;
 if(last[0]==sx0&&last[1]==sy0&&last[2]==sx1&&last[3]==sy1&&last[4]==pri)return 0;
#ifdef __sh__
 vdp2_scrn_priority_set(VDP2_SCRN_NBG0,pri==0x78?2:1);
 vdp2_scrn_priority_set(VDP2_SCRN_NBG1,pri==0x78?1:pri==0x7b?2:3);
 vdp2_scrn_display_set(pri==0x78?(VDP2_SCRN_DISPTP_NBG0|VDP2_SCRN_DISP_NBG1):(VDP2_SCRN_DISP_NBG0|VDP2_SCRN_DISPTP_NBG1));
 for(unsigned layer=0;layer<2;layer++){
  int first=(pri==0x78)?layer==0:layer==1;
  int sx=first?sx0:sx1,sy=first?sy0:sy1;
  int transparent=(pri==0x78)?layer==0:layer==1;
  int wy=2048-sy;
  vdp2_scrn_t screen=layer?VDP2_SCRN_NBG1:VDP2_SCRN_NBG0;
  volatile uint8_t *vram=(volatile uint8_t *)VDP2_VRAM_ADDR(layer?2:0,0);
  if(wf_r18_ring_surface_update(layer,sx,wy,(unsigned)transparent,vram,NULL)!=0)
   return -1;
  vdp2_scrn_scroll_x_set(screen,fix16_int32_from((uint32_t)sx&511u));
  vdp2_scrn_scroll_y_set(screen,fix16_int32_from((uint32_t)wy&255u));
 }
#endif
 last[0]=sx0;last[1]=sy0;last[2]=sx1;last[3]=sy1;last[4]=pri;return 0;
}

static void mark_overlay_dirty(unsigned layer,unsigned cell){
 if(!overlay_dirty[layer][cell]){
  overlay_dirty[layer][cell]=1;
  dirty_cells[layer][dirty_count[layer]++]=(uint16_t)cell;
 }
 last[4]=0x10000;
}
int wf_r18_ring_tile_write(unsigned layer,unsigned cell,uint16_t attr,uint16_t source_tile){
 if(!g||layer>1||cell>=1024)return -1;
 unsigned tile;for(tile=0;tile<tiles_n;tile++)if(u16(g+tiles+tiles_n*128+tile*2)==source_tile)break;
 if(tile==tiles_n)return -1;
 if(!overlay_valid[layer][cell]||overlay[layer][cell][0]!=attr||
    overlay[layer][cell][1]!=tile)mark_overlay_dirty(layer,cell);
 overlay[layer][cell][0]=attr;overlay[layer][cell][1]=(uint16_t)tile;
 overlay_valid[layer][cell]=1;return 0;
}
void wf_r18_ring_tile_restore(unsigned layer,unsigned cell){
 if(layer<2&&cell<1024&&overlay_valid[layer][cell]){
  overlay_valid[layer][cell]=0;mark_overlay_dirty(layer,cell);
 }
}
int wf_r18_ring_palette_set(unsigned layer,const uint8_t *strided,size_t n){
 if(layer>1||!strided||n<0x800)return -1;
#ifdef __sh__
 volatile uint32_t *cram=(volatile uint32_t *)VDP2_CRAM_MODE_2_OFFSET((layer?WF_SATURN_CRAM_BG_FIRST:WF_SATURN_CRAM_FG_FIRST)/256u,0,0);
 for(unsigned bank=0;bank<16;bank++)for(unsigned pen=0;pen<16;pen++){
  uint16_t v=u16(strided+bank*128+pen*2);unsigned r=(v&15)*17,green=((v>>4)&15)*17,b=((v>>8)&15)*17;
  cram[bank*16+pen]=(b<<16)|(green<<8)|r;
 }
#endif
 return 0;
}
