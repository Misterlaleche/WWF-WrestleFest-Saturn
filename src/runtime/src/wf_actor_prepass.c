#include "wf_actor_prepass.h"
#include "wf_gameplay_bindings.h"
#include "wf_r17_human_raw23.h"
#include "wf_r17_follow_10e86.h"
#include "wf_r17_rng1129e.h"

static int bit(const wf_actor_raw_t*a,size_t o,unsigned b){return (wf_u8(a,o)&(uint8_t)(1u<<b))!=0u;}
static void setbit(wf_actor_raw_t*a,size_t o,unsigned b){wf_set_u8(a,o,(uint8_t)(wf_u8(a,o)|(uint8_t)(1u<<b)));}
static void clrbit(wf_actor_raw_t*a,size_t o,unsigned b){wf_set_u8(a,o,(uint8_t)(wf_u8(a,o)&(uint8_t)~(1u<<b)));}
static wf_status_t block(wf_gameplay_binding_context_t*c,uint32_t dep){if(c){c->last_blocked_phase=0x00f18au;c->last_blocked_dependency=dep;}return WF_BLOCKED;}
static wf_actor_raw_t *target(wf_runtime_t*rt,wf_actor_raw_t*a){return wf_actor_from_rom_ptr(rt,wf_be32(a,WF_A_TARGET),0);}

/* Exact $10BE8 facing update used by $F218/$F292. */
static wf_status_t facing_10be8(wf_runtime_t*rt,wf_actor_raw_t*a,wf_gameplay_binding_context_t*c){
 wf_actor_raw_t*t=target(rt,a);if(!t)return block(c,0x00010be8u);
 uint16_t x=wf_be16(t,WF_A_X),off=0u;uint8_t st=wf_u8(t,WF_A_STATE+1u);
 if(st==4u){if(wf_u8(t,WF_A_STATE4_LO)==8u)off=0x40u;else if(wf_u8(t,WF_A_STATE4_LO)==9u)off=0x10u;}
 else if(st==5u){if(wf_u8(t,WF_A_SELECTOR_LO)==0x22u)off=0x30u;else if(wf_u8(t,WF_A_SELECTOR_LO)==0x52u)off=0x10u;}
 if(off!=0u)x=bit(t,WF_A_FACING,7)?(uint16_t)(x-off):(uint16_t)(x+off);
 if(x>=wf_be16(a,WF_A_X))setbit(a,WF_A_FACING,7);else clrbit(a,WF_A_FACING,7);
 return WF_OK;
}

