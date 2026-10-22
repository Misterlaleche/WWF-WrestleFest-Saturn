#include <stddef.h>
#include <stdint.h>
#include "wf_r17_referee.h"
#include "wf_r17_contract.h"
#include "wf_physics.h"

#define REF_PTR 0x001C11F4u
#define BIT8(n) ((uint8_t)(1u<<(n)))

enum {
 R_VISUAL=0x04,R_X=0x06,R_Y=0x0A,R_Z=0x0E,R_LIST=0x12,
 R_LATCH=0x1C,R_STATE=0x20,R_TICK=0x22,R_PHASE=0x24,R_SPEED=0x2A,
 R_ANGLE=0x2C,R_FACING=0x2E,R_TARGET_X=0x30,R_FLAGS34=0x34,R_PIN=0x35,
 R_CONTACT=0x36,R_CORR_X=0x38,R_CORR_Y=0x3A,R_LINK=0x56,R_TARGET2_X=0x60,R_TARGET2_Y=0x62,
 R_SOUND=0x54
};

const uint32_t wf_r17_ref_primary_table_1f952[11]={
 0x0001F97Eu,0x0001F9E0u,0x0001F9F8u,0x0001FA82u,0x0001FA40u,0x0001F99Eu,
 0x0001F9D8u,0x0001F9F8u,0x0001FA40u,0x0001FAECu,0x0001FAECu
};
const uint32_t wf_r17_ref_action_table_1fb1a[11]={
 0x0001FB46u,0x0001FC22u,0x0001FC22u,0x0001FD5Eu,0x0001FC22u,0x0001FF52u,
 0x00020022u,0x000202BAu,0x00020354u,0x000203EEu,0x0001FF52u
};

static int bit(const wf_actor_raw_t*a,size_t o,unsigned n){return (wf_u8(a,o)&BIT8(n))!=0u;}
static void setbit(wf_actor_raw_t*a,size_t o,unsigned n){wf_set_u8(a,o,(uint8_t)(wf_u8(a,o)|BIT8(n)));}
static void clrbit(wf_actor_raw_t*a,size_t o,unsigned n){wf_set_u8(a,o,(uint8_t)(wf_u8(a,o)&(uint8_t)~BIT8(n)));}
static void chgbit(wf_actor_raw_t*a,size_t o,unsigned n){wf_set_u8(a,o,(uint8_t)(wf_u8(a,o)^BIT8(n)));}
static uint8_t low(const wf_actor_raw_t*a,size_t o){return wf_u8(a,o+1u);}
static uint8_t byte_dec(wf_actor_raw_t*a,size_t o){uint8_t v=(uint8_t)(wf_u8(a,o)-1u);wf_set_u8(a,o,v);return v;}
static uint8_t byte_inc(wf_actor_raw_t*a,size_t o){uint8_t v=(uint8_t)(wf_u8(a,o)+1u);wf_set_u8(a,o,v);return v;}
static wf_status_t block(wf_r17_referee_ctx_t*c,uint32_t pc){if(c)c->blocked_dependency_rom=pc;return WF_BLOCKED;}
static wf_actor_raw_t *aptr(wf_runtime_t*rt,uint32_t p){return wf_actor_from_rom_ptr(rt,p,0);}
static uint32_t actor_ptr(const wf_runtime_t*rt,const wf_actor_raw_t*a){ptrdiff_t d;if(!rt||!a||a<rt->actors||a>=rt->actors+WF_ACTOR_SLOTS)return 0u;d=a-rt->actors;return wf_actor_rom_base((unsigned)d);}
static wf_status_t linked(wf_runtime_t*rt,wf_actor_raw_t*r,wf_actor_raw_t**out,wf_r17_referee_ctx_t*c,uint32_t pc){wf_actor_raw_t*a=aptr(rt,wf_be32(r,R_LINK));if(!a)return block(c,pc);*out=a;return WF_OK;}
static int entry_latch(wf_actor_raw_t*r){int was=bit(r,R_LATCH,7);setbit(r,R_LATCH,7);return !was;}
static void set_global_169a(wf_runtime_t*rt,uint16_t v){rt->g.global_169a=v;rt->g.global_169b=(uint8_t)v;}
/* $20082 is ADDQ.B at $1C169A: the high byte of the word, not $169B. */
static void inc_global_169a(wf_runtime_t*rt){set_global_169a(rt,(uint16_t)(rt->g.global_169a+0x100u));}

void wf_r17_referee_init_10718(wf_runtime_t *rt){
 wf_actor_raw_t*r;if(!rt)return;r=&rt->referee;
 wf_set_be16(r,0x00u,0x8000u);wf_set_be16(r,0x02u,0x000Cu);wf_set_be16(r,R_X,0x0280u);
 wf_set_be16(r,R_Y,0x0198u);wf_set_be16(r,R_Z,0x0140u);wf_set_be16(r,R_STATE,0x8000u);
 wf_set_be16(r,R_FLAGS34,0u);rt->g.global_1697=1u;
}
void wf_r17_raw48_pin_trigger_133a2(wf_actor_raw_t*w){if(w)setbit(w,WF_A_PINFLAGS,0);}

