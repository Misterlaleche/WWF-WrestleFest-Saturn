#include "wf_startup_notice.h"
#include "wf_failure_screen.h"
#include <stddef.h>
const char *wf_startup_notice_message(uint32_t id,uint32_t bytes){
 if(id==0x5cu&&bytes==0x400000u)return "4MB RAM CARTRIDGE DETECTED";
 if(id==0x5au&&bytes==0x100000u)return "1MB RAM CARTRIDGE DETECTED";
 return NULL;
}
void wf_startup_notice_draw(volatile uint16_t *bitmap,const char *message){
 for(unsigned i=0;i<512u*256u/2u;i++)bitmap[i]=0;
 if(!message)return;
 unsigned count=0;while(count<40&&message[count])count++;
 /* Even X keeps every device store a halfword. Text stays in 320x240. */
 unsigned left=((320u-count*8u)/2u)&~1u;
 for(unsigned c=0;c<count;c++)for(unsigned column=0;column<8;column+=2){
  unsigned first=wf_failure_glyph_column(message[c],column);
  unsigned second=wf_failure_glyph_column(message[c],column+1);
  for(unsigned y=0;y<8;y++)
   bitmap[(116u+y)*256u+(left+c*8u+column)/2u]=
    (uint16_t)((((first>>y)&1u)<<8)|((second>>y)&1u));
 }
}

#ifndef WF_STARTUP_NOTICE_HOST_TEST
#include <yaul.h>
#include "wf_platform.h"
static vdp2_ioregs_t saved_registers;
static uint32_t saved_palette[2],start_field;
volatile uint32_t wf_startup_notice_active,wf_startup_notice_fields;
bool wf_startup_notice_begin(uint32_t id,uint32_t bytes){
 const char *message=wf_startup_notice_message(id,bytes);
 if(!message)return false;
 /* Only the frontend calls this, before any arcade frame is submitted.
  * Bank0 holds the temporary bitmap; FONT.TEX loads into bank2. Save the
  * configured frontend image and keep its mode2 CRAM interpretation. */
 vdp2_sync_wait();
 vdp2_ioregs_t *r=vdp2_regs_get();saved_registers=*r;
 volatile uint32_t *palette=(volatile uint32_t *)VDP2_CRAM_MODE_2_OFFSET(0,0,0);
 saved_palette[0]=palette[0];saved_palette[1]=palette[1];
 for(unsigned i=0;i<144;i++)r->buffer[i]=0;
 r->tvmd=0x0010;r->ramctl=0x2000;
 for(unsigned i=0x10/2;i<=0x1e/2;i++)r->buffer[i]=0xffff;
 r->buffer[0x10/2]=0x4444;r->bgon=0x0101;r->chctla=0x0012;
 r->buffer[0x78/2]=r->buffer[0x7c/2]=1;r->buffer[0xf8/2]=7;
 vdp2_sync();vdp2_sync_wait();
 palette[0]=RGB888(0,0,0,0).raw;palette[1]=RGB888(0,255,255,255).raw;
 wf_startup_notice_draw((volatile uint16_t *)VDP2_VRAM_ADDR(0,0),message);
 vdp2_tvmd_display_set();vdp2_sync();vdp2_sync_wait();
 start_field=wf_fields;wf_startup_notice_active=bytes>>20;
 return true;
}
void wf_startup_notice_end(void){
 if(!wf_startup_notice_active)return;
 vdp2_tvmd_display_clear();vdp2_sync();vdp2_sync_wait();
 volatile uint32_t *palette=(volatile uint32_t *)VDP2_CRAM_MODE_2_OFFSET(0,0,0);
 palette[0]=saved_palette[0];palette[1]=saved_palette[1];
 *vdp2_regs_get()=saved_registers;
 vdp2_sync();vdp2_sync_wait();
 wf_startup_notice_fields=wf_fields-start_field;wf_startup_notice_active=0;
}
#endif
