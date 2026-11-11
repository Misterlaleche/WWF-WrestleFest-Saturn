#include "wf_failure_screen.h"
#include "wf_cd_wait_diagnostics.h"
#ifdef WF_COMBAT_TAG_STARTUP_STREAM
#include "wf_sdk_assert.h"
#endif
#include <stddef.h>
/* This code only runs after a terminal failure. Keep scarce HWRAM for the
 * unchanged gameplay/cache path, rather than optimizing error text for speed. */
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC optimize ("Oz")
#endif
#ifndef WF_COMBAT_MODULE
static const char labels[]="ERROR\0VIDEO\0TILE\0ATLAS\0CD STAGE\0CD ERROR\0IO STAGE\0IO ERROR\0FAD\0COMMAND\0RESPONSE\0READ BYTES\0COPIED\0SESSION\0PHASE\0PC\0OWNER\0ADDRESS\0COMBAT\0COMBAT ERR\0DEPEND\0CART ID\0CART BYTES\0CART VALID";
#endif
static const char combat_labels[]="ERROR\0VDP ERR\0TILE\0TILE ERR\0CD STG\0CD ERR\0IO STG\0IO ERR\0FAD\0COMMAND\0RESPONSE\0BYTES\0COPIED\0ACK\0FRAMEPH\0PC\0OWNER\0ADDRESS\0COMBAT\0COMBAT ERR\0DEPEND\0CART\0RAM\0VALID";
static const char hex[]="0123456789ABCDEF";

/* Optional observer view. Existing bitmap APIs and non-CD failures retain
 * their layout. Full counters remain in the reader diagnostic global. */
const volatile wf_cd_wait_diagnostics_t *wf_failure_cd_wait_state;
static const char *wait_row(const uint32_t *codes,unsigned row,uint32_t *value){
 const volatile wf_cd_wait_diagnostics_t *d=wf_failure_cd_wait_state;
 if(!d || d->version!=1 || codes[WF_FAILURE_IO_STAGE]!=5 ||
    !codes[WF_FAILURE_IO_ERROR] || !d->result)return 0;
 switch(row){
 case WF_FAILURE_PHASE:*value=d->reason;return "WHY";
 case WF_FAILURE_OWNER:*value=(d->last_hirq<<16)|(d->seen_hirq&0xffffu);return "HIRQ";
 case WF_FAILURE_ADDRESS:
  *value=(d->requery_attempt<<24)|(d->requery_recovered<<16)|((uint32_t)d->requery_result&0xffffu);
  return "RECHECK";
 default:return 0;
 }
}


static const uint8_t glyphs[36][5]={
  {0x3e,0x51,0x49,0x45,0x3e},{0,0x42,0x7f,0x40,0},{0x42,0x61,0x51,0x49,0x46},
  {0x21,0x41,0x45,0x4b,0x31},{0x18,0x14,0x12,0x7f,0x10},{0x27,0x45,0x45,0x45,0x39},
  {0x3c,0x4a,0x49,0x49,0x30},{1,0x71,9,5,3},{0x36,0x49,0x49,0x49,0x36},{6,0x49,0x49,0x29,0x1e},
  {0x7e,0x11,0x11,0x11,0x7e},{0x7f,0x49,0x49,0x49,0x36},{0x3e,0x41,0x41,0x41,0x22},
  {0x7f,0x41,0x41,0x22,0x1c},{0x7f,0x49,0x49,0x49,0x41},{0x7f,9,9,9,1},
  {0x3e,0x41,0x49,0x49,0x7a},{0x7f,8,8,8,0x7f},{0,0x41,0x7f,0x41,0},
  {0x20,0x40,0x41,0x3f,1},{0x7f,8,0x14,0x22,0x41},{0x7f,0x40,0x40,0x40,0x40},
  {0x7f,2,0x0c,2,0x7f},{0x7f,4,8,0x10,0x7f},{0x3e,0x41,0x41,0x41,0x3e},
  {0x7f,9,9,9,6},{0x3e,0x41,0x51,0x21,0x5e},{0x7f,9,0x19,0x29,0x46},
  {0x46,0x49,0x49,0x49,0x31},{1,1,0x7f,1,1},{0x3f,0x40,0x40,0x40,0x3f},
  {0x1f,0x20,0x40,0x20,0x1f},{0x3f,0x40,0x38,0x40,0x3f},{0x63,0x14,8,0x14,0x63},
  {7,8,0x70,8,7},{0x61,0x51,0x49,0x45,0x43}};