/* $204FA.  The ROM stores the matching actor pointers at referee +$4C/+50. */
static wf_status_t gather_204fa(wf_runtime_t*rt,wf_actor_raw_t*r,wf_actor_raw_t**a2,wf_actor_raw_t**a3,wf_r17_referee_ctx_t*c){
 unsigned i,n=0;uint32_t p[2]={0,0};
 wf_r17_m68k_moveq(&rt->m68k_regs,0u,8);
 wf_r17_m68k_moveq(&rt->m68k_regs,1u,0);
 for(i=0;i<WF_ACTOR_SLOTS;i++){
  wf_actor_raw_t*a=&rt->actors[i];if(!bit(a,WF_A_ACTIVE,7)||!bit(a,WF_A_FLAGS33,0))continue;
  wf_r17_m68k_move_w(&rt->m68k_regs,0u,0xffffu); /* $20520 DBRA terminal count. */
 if(n<2u){p[n]=wf_actor_rom_base(i);}
  n++;
  wf_r17_m68k_move_l(&rt->m68k_regs,1u,(uint32_t)n*4u);
 }
 wf_r17_m68k_move_w(&rt->m68k_regs,0u,0xffffu); /* $20520 DBRA terminal count. */
 if(n<2u)return block(c,0x000204FAu);
 wf_set_be32(r,0x4Cu,p[0]);wf_set_be32(r,0x50u,p[1]);*a2=aptr(rt,p[0]);*a3=aptr(rt,p[1]);
 return (*a2&&*a3)?WF_OK:block(c,0x000204FAu);
}
/* $204B2 target midpoint. */
static wf_status_t target_204b2(wf_runtime_t*rt,wf_actor_raw_t*r,wf_r17_referee_ctx_t*c){
 wf_actor_raw_t*a2,*a3;uint16_t x,y;wf_status_t s=gather_204fa(rt,r,&a2,&a3,c);if(s!=WF_OK)return s;
 x=wf_be16(a2,WF_A_X);y=wf_be16(a2,WF_A_Y);
 if(!bit(a2,WF_A_FLAGS33,6)){
  uint16_t x3=wf_be16(a3,WF_A_X),y3=wf_be16(a3,WF_A_Y);
  x=(uint16_t)(((uint32_t)x+(uint32_t)x3)>>1);y=(uint16_t)(((uint32_t)y+(uint32_t)y3)>>1);
 }
 wf_r17_m68k_move_w(&rt->m68k_regs,0u,x);
 wf_r17_m68k_move_w(&rt->m68k_regs,2u,y);
 if(!bit(a2,WF_A_FLAGS33,6))wf_r17_m68k_move_w(&rt->m68k_regs,1u,wf_be16(a2,WF_A_Y)<wf_be16(a3,WF_A_Y)?wf_be16(a2,WF_A_Y):wf_be16(a3,WF_A_Y));
 wf_set_be16(r,R_TARGET_X,x);wf_set_be16(r,0x1Eu,y);return WF_OK;
}
/* $2052E exact threshold flag writer. */
static void target_flags_2052e(wf_actor_raw_t*r){
 wf_set_u8(r,R_PIN,(uint8_t)(wf_u8(r,R_PIN)&0xE7u));
 if(wf_be16(r,R_TARGET_X)<0x0270u)setbit(r,R_PIN,3);
 if(wf_be16(r,0x1Eu)<0x0160u)setbit(r,R_PIN,4);
}

wf_status_t wf_r17_ref_acquire_20556(wf_runtime_t*rt,wf_actor_raw_t*r,int*found){
 unsigned i;if(!rt||!r||!found)return WF_BAD_STATE;*found=0;
 if(rt->g.credited_route==0u){wf_r17_m68k_moveq(&rt->m68k_regs,5u,1);return WF_OK;}
 wf_r17_m68k_moveq(&rt->m68k_regs,4u,8);
 for(i=0;i<WF_ACTOR_SLOTS;i++,wf_r17_m68k_move_w(&rt->m68k_regs,4u,(uint16_t)(8u-i))){
  wf_actor_raw_t*a=&rt->actors[i];if(!bit(a,WF_A_ACTIVE,7))continue;
  if(bit(a,WF_A_PINFLAGS,0)){wf_set_be16(r,R_STATE,0x8001u);wf_set_be32(r,R_LINK,wf_actor_rom_base(i));*found=1;wf_r17_m68k_moveq(&rt->m68k_regs,5u,0);return WF_OK;}
  if(bit(a,WF_A_PINFLAGS,1)&&bit(a,WF_A_FLAGS33,0)){wf_set_be16(r,R_STATE,0x8004u);wf_set_be32(r,R_LINK,wf_actor_rom_base(i));*found=1;wf_r17_m68k_moveq(&rt->m68k_regs,5u,0);return WF_OK;}
 }
 wf_r17_m68k_moveq(&rt->m68k_regs,5u,1);return WF_OK;
}