static wf_status_t state0(wf_runtime_t*rt,wf_actor_raw_t*a,wf_gameplay_binding_context_t*c){
 if(bit(a,WF_A_FLAGS34,1)){
  if(!bit(a,WF_A_FLAGS33,0)){wf_set_be16(a,WF_A_STATE,1u);wf_set_be16(a,WF_A_AE,4u);}
  else clrbit(a,WF_A_FLAGS34,1);
  return WF_OK;
 }
 return facing_10be8(rt,a,c);
}
static const uint16_t dir_f2d6[16]={0x0000,0x0040,0x00c0,0x0000,0x0020,0x00e0,0x0000,0x0000,0x0080,0x0060,0x00a0,0x0000,0x0000,0x0000,0x0000,0x0000};
static wf_status_t state1_0(wf_runtime_t*rt,wf_actor_raw_t*a,wf_gameplay_binding_context_t*c){
 if(bit(a,WF_A_FLAGS34,1)){if(!bit(a,WF_A_FLAGS33,0)){wf_set_be16(a,WF_A_STATE,1u);wf_set_be16(a,WF_A_AE,4u);}else clrbit(a,WF_A_FLAGS34,1);return WF_OK;}
 uint16_t idx=wf_be16(a,WF_A_INPUT_DERIVED);if(idx>=16u)return block(c,0x00f2d6u);wf_set_be16(a,WF_A_ANGLE_WORD,dir_f2d6[idx]);
 wf_status_t s=facing_10be8(rt,a,c);if(s!=WF_OK)return s;
 s=wf_r17_follow_10e86(rt,a,rt->g.mode_0161);if(s!=WF_OK)return block(c,0x00010e86u);
 return WF_OK;
}
static wf_status_t state1_1(wf_actor_raw_t*a){
 if(bit(a,WF_A_INPUT_DERIVED+1u,2)){wf_set_be16(a,WF_A_ANGLE_WORD,0x0008u);if(bit(a,WF_A_FLAGS33,7))wf_set_u8(a,WF_A_ANGLE,(uint8_t)~wf_u8(a,WF_A_ANGLE));}
 else if(bit(a,WF_A_INPUT_DERIVED+1u,3)){wf_set_be16(a,WF_A_ANGLE_WORD,0x0088u);if(bit(a,WF_A_FLAGS33,7))wf_set_u8(a,WF_A_ANGLE,(uint8_t)~wf_u8(a,WF_A_ANGLE));}
 else wf_set_be16(a,WF_A_ANGLE_WORD,0u);
 return WF_OK;
}
static wf_status_t state1(wf_runtime_t*rt,wf_actor_raw_t*a,wf_gameplay_binding_context_t*c){
 static const uint8_t map[14]={0,1,2,2,3,4,4,4,4,4,4,3,3,3};uint16_t i=(uint16_t)(wf_be16(a,WF_A_AE)&0x003fu);if(i>=14u)return block(c,0x00f25au);
 switch(map[i]){case 0:return state1_0(rt,a,c);case 1:return state1_1(a);case 2:case 3:return WF_OK;default:wf_set_be16(a,WF_A_STATE,0u);wf_set_be16(a,WF_A_AE,0u);wf_set_be32(a,WF_A_LINK,0u);return WF_OK;}
}
static wf_status_t state2(wf_runtime_t*rt,wf_actor_raw_t*a,wf_gameplay_binding_context_t*c){
 uint8_t old=wf_u8(a,WF_A_STATE);setbit(a,WF_A_STATE,6);
 if((old&0x40u)==0u){
  if(wf_u8(a,WF_A_LATCHED+1u)==6u||wf_be16(a,WF_A_TIMER44)!=0u)return WF_OK;
  uint16_t oldf=wf_be16(a,WF_A_FACING);uint16_t d=(uint16_t)(wf_be16(a,WF_A_INPUT_DERIVED)&3u);
  if(d!=0u){if(d==1u){wf_set_be16(a,WF_A_ANGLE_WORD,0x0040u);setbit(a,WF_A_FACING,7);}else{wf_set_be16(a,WF_A_ANGLE_WORD,0x00c0u);clrbit(a,WF_A_FACING,7);}if(oldf!=wf_be16(a,WF_A_FACING))setbit(a,WF_A_FLAGS32,2);}
  else {uint8_t v=(uint8_t)(wf_u8(a,WF_A_FACING)&0x80u);v^=0x80u;v=(uint8_t)(v+0x40u);wf_set_u8(a,WF_A_ANGLE,v);}return WF_OK;
 }
 if(bit(a,WF_A_FLAGS34,1)&&wf_be16(a,WF_A_TIMER44)==0u){wf_set_be16(a,WF_A_STATE,3u);return WF_OK;}
 if((rt->g.global_00a3&3u)!=0u&&(rt->g.mode_0161&1u)!=0u&&wf_u8(a,WF_A_TIMER44+1u)==1u){
  wf_actor_raw_t*t=target(rt,a);if(!t)return block(c,0x00f3fau);uint16_t dx=wf_be16(a,WF_A_X)>=wf_be16(t,WF_A_X)?(uint16_t)(wf_be16(a,WF_A_X)-wf_be16(t,WF_A_X)):(uint16_t)(wf_be16(t,WF_A_X)-wf_be16(a,WF_A_X));
  if(dx<0x60u){
   uint8_t o=wf_u8(a,WF_A_TIMER46);setbit(a,WF_A_TIMER46,0);
   if((o&1u)==0u){
    wf_r17_1129e_trace_t tr;int carry=0;wf_status_t rs;unsigned slot=(unsigned)(a-rt->actors);
    if(!rt->g.rng_state_valid||!rt->m68k_regs.valid)return block(c,0x000021b4u);
    wf_r17_m68k_move_w(&rt->m68k_regs,1u,3u); /* $F418 MOVE.W #3,D1 */
    rs=wf_r17_1129e_live(rt,slot,rt->g.mode_0161,rt->g.progression_0162,&rt->g.rng_state,rt->g.frame_counter,&rt->m68k_regs,&tr,&carry);
    if(rs!=WF_OK)return block(c,tr.blocked_dependency_rom?tr.blocked_dependency_rom:0x0001129eu);
    if(carry)wf_set_be16(a,WF_A_STATE,3u);
   }
  }
 }
 if(wf_be16(a,WF_A_TIMER44)!=0u)return WF_OK;
 if(bit(a,WF_A_ANGLE,7)){if(bit(a,WF_A_INPUT_DERIVED+1u,0))wf_set_be16(a,WF_A_STATE,3u);}
 else if(bit(a,WF_A_INPUT_DERIVED+1u,1))wf_set_be16(a,WF_A_STATE,3u);
 return WF_OK;
}
static wf_status_t state5(wf_runtime_t*rt,wf_actor_raw_t*a,wf_gameplay_binding_context_t*c){
 uint8_t s=wf_u8(a,WF_A_SELECTOR_LO);if(s==0x3cu||s==0x3du||s==0x3eu){wf_set_be16(a,WF_A_STATE,0u);return WF_OK;}
 if(s==0x16u){uint16_t i=wf_be16(a,WF_A_INPUT_DERIVED);wf_status_t rs;if(i>=16u)return block(c,0x00f2d6u);wf_set_be16(a,WF_A_ANGLE_WORD,dir_f2d6[i]);rs=wf_r17_follow_10e86(rt,a,rt->g.mode_0161);return rs==WF_OK?WF_OK:block(c,0x00010e86u);}return WF_OK;
}
static wf_status_t dispatch_state(wf_runtime_t*rt,wf_actor_raw_t*a,wf_gameplay_binding_context_t*c){
 uint8_t st=wf_u8(a,WF_A_STATE+1u);switch(st){case 0:return state0(rt,a,c);case 1:return state1(rt,a,c);case 2:return state2(rt,a,c);case 3:case 4:return WF_OK;case 5:return state5(rt,a,c);case 6:case 7:case 8:return WF_OK;case 9:if(bit(a,WF_A_INPUT_DERIVED+1u,3))wf_set_be16(a,WF_A_STATE,0x000au);return WF_OK;case 10:case 11:case 12:return WF_OK;default:return block(c,0x00f1e4u);}
}
wf_status_t wf_phase_actor_prepass_f18a(wf_runtime_t*rt,uint32_t addr,void*user){
 (void)addr;wf_gameplay_binding_context_t*c=(wf_gameplay_binding_context_t*)user;if(!rt)return WF_BAD_STATE;
 wf_r17_m68k_move_w(&rt->m68k_regs,0u,0u);
 for(unsigned i=0;i<WF_ACTOR_SLOTS;i++){
  wf_r17_m68k_move_w(&rt->m68k_regs,0u,(uint16_t)(i+1u));
  wf_actor_raw_t*a=&rt->actors[i];if(!bit(a,WF_A_ACTIVE,7))continue;if(bit(a,WF_A_ROLE,6)||bit(a,WF_A_ROLE,7))continue;
  if(bit(a,WF_A_FLAGS33,1)&&!bit(a,WF_A_FLAGS34,2)){
   wf_r17_human_raw23_ctx_t hc;wf_r17_human_raw23_trace_t ht;wf_status_t hs;
   hc.mode_0161=rt->g.mode_0161;hc.global_1670=&rt->g.global_1670;hc.fixed_object0=&rt->fixed_object0;hc.fixed_object1=&rt->fixed_object1;
   hc.global_0162=rt->g.progression_0162;hc.rng_1129e=0;hc.m68k_live=&rt->m68k_regs;hc.rng_state_1c005c=&rt->g.rng_state;hc.global_1c0080=rt->g.frame_counter;
   hs=wf_r17_human_dcde_raw23(rt,i,&hc,&ht);
   if(hs!=WF_OK)return block(c,ht.blocked_dependency_rom?ht.blocked_dependency_rom:0x0000dcdeu);
  }
  wf_set_be16(a,WF_A_CPU_B4,0u);wf_set_be16(a,WF_A_CPU_B6,0u);
  if(wf_u8(a,WF_A_STATE+1u)==0xffu)continue;
  wf_r17_m68k_move_w(&rt->m68k_regs,1u,(uint16_t)(wf_u8(a,WF_A_STATE+1u)*4u));
  wf_status_t s=dispatch_state(rt,a,c);if(s!=WF_OK)return s;
 }
 return WF_OK;
}
