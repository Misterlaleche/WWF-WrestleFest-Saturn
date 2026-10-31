#include "wf_video_plan.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static wf_video_plan_t plan;static uint8_t bg[4096],text[8192],pal[0x800],spr[16];
static unsigned uploads,loaded[2][1024];
static bool upload(void*u,unsigned layer,unsigned slot,unsigned code){(void)u;assert(layer<2&&slot<1024&&code<4096);loaded[layer][slot]=code;uploads++;return true;}
int main(void){
 wf_video_plan_init(&plan);
 bg[1]=0xc3;bg[2]=0x0a;bg[3]=0xbc;
 assert(wf_video_plan_map(&plan,1,bg,upload,0));
 assert((plan.maps[1][0]&0xc0000000u)==0xc0000000u);assert(((plan.maps[1][0]>>16)&0x7f)==35);
 unsigned count=uploads;assert(wf_video_plan_map(&plan,1,bg,upload,0)&&uploads==count);
 assert((plan.maps[1][0]&0x7fff)>=4096);
 text[0]=0; text[1]=0x34;text[3]=0xa2;
 wf_video_plan_text(&plan,text);assert(plan.text[0]==0x000a2234u);assert(plan.text[2048]==plan.text[0]);
 pal[0x80+30]=0x0a;pal[0x80+31]=0x5f;
 wf_video_plan_palette(plan.palette,pal);assert(plan.palette[31]==0x00aa55ffu);
 spr[1]=240;spr[3]=0x39;spr[5]=2;spr[9]=7;spr[11]=8;
 wf_video_piece_t pieces[4];unsigned n=0;
 assert(wf_video_plan_sprites(spr,1,pieces,4,&n));assert(n==1); /* flipY chain: first cell above screen */
 assert(pieces[0].code==3&&pieces[0].x==8&&pieces[0].y==-8&&pieces[0].bank==7&&pieces[0].flip==3);
 assert(!wf_video_plan_sprites(spr,1,pieces,0,&n));
 for(unsigned pass=0;pass<3;pass++){
  for(unsigned i=0;i<1024;i++){unsigned code=(pass==1?1024:0)+i;bg[i*4]=0;bg[i*4+1]=0;bg[i*4+2]=(uint8_t)(code>>8);bg[i*4+3]=(uint8_t)code;}
  assert(wf_video_plan_map(&plan,1,bg,upload,0));
  for(unsigned i=0;i<1024;i++){unsigned slot=((plan.maps[1][i]&0x7fff)-4096)/4;assert(loaded[1][slot]==(pass==1?1024:0)+i);}
 }
 puts("video plan: bank identity, flips, cache reuse, RGB444 and sprite clipping PASS");
}