static wf_status_t primary5(wf_runtime_t*rt,wf_actor_raw_t*r,wf_r17_referee_ctx_t*c){
 wf_status_t s;int f;if((rt->g.mode_0161&1u)==0u){s=target_204b2(rt,r,c);if(s!=WF_OK)return s;}else{wf_set_be16(r,R_TARGET_X,0x0268u);wf_set_be16(r,0x1Eu,0x0158u);}
 target_flags_2052e(r);
 if(wf_be32(r,R_LINK)!=0u){wf_actor_raw_t*a=aptr(rt,wf_be32(r,R_LINK));if(!a)return block(c,0x0001F9C8u);if(wf_be16(a,WF_A_LOCKOUT)==0u)return WF_OK;}
 return wf_r17_ref_acquire_20556(rt,r,&f);
}
static wf_status_t cancel_1f9e0(wf_runtime_t*rt,wf_actor_raw_t*r,wf_r17_referee_ctx_t*c){
 wf_actor_raw_t*a;wf_status_t s=linked(rt,r,&a,c,0x0001F9E0u);if(s!=WF_OK)return s;
 if(bit(a,WF_A_PINFLAGS,0))return WF_OK;
 if(!c||!c->clear_206fe)return block(c,0x000206FEu);
 s=c->clear_206fe(rt,c->user);if(s!=WF_OK)return s;
 wf_set_be16(r,R_STATE,0x8005u);return WF_OK;
}
static wf_status_t primary6(wf_runtime_t*rt,wf_actor_raw_t*r,wf_r17_referee_ctx_t*c){if(wf_u8(r,R_PHASE+1u)>=6u)return WF_OK;return cancel_1f9e0(rt,r,c);}
static wf_status_t primary2_7(wf_runtime_t*rt,wf_actor_raw_t*r,wf_r17_referee_ctx_t*c){
 wf_actor_raw_t*old;int found;wf_status_t s=linked(rt,r,&old,c,0x0001F9F8u);if(s!=WF_OK)return s;
 if(wf_be16(old,WF_A_LOCKOUT)!=0u){wf_set_be16(r,R_STATE,0x8005u);return WF_OK;}
 s=wf_r17_ref_acquire_20556(rt,r,&found);if(s!=WF_OK)return s;if(found)return WF_OK;
 if(!bit(old,WF_A_FLAGS32,0)&&!(low(old,WF_A_STATE)==1u&&wf_u8(old,WF_A_CPU_SUBSTATE)==4u))return WF_OK;
 if(actor_ptr(rt,old)==rt->g.global_1688){wf_set_be16(r,R_STATE,0x8005u);return WF_OK;}
 if(!aptr(rt,rt->g.global_1688))return block(c,0x001C1688u);
 wf_set_be32(r,R_LINK,rt->g.global_1688);wf_set_be16(r,R_STATE,0x8002u);return WF_OK;
}
static wf_status_t primary4_8(wf_runtime_t*rt,wf_actor_raw_t*r,wf_r17_referee_ctx_t*c){
 unsigned i;wf_actor_raw_t*a;wf_status_t s=linked(rt,r,&a,c,0x0001FA40u);if(s!=WF_OK)return s;
 if(wf_be16(a,WF_A_LOCKOUT)!=0u||!bit(a,WF_A_PINFLAGS,1)){wf_set_be16(r,R_STATE,0x8005u);return WF_OK;}
 for(i=0;i<WF_ACTOR_SLOTS;i++)if(bit(&rt->actors[i],WF_A_PINFLAGS,0)){wf_set_be32(r,R_LINK,wf_actor_rom_base(i));wf_set_be16(r,R_STATE,0x8001u);return WF_OK;}
 return WF_OK;
}
static wf_status_t primary3(wf_runtime_t*rt,wf_actor_raw_t*r,wf_r17_referee_ctx_t*c){
 wf_actor_raw_t*a2,*a3;uint16_t x2,x3,mid,rx,d;wf_status_t s;if((rt->g.mode_0161&2u)!=0u)rt->g.global_1682=0u;
 s=gather_204fa(rt,r,&a2,&a3,c);if(s!=WF_OK)return s;x2=wf_be16(a2,WF_A_X);x3=wf_be16(a3,WF_A_X);mid=(uint16_t)(((uint32_t)x2+(uint32_t)x3)>>1);rx=wf_be16(r,R_X);d=rx>=mid?(uint16_t)(rx-mid):(uint16_t)(mid-rx);
 wf_set_u8(r,R_ANGLE+1u,rx<mid?0x40u:0xC0u);if(d<0x20u)chgbit(r,R_ANGLE+1u,7);else if(d<0x28u)wf_set_be16(r,R_SPEED,0u);else wf_set_u8(r,R_SPEED+1u,0x14u);return WF_OK;
}
static wf_status_t primary_dispatch(wf_runtime_t*rt,wf_actor_raw_t*r,wf_r17_referee_ctx_t*c){
 uint8_t st=wf_u8(r,R_STATE+1u);int f;wf_status_t s;if(st>=11u)return block(c,0x0001F952u);
 switch(st){
  case 0:if(rt->g.global_1682!=0u){rt->g.global_1682=(uint16_t)(rt->g.global_1682-1u);if(rt->g.global_1682==0u){wf_set_be16(r,R_STATE,0x8002u);if(!aptr(rt,rt->g.global_1684))return block(c,0x001C1684u);wf_set_be32(r,R_LINK,rt->g.global_1684);return WF_OK;}}return primary5(rt,r,c);
  case 1:return cancel_1f9e0(rt,r,c);
  case 2:case 7:return primary2_7(rt,r,c);
  case 3:return primary3(rt,r,c);
  case 4:case 8:return primary4_8(rt,r,c);
  case 5:return primary5(rt,r,c);
  case 6:return primary6(rt,r,c);
  case 9:case 10:return WF_OK;
 }
 s=wf_r17_ref_acquire_20556(rt,r,&f);(void)s;return block(c,0x0001F952u);
}

