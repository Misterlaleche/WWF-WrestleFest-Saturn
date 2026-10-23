#include <assert.h>
#include <stdio.h>
#include <yaul.h>
#include "wf_r18_ring.h"
#include "wf_saturn_memory.h"
uint8_t mock_vdp2_vram[0x80000];uint32_t mock_cram[1024];
unsigned mock_sprite_priorities[8],mock_screen_priorities[3],mock_display;
static uint8_t blob[114918];
/* Independent board-order fixture (WP26 and MAME):7C FG,sprites,BG;
 *7B FG,BG,sprites;78 BG,FG,sprites. Test actual hardware configuration
 * through visible overlapping pixels, rather than a fixed priority number. */
static unsigned visible(unsigned fg,unsigned bg,unsigned sprite,unsigned klass){
 unsigned p=0,color=0;
 unsigned transparent[2]={!(mock_display&0x100u),!(mock_display&0x200u)};
 unsigned pixels[2]={fg,bg};
 for(unsigned l=0;l<2;l++)if((!transparent[l]||(pixels[l]&15u))&&mock_screen_priorities[l]>p){p=mock_screen_priorities[l];color=10+l;}
 if(sprite&&mock_sprite_priorities[klass]>p)color=20;
 return color;
}
int main(int argc,char**argv){
 assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f&&fread(blob,1,sizeof blob,f)==sizeof blob);fclose(f);assert(!wf_r18_ring_bind(blob,sizeof blob));
 uint8_t palette[0x800]={0};palette[3]=15;
 wf_saturn_cram_reset();assert(!wf_r18_ring_palette_set(0,palette,sizeof palette));
 assert(mock_cram[1]==255u&&wf_saturn_cram_writes==256);
 assert(!wf_r18_ring_palette_set(0,palette,sizeof palette));assert(wf_saturn_cram_writes==256);
 palette[3]=14;assert(!wf_r18_ring_palette_set(0,palette,sizeof palette));
 assert(mock_cram[1]==238u&&wf_saturn_cram_writes==257);
 const unsigned modes[]={0x7c,0x7b,0x78};
 for(unsigned m=0;m<3;m++){
  for(unsigned k=0;k<8;k++)mock_sprite_priorities[k]=4; /* actual old entry */
  assert(!wf_r18_ring_update(480,560,480,560,(uint8_t)modes[m]));
  for(unsigned k=0;k<8;k++){
   assert(visible(1,1,1,k)==(m==0?11u:20u));
   assert(visible(1,0,1,k)==20u); /* transparent BG pen0 never hides sprite */
  }
  assert(visible(1,1,0,0)==(m==2?10u:11u));
  assert(visible(0,0,0,0)==(m==2?11u:10u)); /* opaque base includes pen0 */
 }
 puts("PASS Saturn ring priority: foreground rope occludes wrestlers/side ropes in7C;7B/78 preserve their board order,8sprite classes and pen0");return 0;
}
