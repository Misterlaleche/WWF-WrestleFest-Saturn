#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "wf_r18_hud.h"
#include "wf_saturn_memory.h"
static wf_r18_video_t video;
static uint16_t names[4096];
static wf_runtime_t rt;
int main(void){
 rt.g.gameplay_vblank_enable=128;rt.g.frame_handshake[1]=1;
 rt.g.global_169e=0x2000;rt.g.global_16a4=1;
 assert(wf_r18_hud_irq(&rt,&video)==WF_OK);
 assert(video.palettes[0x71d]==15&&video.palettes[0x71f]==3);
 rt.g.global_16a4=0;assert(wf_r18_hud_irq(&rt,&video)==WF_OK);
 assert(video.palettes[0x71d]==255&&video.palettes[0x71f]==15);
 size_t writes=0;wf_r18_hud_reset(&video);
 assert(wf_r18_hud_map_update(&video,names,&writes)==WF_OK&&writes==4096);
 for(unsigned i=0;i<4096;i++)assert(names[i]==0);
 wf_r17_fg0_write_t w[]={{0xc0001,0xab,1},{0xc0003,0xcd,1},{0xc1fff,0xf2,1},{0xc2001,0xff,1}};
 assert(wf_r18_video_batch(NULL,w,4,&video)==WF_OK);
 assert(wf_r18_hud_map_update(&video,names,&writes)==WF_OK&&writes==4);
 assert(names[0]==0xcdab&&names[2048]==0xcdab&&names[2047]==0xf200&&names[4095]==0xf200);
 assert(wf_r18_hud_map_update(&video,names,&writes)==WF_OK&&writes==0);
 assert(wf_r18_video_batch(NULL,w,4,&video)==WF_OK);
 assert(wf_r18_hud_map_update(&video,names,&writes)==WF_OK&&writes==0);
 /* Invalid batches must not change a cell or consume pending dirtiness. */
 wf_r17_fg0_write_t bad[]={{0xc0001,0,1},{0xdeadbeef,0,1}};
 assert(wf_r18_video_batch(NULL,bad,2,&video)==WF_BLOCKED);
 assert(wf_r18_hud_map_update(&video,names,&writes)==WF_OK&&writes==0&&names[0]==0xcdab);
 /* Exercise every tile/palette word: a lost upper glyph bit or bank bit
  * changes the visible character/color, not merely internal bookkeeping. */
 static wf_r17_fg0_write_t all[4096];
 for(unsigned round=0;round<32;round++){
  for(unsigned cell=0;cell<2048;cell++){
   unsigned name=round*2048u+cell;
   all[cell*2]=(wf_r17_fg0_write_t){0xc0001u+cell*4u,name&255u,1};
   all[cell*2+1]=(wf_r17_fg0_write_t){0xc0003u+cell*4u,name>>8,1};
  }
  assert(wf_r18_video_batch(NULL,all,4096,&video)==WF_OK);
  assert(wf_r18_hud_map_update(&video,names,&writes)==WF_OK);
  for(unsigned cell=0;cell<2048;cell++)assert(names[cell]==round*2048u+cell&&names[cell+2048]==names[cell]);
 }
 assert(wf_saturn_memory_contract_validate()==WF_OK);
 puts("PASS native HUD: 64x32 arcade byte cells, 4096 glyph/palette combinations, dirty-only writes, independent palettes and sprite RAM exclusion");
 return 0;
}