/* $205AC.  This is kept byte/word shaped, including the exact 32-byte angle table. */
static void direction_205ac(wf_runtime_t*rt,wf_actor_raw_t*r){
 static const uint8_t t[32]={0xC0,0x80,0x40,0x80,0x40,0x80,0x40,0x80,0x40,0x00,0xC0,0x00,0x40,0x00,0x40,0x00,0x40,0x80,0xC0,0x80,0x40,0x80,0xC0,0x80,0xC0,0x00,0x40,0x00,0xC0,0x00,0x40,0x00};
 uint16_t idx=(uint16_t)((wf_be16(r,R_FLAGS34)&0x001Eu)>>1),tx=wf_be16(r,R_TARGET_X),x=wf_be16(r,R_X),dx=(uint16_t)(tx-x),m;
 wf_r17_m68k_moveq(&rt->m68k_regs,0u,0);wf_r17_m68k_move_w(&rt->m68k_regs,0u,idx);
 wf_r17_m68k_move_w(&rt->m68k_regs,1u,dx);
 if(dx<8u){m=(uint16_t)(idx&0x000Au);wf_r17_m68k_move_w(&rt->m68k_regs,1u,m);if(m!=0u&&m!=0x000Au){wf_set_be16(r,R_SPEED,0u);return;}}
 wf_r17_m68k_move_w(&rt->m68k_regs,1u,tx);
 if(tx<x){idx=(uint16_t)(idx|0x0010u);setbit(r,R_FACING,7);}
 wf_r17_m68k_move_w(&rt->m68k_regs,0u,idx);
 wf_set_u8(r,R_ANGLE+1u,t[idx]);wf_set_u8(r,R_SPEED+1u,0x16u);
}
static void add_corr_xy(wf_actor_raw_t*r){wf_set_be16(r,R_X,(uint16_t)(wf_be16(r,R_X)+wf_be16(r,R_CORR_X)));wf_set_be16(r,R_Y,(uint16_t)(wf_be16(r,R_Y)+wf_be16(r,R_CORR_Y)));}
static void walk_cycle8(wf_actor_raw_t*r){if(byte_dec(r,R_TICK+1u)!=0u)return;wf_set_u8(r,R_TICK+1u,8u);if(byte_dec(r,R_PHASE+1u)==0u){wf_set_u8(r,R_VISUAL+1u,0u);wf_set_u8(r,R_PHASE+1u,4u);}else byte_inc(r,R_VISUAL+1u);}