uint8_t wf_failure_glyph_column(char c,unsigned column){
 unsigned index=c>='0'&&c<='9'?(unsigned)(c-'0'):c>='A'&&c<='Z'?(unsigned)(c-'A'+10):36u;
 return index<36u&&column<5u?glyphs[index][column]:0;
}
static void character_width(volatile uint16_t *bitmap,unsigned x,unsigned y,char c,unsigned width){
 unsigned index=c>='0'&&c<='9'?(unsigned)(c-'0'):c>='A'&&c<='Z'?(unsigned)(c-'A'+10):36u;
 for(unsigned column=0;column<width;column+=2){
  unsigned first=index<36&&column<5?glyphs[index][column]:0;
  unsigned second=index<36&&column+1<5?glyphs[index][column+1]:0;
  for(unsigned row=0,pair=(first<<8)|second;row<8;row++,pair>>=1)
   bitmap[(y+row)*(WF_FAILURE_WIDTH/2u)+(x+column)/2u]=(uint16_t)(pair&0x0101u);
 }
}
#ifndef WF_COMBAT_MODULE
static void character(volatile uint16_t *bitmap,unsigned x,unsigned y,char c){character_width(bitmap,x,y,c,8);}
static void text(volatile uint16_t *bitmap,unsigned x,unsigned y,const char *label){
 while(*label){character(bitmap,x,y,*label++);x+=8u;}
}
void wf_failure_bitmap_draw(volatile uint16_t *bitmap,const uint32_t codes[WF_FAILURE_CODE_COUNT]){
 for(unsigned i=0;i<WF_FAILURE_WIDTH*WF_FAILURE_HEIGHT/2u;i++)bitmap[i]=0;
 text(bitmap,24,12,"WWF SATURN ERROR");text(bitmap,248,12,"HEX");
 const char *label=labels;
 for(unsigned line=0;line<WF_FAILURE_CODE_COUNT;line++){
  uint32_t value=codes[line];const char *wait=wait_row(codes,line,&value);
  unsigned y=32u+line*8u;text(bitmap,24,y,wait?wait:label);
  while(*label++){}
  for(unsigned digit=0;digit<8;digit++)character(bitmap,160u+digit*8u,y,hex[(value>>(28u-digit*4u))&15u]);
 }
}

