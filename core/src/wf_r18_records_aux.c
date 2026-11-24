#include "wf_r18_records_aux.h"
#include <string.h>

#define TRY(call) do { wf_status_t status=(call); if(status!=WF_OK)return status; } while(0)
static uint16_t d(wf_r18_records_aux_context_t *c,unsigned n) { return (uint16_t)c->machine->regs->d[n]; }
static void dw(wf_r18_records_aux_context_t *c,unsigned n,uint16_t v) { wf_r17_m68k_move_w(c->machine->regs,n,v); }
static void db(wf_r18_records_aux_context_t *c,unsigned n,uint8_t v) { wf_r17_m68k_move_b(c->machine->regs,n,v); }
static void q(wf_r18_records_aux_context_t *c,unsigned n,int8_t v) { wf_r17_m68k_moveq(c->machine->regs,n,v); }
static uint32_t indexed(uint32_t a,uint16_t offset) { return a+(int16_t)offset; }
static int royal(wf_r18_records_aux_context_t *c) { return (c->machine->ram[0x161]&1u)!=0; }
static wf_status_t block(wf_r18_records_aux_context_t *c,uint32_t pc,uint32_t at) {
 c->machine->dependency=c->blocked_dependency=pc;c->blocked_address=at&0xffffffu;return WF_BLOCKED;
}
static wf_status_t read_rom(wf_r18_records_aux_context_t *c,uint32_t pc,uint32_t at,unsigned bytes,uint32_t *out) {
 uint8_t b[2];at&=0xffffffu;
 if(at>0x80000u||bytes>0x80000u-at||c->read_rom(c->user,at,b,bytes)!=WF_OK)return block(c,pc,at);
 *out=b[0];if(bytes==2u)*out=(*out<<8)|b[1];return WF_OK;
}
static wf_status_t store(wf_r18_records_aux_context_t *c,uint32_t pc,uint32_t at,uint32_t value,unsigned bytes) {
 at&=0xffffffu;if(c->store_bus(c->user,at,value,bytes)!=WF_OK)return block(c,pc,at);return WF_OK;
}
/* $77D8. The table guard follows ASL.W, so rejected identities retain D0. */
static wf_status_t portrait_77d8(wf_r18_records_aux_context_t *c) {
 uint32_t saved_d[4],saved_a[2],value;uint16_t identity=d(c,0);uint32_t *a=c->machine->a;
 memcpy(saved_d,c->machine->regs->d,sizeof saved_d);memcpy(saved_a,a,sizeof saved_a);
 a[1]=0x7814u;dw(c,0,(uint16_t)(d(c,0)<<1));
 if(identity>11u)return block(c,0x77e4u,indexed(a[1],d(c,0)));
 TRY(read_rom(c,0x77e4u,indexed(a[1],d(c,0)),2,&value));dw(c,0,(uint16_t)value);q(c,2,2);
 do {
  q(c,3,2);
  do {
   TRY(store(c,0x77ecu,a[0]+1u,(uint8_t)d(c,0),1));dw(c,1,d(c,0));dw(c,1,(uint16_t)(d(c,1)>>8));
   TRY(store(c,0x77f4u,a[0]+3u,(uint8_t)d(c,1),1));dw(c,0,(uint16_t)(d(c,0)+1u));a[0]+=4u;
   dw(c,3,(uint16_t)(d(c,3)-1u));
  } while(d(c,3)!=0xffffu);
  a[0]+=0xf4u;dw(c,2,(uint16_t)(d(c,2)-1u));
 } while(d(c,2)!=0xffffu);
 memcpy(c->machine->regs->d,saved_d,sizeof saved_d);memcpy(a,saved_a,sizeof saved_a);return WF_OK;
}
/* $A5BE deliberately leaves its D/A results live for its caller. */
static wf_status_t badge_a5be(wf_r18_records_aux_context_t *c) {
 uint32_t value,*a=c->machine->a;uint16_t identity=d(c,1);
 dw(c,4,d(c,1));dw(c,4,(uint16_t)(d(c,4)<<1));a[2]=0xa608u;q(c,3,4);
 if(d(c,1)>=10u)dw(c,3,10u);
 if(royal(c)) {
  a[2]=0xa620u;dw(c,4,(uint16_t)(d(c,4)<<1));
  if(identity>12u)return block(c,0xa5e6u,indexed(a[2],d(c,4))+2u);
  TRY(read_rom(c,0xa5e6u,indexed(a[2],d(c,4))+2u,2,&value));dw(c,3,(uint16_t)value);
 } else if(identity>12u)return block(c,0xa5eau,indexed(a[2],d(c,4)));
 TRY(read_rom(c,0xa5eau,indexed(a[2],d(c,4)),2,&value));dw(c,4,(uint16_t)value);
 do {
  TRY(store(c,0xa5eeu,a[1]+1u,(uint8_t)d(c,4),1));dw(c,5,d(c,4));dw(c,5,(uint16_t)(d(c,5)>>8));
  TRY(store(c,0xa5f6u,a[1]+3u,(uint8_t)d(c,5),1));a[1]+=4u;dw(c,4,(uint16_t)(d(c,4)+1u));
  dw(c,3,(uint16_t)(d(c,3)-1u));
 } while(d(c,3)!=0xffffu);
 return WF_OK;
}
static wf_status_t records_a4d4(wf_r18_records_aux_context_t *c) {
 uint32_t saved_d[8],saved_a[7],*a=c->machine->a;uint8_t *ram=c->machine->ram;
 memcpy(saved_d,c->machine->regs->d,sizeof saved_d);memcpy(saved_a,a,sizeof saved_a);q(c,7,0);
 do {
  a[0]=royal(c)?0x1c0104u:0x1c00c2u;dw(c,1,d(c,7));c->machine->regs->d[1]=(uint32_t)d(c,1)*6u;
  db(c,2,ram[a[0]-0x1c0000u+d(c,1)+5u]);db(c,1,ram[a[0]-0x1c0000u+d(c,1)+4u]);
  if(d(c,7)<3u) {
   a[1]=0xc0378u;q(c,3,1);if(royal(c))a[1]+=12u;dw(c,6,d(c,7));
   for(;;) { dw(c,6,(uint16_t)(d(c,6)-1u));if(d(c,6)&0x8000u)break;a[1]+=0x400u; }
   for(;;) {
    a[0]=a[1];dw(c,0,d(c,1));TRY(portrait_77d8(c));if(royal(c))break;
    a[1]+=16u;dw(c,1,d(c,2));dw(c,3,(uint16_t)(d(c,3)-1u));if(d(c,3)==0xffffu)break;
   }
  } else {
   a[1]=0xc0f64u;q(c,6,1);if(royal(c))a[1]+=12u;
   for(;;) {
    dw(c,3,d(c,7));dw(c,3,(uint16_t)(d(c,3)-3u));
    for(;;) { dw(c,3,(uint16_t)(d(c,3)-1u));if(d(c,3)&0x8000u)break;a[1]+=0x200u; }
    TRY(badge_a5be(c));
    if(royal(c)) {
     if(d(c,1)>=10u) { dw(c,1,12u);TRY(badge_a5be(c)); }
     break;
    }
    if(d(c,1)>=10u)break;
    dw(c,6,(uint16_t)(d(c,6)-1u));if(d(c,6)&0x8000u)break;
    TRY(store(c,0xa590u,a[1],0xffa2fffbu,4));a[1]=0xc0f7cu;dw(c,1,d(c,2));
   }
  }
  dw(c,7,(uint16_t)(d(c,7)+1u));
 } while(d(c,7)!=11u);
 memcpy(c->machine->regs->d,saved_d,sizeof saved_d);memcpy(a,saved_a,sizeof saved_a);return WF_OK;
}
static wf_status_t glyph_7940(wf_r18_records_aux_context_t *c) {
 TRY(store(c,0x7940u,c->machine->a[3]+1u,(uint8_t)d(c,4),1));dw(c,6,d(c,4));
 dw(c,6,(uint16_t)(d(c,6)>>8));dw(c,6,(uint16_t)(d(c,6)&15u));dw(c,6,(uint16_t)(d(c,6)|0xf0u));
 return store(c,0x7950u,c->machine->a[3]+3u,(uint8_t)d(c,6),1);
}
static wf_status_t frame_78cc(wf_r18_records_aux_context_t *c) {
 uint32_t value,*a=c->machine->a;
 a[1]=0x7956u;dw(c,1,d(c,0));c->machine->regs->d[1]=(uint32_t)d(c,1)*6u;
 TRY(read_rom(c,0x78d8u,indexed(a[1],d(c,1)),2,&value));dw(c,2,(uint16_t)value);
 c->machine->regs->d[2]&=0xffffu;a[2]=c->machine->regs->d[2];a[2]+=0xc0000u;
 TRY(read_rom(c,0x78eau,indexed(a[1],d(c,1))+2u,2,&value));dw(c,2,(uint16_t)value);
 TRY(read_rom(c,0x78eeu,indexed(a[1],d(c,1))+4u,2,&value));dw(c,3,(uint16_t)value);
 do {
  a[3]=a[2];a[1]=0x79bcu;dw(c,1,d(c,0));dw(c,1,(uint16_t)(d(c,1)&255u));dw(c,1,(uint16_t)(d(c,1)<<2));
  TRY(read_rom(c,0x7902u,indexed(a[1],d(c,1)),2,&value));dw(c,4,(uint16_t)value);q(c,5,0);
  TRY(read_rom(c,0x7908u,indexed(a[1],d(c,1))+2u,1,&value));db(c,5,(uint8_t)value);
  TRY(read_rom(c,0x790cu,indexed(a[1],d(c,1))+3u,1,&value));db(c,1,(uint8_t)value);
  do {
   TRY(glyph_7940(c));
   if(d(c,4)&0x8000u) {
    uint32_t saved_d4=c->machine->regs->d[4],saved_a3=a[3];a[3]+=0x100u;dw(c,4,(uint16_t)(d(c,4)+d(c,5)));
    TRY(glyph_7940(c));c->machine->regs->d[4]=saved_d4;a[3]=saved_a3;
   }
   a[3]+=4u;dw(c,4,(uint16_t)(d(c,4)+1u));dw(c,1,(uint16_t)(d(c,1)-1u));
  } while(d(c,1)!=0xffffu);
  a[2]+=c->machine->regs->d[2];dw(c,3,(uint16_t)(d(c,3)-1u));
 } while(d(c,3)!=0xffffu);
 return WF_OK;
}
static wf_status_t frames_7874(wf_r18_records_aux_context_t *c) {
 uint32_t saved_d[7],saved_a[4],*a=c->machine->a;
 memcpy(saved_d,c->machine->regs->d,sizeof saved_d);memcpy(saved_a,a,sizeof saved_a);q(c,0,0);
 for(;;) {
  TRY(frame_78cc(c));dw(c,0,(uint16_t)(d(c,0)+1u));
  if(royal(c)) { if(d(c,0)>=17u)break;if(d(c,0)==10u)dw(c,0,12u); }
  else if(d(c,0)>=12u)break;
 }
 a[3]=0xc0f34u;dw(c,4,0x2dcu);dw(c,0,7u);
 do {
  TRY(glyph_7940(c));a[3]+=0x200u;dw(c,4,(uint16_t)(d(c,4)+1u));dw(c,0,(uint16_t)(d(c,0)-1u));
 } while(d(c,0)!=0xffffu);
 memcpy(c->machine->regs->d,saved_d,sizeof saved_d);memcpy(a,saved_a,sizeof saved_a);return WF_OK;
}
wf_status_t wf_r18_records_aux_call(wf_r18_records_aux_context_t *c,uint32_t owner) {
 if(!c||!c->machine||!c->machine->ram||!c->machine->regs||!c->machine->regs->valid||!c->read_rom||!c->store_bus)return WF_BAD_STATE;
 c->blocked_dependency=c->blocked_address=c->machine->dependency=0;
 if(owner==0xa4d4u)return records_a4d4(c);
 if(owner==0x7874u)return frames_7874(c);
 return block(c,owner,0);
}