static wf_status_t action0(wf_runtime_t*rt,wf_actor_raw_t*r,wf_r17_referee_ctx_t*c){
 wf_status_t s;if((rt->g.mode_0161&1u)!=0u){if(entry_latch(r)){wf_set_u8(r,R_TICK+1u,8u);wf_set_u8(r,R_VISUAL,wf_u8(r,R_FACING));wf_set_u8(r,WF_A_MOTION_MODE,1u);}return WF_OK;}
 if(entry_latch(r)){wf_set_u8(r,R_TICK+1u,8u);wf_set_u8(r,R_PHASE+1u,4u);wf_set_u8(r,R_VISUAL+1u,0u);wf_set_u8(r,R_VISUAL,wf_u8(r,R_FACING));wf_set_u8(r,WF_A_MOTION_MODE,1u);}
 direction_205ac(rt,r);if(wf_u8(r,R_SPEED+1u)==0u)return WF_OK;wf_physics_integrate_common(r);wf_set_be16(r,R_CONTACT,0u);
 if(!c||!c->resolver_280dc)return block(c,0x000280DCu);
 s=c->resolver_280dc(rt,r,c->user);if(s!=WF_OK)return s;
 if(wf_u8(r,R_CONTACT)!=0u){add_corr_xy(r);wf_set_u8(r,R_PIN,(uint8_t)(wf_u8(r,R_PIN)&0xF9u));if(bit(r,R_CONTACT+1u,0))wf_set_u8(r,R_PIN,(uint8_t)(wf_u8(r,R_PIN)|6u));else if(bit(r,R_CONTACT+1u,1))wf_set_u8(r,R_PIN,(uint8_t)(wf_u8(r,R_PIN)|2u));else if(bit(r,R_CONTACT+1u,3))wf_set_u8(r,R_PIN,(uint8_t)(wf_u8(r,R_PIN)|4u));}
 walk_cycle8(r);return WF_OK;
}
static wf_status_t approach(wf_runtime_t*rt,wf_actor_raw_t*r,wf_r17_referee_ctx_t*c){
 wf_actor_raw_t*a;uint16_t tx,ty,off=0x50u;wf_status_t s;if(entry_latch(r)){
  clrbit(r,R_FACING,7);wf_set_u8(r,R_TICK+1u,1u);wf_set_u8(r,R_PHASE+1u,4u);wf_set_u8(r,R_VISUAL+1u,0u);wf_set_u8(r,R_SPEED+1u,0x1Cu);
  s=linked(rt,r,&a,c,0x0001FC22u);if(s!=WF_OK)return s;tx=wf_be16(a,WF_A_X);ty=wf_be16(a,WF_A_Y);
  if(wf_u8(r,R_STATE+1u)==4u){off=0x30u;ty=(uint16_t)(ty-0x10u);if(low(a,WF_A_STATE)==5u){if(wf_u8(a,WF_A_SELECTOR_LO)==0x22u)ty=(uint16_t)(ty-0x10u);else if(wf_u8(a,WF_A_SELECTOR_LO)==0x09u)off=0x50u;}}
  if(wf_be16(a,WF_A_X)<0x0280u)tx=(uint16_t)(tx-off);else tx=(uint16_t)(tx+off);wf_set_be16(r,R_TARGET2_X,tx);wf_set_be16(r,R_TARGET2_Y,ty);
  if(tx<wf_be16(r,R_X))setbit(r,R_FACING,7);else clrbit(r,R_FACING,7);wf_set_u8(r,R_VISUAL,wf_u8(r,R_FACING));
 }
 if(!c||!c->direction_20c8)return block(c,0x000020C8u);
 s=c->direction_20c8(rt,r,c->user);if(s!=WF_OK)return s;
 wf_physics_integrate_common(r);
 if(!c->resolver_280dc)return block(c,0x000280DCu);
 wf_set_be16(r,R_CONTACT,0u);s=c->resolver_280dc(rt,r,c->user);if(s!=WF_OK)return s;
 if(wf_u8(r,R_CONTACT+1u)!=0u){add_corr_xy(r);}
 if(wf_u8(r,R_CONTACT+1u)==0u){uint16_t dx=(uint16_t)(wf_be16(r,R_X)-wf_be16(r,R_TARGET2_X)),dy=(uint16_t)(wf_be16(r,R_Y)-wf_be16(r,R_TARGET2_Y));if(wf_be16(r,R_X)<wf_be16(r,R_TARGET2_X))dx=(uint16_t)(0u-dx);if(wf_be16(r,R_Y)<wf_be16(r,R_TARGET2_Y))dy=(uint16_t)(0u-dy);if(dx>=6u||dy>=6u){walk_cycle8(r);return WF_OK;}}
 s=linked(rt,r,&a,c,0x0001FC22u);if(s!=WF_OK)return s;
 if(wf_u8(r,R_LATCH+1u)==2u){wf_set_be16(r,R_Y,(uint16_t)(wf_be16(a,WF_A_Y)+1u));wf_set_be16(r,R_STATE,0x8007u);return WF_OK;}
 if(wf_u8(r,R_LATCH+1u)==1u){wf_set_be16(r,R_Y,(uint16_t)(wf_be16(a,WF_A_Y)+1u));wf_set_be16(r,R_PHASE,0u);wf_set_be16(r,R_STATE,0x8006u);return WF_OK;}
 wf_set_be16(r,R_STATE,0x8008u);return WF_OK;
}