#endif
static void compact_text(volatile uint16_t *bitmap,unsigned x,unsigned y,const char *s){
 while(*s){character_width(bitmap,x,y,*s++,6);x+=6;}
}
static void compact_hex(volatile uint16_t *bitmap,unsigned x,unsigned y,uint32_t value,unsigned digits){
 for(unsigned i=0;i<digits;i++)character_width(bitmap,x+i*6,y,hex[(value>>((digits-i-1)*4))&15u],6);
}
const char *wf_failure_actor_name(unsigned id){
 /* RAW identities, not selector positions: the recovered TAG R3 table in
  * wf_r18_fault_pages.h:wf_fault_name is the authority for these labels. */
 static const char names[12][11]={"HOGAN","WARRIOR","SNAKE","BOSSMAN","HAWK","ANIMAL",
  "SLAUGHTER","PERFECT","EARTHQUAKE","DIBIASE","SMASH","CRUSH"};
 return id<12?names[id]:"UNKNOWN";
}
void wf_failure_combat_bitmap_draw(volatile uint16_t *bitmap,const uint32_t codes[WF_FAILURE_CODE_COUNT],const wf_failure_combat_t *c){
 for(unsigned i=0;i<WF_FAILURE_WIDTH*WF_FAILURE_HEIGHT/2u;i++)bitmap[i]=0;
 compact_text(bitmap,8,8,"WWF ERROR R03");compact_text(bitmap,236,8,!c->valid?"SETUP":c->royal?"ROYAL":"TAG");
 const char *label=combat_labels;
 uint32_t values[WF_FAILURE_CODE_COUNT];
 for(unsigned i=0;i<WF_FAILURE_CODE_COUNT;i++)values[i]=codes[i];
 values[WF_FAILURE_VIDEO]=c->render_error;values[WF_FAILURE_TILE]=c->secondary_tile;
 values[WF_FAILURE_ATLAS]=c->secondary_error;values[WF_FAILURE_SESSION]=c->ack;
 values[WF_FAILURE_PHASE]=c->frame_phase;values[WF_FAILURE_ADDRESS]=c->rom_address;
 for(unsigned i=0;i<WF_FAILURE_CODE_COUNT;i++){
  unsigned x=8+(i/12)*152,y=24+(i%12)*8;
  const char *wait=wait_row(codes,i,&values[i]);
  compact_text(bitmap,x,y,wait?wait:label);compact_hex(bitmap,x+86,y,values[i],8);
  while(*label++){}
 }
 const uint32_t counters[]={c->frame,c->rng,c->scene,c->site};
 static const uint8_t counter_x[]={8,98,176,224},counter_value_x[]={44,122,200,254};
 const char *counter_label="FRAME\0RNG\0MAP\0SITE";
 for(unsigned i=0;i<4;i++){
  compact_text(bitmap,counter_x[i],128,counter_label);
  compact_hex(bitmap,counter_value_x[i],128,counters[i],i<2?8:2);
  while(*counter_label++){}
 }
 compact_text(bitmap,8,144,"S ID   FIGHTER    A  STAT ANIM X    Y    HP   P");
 for(unsigned i=0;i<9;i++){
  const wf_failure_actor_t *a=c->actors+i;unsigned y=152+i*8;
  static const uint16_t positions[]={20,134,164,194,224,254,116,284};
  static const uint8_t offsets[]={offsetof(wf_failure_actor_t,id),offsetof(wf_failure_actor_t,state),
   offsetof(wf_failure_actor_t,animation),offsetof(wf_failure_actor_t,x),offsetof(wf_failure_actor_t,y),
   offsetof(wf_failure_actor_t,stamina),offsetof(wf_failure_actor_t,active),offsetof(wf_failure_actor_t,action)};
  for(unsigned field=0;field<8;field++){
   const uint8_t *p=(const uint8_t *)a+offsets[field];
   unsigned value=field<6?*(const uint16_t *)p:*p;
   compact_hex(bitmap,positions[field],y,value,field<6?4:2);
  }
  compact_hex(bitmap,8,y,i,1);
  compact_text(bitmap,50,y,a->active?wf_failure_actor_name(a->id):"INACTIVE");
 }
 compact_text(bitmap,8,224,"HEX");compact_text(bitmap,56,224,"SLOT");compact_hex(bitmap,116,224,c->bad_slot,2);
#ifdef WF_COMBAT_TAG_STARTUP_STREAM
#ifdef WF_COMBAT_TAG_EXCEPTION_SHIM
 if(wf_sdk_exception_record.code){
  compact_text(bitmap,8,232,"CPU");compact_text(bitmap,38,232,"PC");
  compact_hex(bitmap,56,232,wf_sdk_exception_record.registers.pc,8);
  compact_text(bitmap,116,232,"SR");compact_hex(bitmap,134,232,wf_sdk_exception_record.registers.sr,8);
  compact_text(bitmap,194,232,"MSG");compact_hex(bitmap,218,232,(uint32_t)(uintptr_t)wf_sdk_exception_record.message,8);
 }else
#endif
 if(wf_sdk_assert_record.code){
  compact_text(bitmap,8,232,"ASSERT");compact_text(bitmap,56,232,"LINE");
  compact_hex(bitmap,86,232,wf_sdk_assert_record.line_number,8);
  compact_text(bitmap,146,232,"FILE");compact_hex(bitmap,176,232,(uint32_t)(uintptr_t)wf_sdk_assert_record.file,8);
  compact_text(bitmap,236,232,"FUNC");compact_hex(bitmap,266,232,(uint32_t)(uintptr_t)wf_sdk_assert_record.function,8);
 }
#endif

}

/* Fixed terminal screen, independent of SDK display state. Register offsets
 * are from Sega VDP2 manual ST-058-R2, register map pp.295-308. All stores
 * to the actual device are halfwords; this image also resets windows/offsets. */
void wf_failure_register_image(uint16_t *r){
 for(unsigned i=0;i<144;i++)r[i]=0;
 r[0]=0x8010;r[0x0e/2]=0x1000;
 for(unsigned i=0x10/2;i<=0x1e/2;i++)r[i]=0xffff;
 r[0x10/2]=0x4444;r[0x20/2]=0x0101;r[0x28/2]=0x0012;
 r[0x78/2]=r[0x7c/2]=1;r[0xf8/2]=7;
}

#ifndef WF_FAILURE_HOST_TEST
#include <yaul.h>
#include "wf_cd.h"
#include "wp1979_cd_read.h"
#define OPTIONAL(name) extern volatile uint32_t name __attribute__((weak))
OPTIONAL(wf_video_error);OPTIONAL(wf_video_tile);OPTIONAL(wf_video_atlas);
OPTIONAL(wf_combat_stage);OPTIONAL(wf_combat_error);OPTIONAL(wf_combat_dependency);
OPTIONAL(wf_cart_id);OPTIONAL(wf_cart_bytes);OPTIONAL(wf_cart_valid);
extern volatile wf_cd_wait_diagnostics_t wf_cd_wait_diagnostics __attribute__((weak));
extern void wf_failure_context_read(wf_failure_context_t *) __attribute__((weak));
extern void wf_failure_combat_read(wf_failure_combat_t *) __attribute__((weak));
const wf_failure_combat_t *wf_failure_combat_state;
const uint32_t *wf_failure_codes_ptr;
volatile uint32_t wf_failure_shown;
static uint32_t optional(const volatile uint32_t *value){return value?*value:UINT32_MAX;}

