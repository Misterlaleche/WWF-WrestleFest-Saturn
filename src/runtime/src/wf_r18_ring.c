#include "wf_r18_ring.h"
#include <string.h>
#ifdef __sh__
#include <yaul.h>
#endif
static const uint8_t *g;
static uint32_t tiles,palettes;
static uint16_t tiles_n;
static uint16_t overlay[2][1024][2];
static uint8_t overlay_valid[2][1024];
static int last[5];
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
 for(unsigned i=0;i<5;i++)last[i]=0x10000;
#ifdef __sh__
 for(unsigned layer=0;layer<2;layer++){
  vdp2_scrn_bitmap_format_t f={.scroll_screen=layer?VDP2_SCRN_NBG1:VDP2_SCRN_NBG0,.ccc=VDP2_SCRN_CCC_PALETTE_256,.bitmap_size=VDP2_SCRN_BITMAP_SIZE_512X256,.palette_base=VDP2_CRAM_MODE_2_OFFSET(2+layer,0,0),.bitmap_base=VDP2_VRAM_ADDR(layer,0)};
  vdp2_scrn_bitmap_format_set(&f);
  vdp2_scrn_scroll_x_set(f.scroll_screen,0);vdp2_scrn_scroll_y_set(f.scroll_screen,0);
  volatile uint32_t *cram=(volatile uint32_t *)f.palette_base;
  for(unsigned color=0;color<256;color++){uint16_t v=u16(g+palettes+layer*512+color*2);unsigned r=(v&15)*17,green=((v>>4)&15)*17,b=((v>>8)&15)*17;cram[color]=(b<<16)|(green<<8)|r;}
 }
 vdp2_cram_mode_set(2);
 vdp2_vram_cycp_t *cy=vdp2_vram_cycp_get();
 cy->pt[0].raw=0x44eeeeee;cy->pt[1].raw=0x55eeeeee;vdp2_vram_cycp_set(cy);
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
  volatile uint16_t *vram=(volatile uint16_t *)VDP2_VRAM_ADDR(layer,0);
  for(unsigned y=0;y<240;y++)for(unsigned x=0;x<320;x+=2){
   uint8_t a,b;wf_r18_ring_pixel(layer,sx+(int)x,2048-sy+(int)y,&a);wf_r18_ring_pixel(layer,sx+(int)x+1,2048-sy+(int)y,&b);
   if(transparent){if(!(a&15))a=0;if(!(b&15))b=0;}
   vram[y*256+x/2]=(uint16_t)((a<<8)|b);
  }
 }
#endif
 last[0]=sx0;last[1]=sy0;last[2]=sx1;last[3]=sy1;last[4]=pri;return 0;
}

int wf_r18_ring_tile_write(unsigned layer,unsigned cell,uint16_t attr,uint16_t source_tile){
 if(!g||layer>1||cell>=1024)return -1;
 unsigned tile;for(tile=0;tile<tiles_n;tile++)if(u16(g+tiles+tiles_n*128+tile*2)==source_tile)break;
 if(tile==tiles_n)return -1;
 overlay[layer][cell][0]=attr;overlay[layer][cell][1]=(uint16_t)tile;overlay_valid[layer][cell]=1;last[4]=0x10000;return 0;
}
void wf_r18_ring_tile_restore(unsigned layer,unsigned cell){if(layer<2&&cell<1024){overlay_valid[layer][cell]=0;last[4]=0x10000;}}
int wf_r18_ring_palette_set(unsigned layer,const uint8_t *strided,size_t n){
 if(layer>1||!strided||n<0x800)return -1;
#ifdef __sh__
 volatile uint32_t *cram=(volatile uint32_t *)VDP2_CRAM_MODE_2_OFFSET(2+layer,0,0);
 for(unsigned bank=0;bank<16;bank++)for(unsigned pen=0;pen<16;pen++){
  uint16_t v=u16(strided+bank*128+pen*2);unsigned r=(v&15)*17,green=((v>>4)&15)*17,b=((v>>8)&15)*17;
  cram[bank*16+pen]=(b<<16)|(green<<8)|r;
 }
#endif
 return 0;
}