static wf_status_t emit_audio(wf_runtime_t*rt,wf_actor_raw_t*r,wf_r17_referee_ctx_t*c){if(!c||!c->audio_2052)return block(c,0x00002052u);return c->audio_2052(rt,wf_be16(r,R_SOUND),c->user);}
static wf_status_t count6(wf_runtime_t*rt,wf_actor_raw_t*r,wf_r17_referee_ctx_t*c){
 wf_actor_raw_t*a1,*a2;wf_status_t s;if(entry_latch(r)){
  set_global_169a(rt,0u);wf_set_be16(r,R_PHASE,0u);wf_set_be16(r,R_SOUND,0x3162u);s=linked(rt,r,&a1,c,0x0002003Cu);if(s!=WF_OK)return s;wf_set_u8(r,R_FACING,wf_u8(a1,WF_A_FACING));wf_set_u8(r,R_VISUAL,wf_u8(r,R_FACING));wf_set_u8(r,R_TICK+1u,20u);wf_set_u8(r,R_VISUAL+1u,4u);
 }
 s=linked(rt,r,&a1,c,0x00020058u);if(s!=WF_OK)return s;a2=aptr(rt,wf_be32(a1,WF_A_LINK));if(!a2)return block(c,0x0002005Cu);
 if(byte_dec(r,R_TICK+1u)!=0u)return WF_OK;
 byte_inc(r,R_PHASE+1u);byte_inc(a2,WF_A_PROGRESS109);
 if(wf_u8(r,R_PHASE+1u)==7u){if((rt->g.mode_0161&1u)==0u){wf_set_be16(r,R_STATE,0x8009u);if(!c||!c->clear_206fe)return block(c,0x000206FEu);s=c->clear_206fe(rt,c->user);if(s!=WF_OK)return s;wf_set_be16(a1,0x4Au,4u);return WF_OK;}if(!c||!c->clear_206fe)return block(c,0x000206FEu);s=c->clear_206fe(rt,c->user);if(s!=WF_OK)return s;wf_set_be16(r,R_STATE,0x8005u);return WF_OK;}
 if((wf_u8(r,R_PHASE+1u)&1u)==0u){
  inc_global_169a(rt);
  /* $20088 MOVEQ #0; MOVE.W +$24; LSR.W #1 before $2067C. */
  wf_r17_m68k_move_l(&rt->m68k_regs,0u,(uint32_t)(wf_be16(r,R_PHASE)>>1));
  if(!c||!c->display_2067c)return block(c,0x0002067Cu);
  s=c->display_2067c(rt,c->user);if(s!=WF_OK)return s;
  /* Both output owners preserve D0; the caller selects the sound at $20094. */
  wf_r17_m68k_move_w(&rt->m68k_regs,0u,wf_be16(r,R_SOUND));
  s=emit_audio(rt,r,c);if(s!=WF_OK)return s;
  wf_set_be16(r,R_SOUND,(uint16_t)(wf_be16(r,R_SOUND)+1u));
 }
 if(wf_u8(r,R_VISUAL+1u)==5u){wf_set_u8(r,R_TICK+1u,56u);wf_set_u8(r,R_VISUAL+1u,6u);}else{wf_set_u8(r,R_TICK+1u,32u);wf_set_u8(r,R_VISUAL+1u,5u);}
 if(wf_u8(r,R_PHASE+1u)==6u){wf_set_u8(r,R_TICK+1u,(uint8_t)(wf_u8(r,R_TICK+1u)+16u));if((rt->g.mode_0161&1u)!=0u){if(!c||!c->royal_phase6_21358)return block(c,0x00021358u);s=c->royal_phase6_21358(rt,a1,c->user);if(s!=WF_OK)return s;wf_set_be16(a1,WF_A_CREDIT,(uint16_t)(wf_be16(a1,WF_A_CREDIT)+1u));a1=aptr(rt,wf_be32(a1,WF_A_LINK));if(!a1)return block(c,0x000200ECu);setbit(a1,WF_A_FLAGS32,4);return WF_OK;}
  wf_set_be16(a1,WF_A_LOCKOUT,0x4000u);a2=aptr(rt,wf_be32(a1,WF_A_TEAM_LINK));if(!a2)return block(c,0x00020102u);wf_set_be16(a2,WF_A_LOCKOUT,0x4000u);a1=aptr(rt,wf_be32(a1,WF_A_LINK));if(!a1)return block(c,0x0002010Cu);wf_set_be16(a1,WF_A_LOCKOUT,0x4001u);a2=aptr(rt,wf_be32(a1,WF_A_TEAM_LINK));if(!a2)return block(c,0x00020116u);wf_set_be16(a2,WF_A_LOCKOUT,0x4001u);rt->g.global_169e=0u;if(!c||!c->tag_phase6_90d6)return block(c,0x000090D6u);return c->tag_phase6_90d6(rt,c->user);}
 return WF_OK;
}

static wf_status_t action5_10(wf_runtime_t*rt,wf_actor_raw_t*r,wf_r17_referee_ctx_t*c){
 static const uint8_t dir[4]={0x00u,0xC0u,0x80u,0x40u};static const uint8_t mask[4]={0x04u,0x02u,0x08u,0x01u};
 uint8_t idx;wf_status_t s;if(entry_latch(r)){wf_set_u8(r,R_SPEED+1u,0x16u);wf_set_u8(r,R_VISUAL+1u,0u);wf_set_u8(r,R_PHASE+1u,4u);wf_set_u8(r,R_TICK+1u,1u);if(wf_u8(r,R_LATCH+1u)==10u)wf_set_u8(r,R_ANGLE+1u,0xC0u);else{idx=(uint8_t)((wf_be16(r,R_FLAGS34)&6u)>>1);wf_set_u8(r,R_ANGLE+1u,dir[idx]);}}
 wf_physics_integrate_common(r);wf_set_be16(r,R_CONTACT,0u);if(!c||!c->resolver_280dc)return block(c,0x000280DCu);s=c->resolver_280dc(rt,r,c->user);if(s!=WF_OK)return s;
 if(wf_u8(r,R_CONTACT)!=0u){if(wf_u8(r,R_LATCH+1u)==10u)wf_set_be16(r,R_SPEED,0u);else{add_corr_xy(r);idx=(uint8_t)((wf_be16(r,R_FLAGS34)&6u)>>1);if((wf_be16(r,R_CONTACT)&mask[idx])==mask[idx]){wf_set_be16(r,R_STATE,0x8000u);return WF_OK;}}}
 if(byte_dec(r,R_TICK+1u)!=0u)return WF_OK;
 wf_set_u8(r,R_TICK+1u,6u);if(byte_dec(r,R_PHASE+1u)==0u){wf_set_u8(r,R_VISUAL+1u,0u);wf_set_u8(r,R_PHASE+1u,4u);}else byte_inc(r,R_VISUAL+1u);return WF_OK;
}