void wf_failure_screen_show(uint32_t error){
 /* Terminal takeover: the failed renderer may never acknowledge its queue,
  * and callers may already have IRQs masked. Never wait on SDK sync flags. */
 cpu_intc_mask_set(15);scu_ic_mask_set(SCU_IC_MASK_ALL);
 wf_failure_context_t context={UINT32_MAX,UINT32_MAX,UINT32_MAX,UINT32_MAX,UINT32_MAX};
 if(wf_failure_context_read)wf_failure_context_read(&context);
 wf_failure_combat_t combat_snapshot={0};
 wf_failure_combat_state=&combat_snapshot;
 if(wf_failure_combat_read)wf_failure_combat_read(&combat_snapshot);
 uint32_t codes[WF_FAILURE_CODE_COUNT]={
  error,optional(&wf_video_error),optional(&wf_video_tile),optional(&wf_video_atlas),
  wf_cd_stage,wf_cd_error,wp1979_cd_diagnostics.stage,(uint32_t)wp1979_cd_diagnostics.error,
  wp1979_cd_diagnostics.fad,
  (uint32_t)wp1979_cd_diagnostics.command[0]<<16|wp1979_cd_diagnostics.command[1],
  (uint32_t)wp1979_cd_diagnostics.response[0]<<16|wp1979_cd_diagnostics.response[1],
  wp1979_cd_diagnostics.requested_bytes,wp1979_cd_diagnostics.copied_bytes,
  context.session_phase,context.phase,context.pc,context.owner,context.address,
  optional(&wf_combat_stage),optional(&wf_combat_error),optional(&wf_combat_dependency),
  optional(&wf_cart_id),optional(&wf_cart_bytes),optional(&wf_cart_valid)};
 wf_failure_cd_wait_state=&wf_cd_wait_diagnostics;
 wf_failure_codes_ptr=codes;
 volatile vdp2_ioregs_t *hardware=(volatile vdp2_ioregs_t *)VDP2_IOREG_BASE;
 hardware->tvmd&=(uint16_t)~0x8000u;
 /* Poll hardware, not an IRQ-completed SDK queue. ST-097-R5 p45 forbids
  * writing DnEN during active DMA; DSTP is prohibited by ST-208 errata.
  * A truly stuck DMA cannot safely hand its destination memory to the CPU. */
 if(!wf_failure_dma_idle((const volatile uint32_t *)(SCU_IOREG_BASE+DSTA),0x100000u)){
  wf_failure_shown=2;for(;;)__asm__ volatile("nop");
 }
 scu_dma_stop();cpu_dmac_stop();
 volatile vdp1_ioregs_t *vdp1=(volatile vdp1_ioregs_t *)VDP1_IOREG_BASE;
 vdp1->ptmr=0;vdp1->endr=1;
 /* Reset the register image, including windows, color offsets and scaling
  * inherited from combat. Commit the fixed bitmap mode by CPU words. */
 vdp2_ioregs_t *registers=vdp2_regs_get();
 wf_failure_register_image(registers->buffer);
 /* Commit CRAM interpretation before palette stores. Combat may have left
  * another color RAM mode selected; changing it afterwards remaps those words. */
 for(unsigned i=7;i<144;i++)hardware->buffer[i]=registers->buffer[i];
 hardware->exten=0;
 volatile uint16_t *palette=(volatile uint16_t *)VDP2_CRAM_MODE_1_OFFSET(0,0,0);
 palette[0]=RGB1555(1,10,0,0).raw;palette[1]=RGB1555(1,31,31,31).raw;
#ifdef WF_COMBAT_MODULE
 wf_failure_combat_bitmap_draw((volatile uint16_t *)VDP2_VRAM_ADDR(0,0),codes,&combat_snapshot);
#else
 wf_failure_bitmap_draw((volatile uint16_t *)VDP2_VRAM_ADDR(0,0),codes);
#endif
 /* Status/counters0x04..0x0C were never copied. Enable only after pixels
  * and colors are ready; all device stores above are aligned halfwords. */
 hardware->tvmd=registers->tvmd;
 wf_failure_shown=1;
 /* Terminal failure: no game callback or delayed register copy can replace
  * the diagnostic after this point. No interrupt-dependent waits follow. */
 for(;;)__asm__ volatile("nop");
}
#endif