static wf_status_t action7(wf_runtime_t*rt,wf_actor_raw_t*r,wf_r17_referee_ctx_t*c){
 wf_actor_raw_t*a,*g;wf_status_t s;if(entry_latch(r)){chgbit(r,R_FACING,7);wf_set_u8(r,R_TICK+1u,10u);wf_set_u8(r,R_VISUAL+1u,7u);s=linked(rt,r,&a,c,0x000202D6u);if(s!=WF_OK)return s;clrbit(r,R_FACING,7);if(wf_be16(a,WF_A_X)>=wf_be16(r,R_X))setbit(r,R_FACING,7);wf_set_u8(r,R_VISUAL,wf_u8(r,R_FACING));g=aptr(rt,rt->g.global_1684);if(!g)return block(c,0x001C1684u);if(!bit(g,WF_A_FLAGS32,0))setbit(g,WF_A_FLAGS34,1);clrbit(g,WF_A_FLAGS34,0);g=aptr(rt,rt->g.global_1688);if(!g)return block(c,0x001C1688u);if(!bit(g,WF_A_FLAGS32,0))setbit(g,WF_A_FLAGS34,1);clrbit(g,WF_A_FLAGS34,0);}
 if(byte_dec(r,R_TICK+1u)!=0u)return WF_OK;
 if(wf_u8(r,R_VISUAL+1u)==7u){wf_set_u8(r,R_TICK+1u,8u);wf_set_u8(r,R_VISUAL+1u,8u);}else{wf_set_u8(r,R_TICK+1u,16u);wf_set_u8(r,R_VISUAL+1u,7u);}return WF_OK;
}
static wf_status_t action8(wf_runtime_t*rt,wf_actor_raw_t*r,wf_r17_referee_ctx_t*c){
 wf_actor_raw_t*a;wf_status_t s;if(entry_latch(r)){wf_set_u8(r,R_TICK+1u,10u);wf_set_u8(r,R_VISUAL+1u,13u);s=linked(rt,r,&a,c,0x00020368u);if(s!=WF_OK)return s;if(bit(a,WF_A_FLAGS32,0))wf_set_u8(r,R_VISUAL+1u,7u);clrbit(r,R_FACING,7);if(wf_be16(a,WF_A_X)>=wf_be16(r,R_X))setbit(r,R_FACING,7);wf_set_u8(r,R_VISUAL,wf_u8(r,R_FACING));}
 if(byte_dec(r,R_TICK+1u)!=0u)return WF_OK;
 s=linked(rt,r,&a,c,0x0002039Au);if(s!=WF_OK)return s;if(bit(a,WF_A_FLAGS32,0)){if(wf_u8(r,R_VISUAL+1u)==8u){wf_set_u8(r,R_TICK+1u,8u);wf_set_u8(r,R_VISUAL+1u,7u);}else{wf_set_u8(r,R_TICK+1u,10u);wf_set_u8(r,R_VISUAL+1u,8u);}}else{if(wf_u8(r,R_VISUAL+1u)==13u){wf_set_u8(r,R_TICK+1u,8u);wf_set_u8(r,R_VISUAL+1u,14u);}else{wf_set_u8(r,R_TICK+1u,10u);wf_set_u8(r,R_VISUAL+1u,13u);}}return WF_OK;
}
static wf_status_t result_chain_lock(wf_runtime_t*rt,wf_actor_raw_t*a,wf_r17_referee_ctx_t*c){wf_actor_raw_t*t,*p;if(!a)return block(c,0x00020472u);setbit(a,WF_A_LOCKOUT,7);t=aptr(rt,wf_be32(a,WF_A_TEAM_LINK));if(!t)return block(c,0x00020478u);setbit(t,WF_A_LOCKOUT,7);p=aptr(rt,wf_be32(a,WF_A_LINK));if(!p)return block(c,0x00020482u);setbit(p,WF_A_LOCKOUT,7);t=aptr(rt,wf_be32(p,WF_A_TEAM_LINK));if(!t)return block(c,0x0002048Cu);setbit(t,WF_A_LOCKOUT,7);return WF_OK;}
static wf_status_t action9(wf_runtime_t*rt,wf_actor_raw_t*r,wf_r17_referee_ctx_t*c){
 wf_actor_raw_t*a;wf_status_t s;if(entry_latch(r)){wf_set_u8(r,R_TICK+1u,8u);wf_set_u8(r,R_VISUAL+1u,15u);}if(byte_dec(r,R_TICK+1u)!=0u)return WF_OK;
 if(wf_u8(r,R_VISUAL+1u)==16u){if(!c||!c->result_20156)return block(c,0x00020156u);s=c->result_20156(rt,c->user);if(s!=WF_OK)return s;if((rt->g.mode_0161&1u)==0u){s=linked(rt,r,&a,c,0x0002041Cu);if(s!=WF_OK)return s;if(!bit(a,WF_A_ROLE,7)){rt->g.global_15d2=0x0Fu;rt->g.global_15d3=0x2Au;}else{rt->g.global_15d4=rt->g.global_15d5=0u;rt->g.global_15d2=0x0Fu;rt->g.global_15d3=0x2Cu;}}}
 else if(wf_u8(r,R_VISUAL+1u)==17u){if((rt->g.mode_0161&1u)!=0u){wf_set_be16(r,R_STATE,0x800Au);return WF_OK;}s=linked(rt,r,&a,c,0x0002045Cu);if(s!=WF_OK)return s;if(bit(a,WF_A_ROLE,7)){if(!c||!c->audio_2052)return block(c,0x00002052u);s=c->audio_2052(rt,0x3108u,c->user);if(s!=WF_OK)return s;}s=result_chain_lock(rt,a,c);if(s!=WF_OK)return s;wf_set_be16(r,R_STATE,0x8005u);return WF_OK;}
 byte_inc(r,R_VISUAL+1u);wf_set_u8(r,R_TICK+1u,18u);return WF_OK;
}

static wf_status_t action_dispatch(wf_runtime_t*rt,wf_actor_raw_t*r,wf_r17_referee_ctx_t*c){
 uint8_t st=wf_u8(r,R_LATCH+1u);if(st>=11u)return block(c,0x0001FB1Au);
 switch(st){case 0:return action0(rt,r,c);case 1:case 2:case 4:return approach(rt,r,c);case 3:return block(c,0x0001FD5Eu);case 5:case 10:return action5_10(rt,r,c);case 6:return count6(rt,r,c);case 7:return action7(rt,r,c);case 8:return action8(rt,r,c);case 9:return action9(rt,r,c);}return block(c,0x0001FB1Au);
}

wf_status_t wf_r17_referee_logic_1f914(wf_runtime_t*rt,wf_actor_raw_t*r,void*user){
 wf_r17_referee_ctx_t*c=(wf_r17_referee_ctx_t*)user;wf_status_t s;if(!rt||!r||!c)return WF_BAD_STATE;c->blocked_dependency_rom=0u;
 wf_r17_m68k_move_l(&rt->m68k_regs,0u,(uint32_t)wf_u8(r,R_STATE+1u)*4u);
 s=primary_dispatch(rt,r,c);if(s!=WF_OK)return s;
 if(bit(r,R_STATE,7)){clrbit(r,R_STATE,7);wf_set_be16(r,R_LATCH,wf_be16(r,R_STATE));}
 wf_r17_m68k_move_l(&rt->m68k_regs,0u,(uint32_t)wf_u8(r,R_LATCH+1u)*4u);
 return action_dispatch(rt,r,c);
}


/* $F8E4. A1 is the fixed auxiliary object at $1C1164. The referee itself
 * receives render-list 1 before the auxiliary object is populated/rendered. */
wf_status_t wf_r17_royal_f8e4(wf_runtime_t *rt,wf_actor_raw_t *referee,wf_r17_render_ctx_t *render){
 wf_actor_raw_t *o;uint16_t x;wf_status_t s;if(!rt||!referee||!render)return WF_BAD_STATE;o=&rt->referee_aux;
 wf_set_be16(referee,WF_A_RENDER_LIST,1u);wf_set_be16(o,WF_A_RENDER_LIST,0u);x=wf_be16(referee,WF_A_X);
 wf_set_be16(o,WF_A_ACTIVE,0x8000u);wf_set_be16(o,WF_A_CHARACTER,0x000Eu);
 if(x<0x0290u){wf_set_be16(o,WF_A_MIRROR,0x0001u);wf_set_be16(o,WF_A_X,0x0269u);wf_set_be16(o,WF_A_Y,0x0151u);wf_set_be16(o,WF_A_Z,0x0146u);}
 else if(x>=0x03B0u){wf_set_be16(o,WF_A_MIRROR,0x8001u);wf_set_be16(o,WF_A_X,0x03C7u);wf_set_be16(o,WF_A_Y,0x0151u);wf_set_be16(o,WF_A_Z,0x0146u);}
 else{wf_set_be16(o,WF_A_MIRROR,0u);wf_set_be16(o,WF_A_X,x);wf_set_be16(o,WF_A_Y,0x0151u);wf_set_be16(o,WF_A_Z,0x013Eu);}
 s=wf_r17_world_to_screen_247c(o,render->scroll_a_x,render->scroll_a_y);if(s!=WF_OK)return s;return wf_r17_queue_27b8(render,o);
}
