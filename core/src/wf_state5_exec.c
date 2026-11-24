#include <string.h>
#include "wf_state5_exec.h"
#include "wf_state5.h"
#include "wf_cpu_runtime.h"
#include "wf_r17_resolver_280dc.h"
#include "wf_r17_candidate_2187e.h"
extern wf_status_t wf_cpu_pursuit_1d5a2(wf_runtime_t*,unsigned,wf_state5_trace_t*);

#define BIT(n) ((uint8_t)(1u<<(n)))

static const uint16_t ba_timer[6][3]={
 {0x004d,0x004d,0x0088},{0x00c3,0x0138,0x01ae},{0x004d,0x0088,0x0088},
 {0x0088,0x0088,0x00c3},{0x0088,0x0088,0x00c3},{0x00c3,0x00fe,0x0138}
};
static const uint8_t id_1e582[12]={0x5c,0x60,0x62,0x5c,0x60,0x60,0x5c,0x64,0x5a,0x60,0x60,0x5e};
static const uint8_t id_1e58e[12]={0x4c,0x4c,0x4e,0x48,0x4e,0x4e,0x48,0x52,0x48,0x48,0x48,0x48};

static int btest(const wf_actor_raw_t *a,size_t o,unsigned bit){return (wf_u8(a,o)&BIT(bit))!=0;}
static int bset(wf_actor_raw_t *a,size_t o,unsigned bit){uint8_t v=wf_u8(a,o),m=BIT(bit);wf_set_u8(a,o,(uint8_t)(v|m));return (v&m)!=0;}
static void bclr(wf_actor_raw_t *a,size_t o,unsigned bit){wf_set_u8(a,o,(uint8_t)(wf_u8(a,o)&(uint8_t)~BIT(bit)));}
static wf_status_t blocked(wf_state5_trace_t *t,uint32_t pc){if(t)t->blocked_dependency_rom=pc;return WF_BLOCKED;}
static wf_actor_raw_t *target(wf_runtime_t *rt,wf_actor_raw_t *a,unsigned *slot){return wf_actor_from_rom_ptr(rt,wf_be32(a,WF_A_TARGET),slot);}
static wf_actor_raw_t *team(wf_runtime_t *rt,wf_actor_raw_t *a,unsigned *slot){return wf_actor_from_rom_ptr(rt,wf_be32(a,WF_A_TEAM_LINK),slot);}
static uint32_t selfptr(unsigned slot){return wf_actor_rom_base(slot);}

wf_status_t wf_r17_candidate_2187e(wf_runtime_t*rt,unsigned slot,uint32_t*candidate,int*found,uint32_t*dependency){
 if(!rt||slot>=WF_ACTOR_SLOTS||!candidate||!found)return WF_BAD_STATE;
 if(dependency)*dependency=0u;
 *candidate=0x1c0f1cu;*found=0;
 wf_actor_raw_t*a=&rt->actors[slot];uint32_t own=selfptr(slot),link=wf_be32(a,WF_A_LINK);
 rt->g.global_1692=0x400u;
 for(unsigned i=4u;i<9u;i++){
  wf_actor_raw_t*c=&rt->actors[i];uint32_t cp=selfptr(i);
  /* CMPA compares the full raw LONG; dereference bus masking is separate. */
  if(cp==own||cp==link||!btest(c,0u,7u)||btest(c,WF_A_CPU_B5,3u)||btest(c,WF_A_CPU_B5,7u)||btest(c,WF_A_FLAGS32,4u))continue;
  wf_actor_raw_t*base=a;
  if(wf_u8(a,WF_A_SELECTOR_LO)==0x48u){
   base=wf_actor_from_rom_ptr(rt,link,NULL);
   if(!base){if(dependency)*dependency=0x218ceu;return WF_BLOCKED;}
   if(btest(base,WF_A_ROLE,5u)==btest(c,WF_A_ROLE,5u))continue;
  }else if(btest(a,WF_A_ROLE,5u)!=btest(c,WF_A_ROLE,5u))continue;
  uint16_t x=wf_be16(base,WF_A_X),cx=wf_be16(c,WF_A_X),y=wf_be16(base,WF_A_Y),cy=wf_be16(c,WF_A_Y);
  uint16_t dx=x>=cx?(uint16_t)(x-cx):(uint16_t)(cx-x),dy=y>=cy?(uint16_t)(y-cy):(uint16_t)(cy-y);
  uint16_t distance=(uint16_t)(dx+dy);
  if(distance>=rt->g.global_1692)continue;
  rt->g.global_1692=distance;rt->g.global_168c=cp;
 }
 if(rt->g.global_1692!=0x400u){
  *candidate=rt->g.global_168c;*found=1;
  wf_actor_raw_t*c=wf_actor_from_rom_ptr(rt,*candidate,NULL);
  (void)bset(c,WF_A_CPU_B5,7u);bclr(c,WF_A_CPU_B6,7u);
 }
 return WF_OK;
}

static wf_status_t reciprocal_link(wf_runtime_t *rt,unsigned slot,wf_actor_raw_t *a,wf_state5_trace_t *tr){
 if(bset(a,WF_A_SELECTOR,6)) return WF_OK;
 wf_actor_raw_t *t=target(rt,a,0); if(!t) return blocked(tr,0x001E6EAu);
 /* $1E6EE copies A1 itself, including any tagged high BYTE. */
 wf_set_be32(a,WF_A_LINK,wf_be32(a,WF_A_TARGET));
 if(wf_be32(t,WF_A_LINK)==0u) wf_set_be32(t,WF_A_LINK,selfptr(slot));
 return WF_OK;
}

/* $1F45A/$1F4CE/$1F51E: exact target offsets before $1D5A2. */
static wf_status_t approach(wf_runtime_t*rt,unsigned slot,wf_actor_raw_t*a,wf_state5_trace_t*tr,unsigned route){
 wf_actor_raw_t*t=target(rt,a,0);if(!t)return blocked(tr,route);
 uint16_t x=wf_be16(t,WF_A_X),y=wf_be16(t,WF_A_Y),d0=route==0x1f4ceu?0xa0u:0x48u,d1=route==0x1f4ceu?0x60u:0x30u;
 int face=btest(t,WF_A_FACING,7);unsigned ts=wf_main_state(t),tsel=wf_u8(t,WF_A_SELECTOR_LO);
 if(route==0x1f51eu)x=(uint16_t)(x+(face?8:-8));
 else{
  wf_r17_m68k_move_w(&rt->m68k_regs,0,d0);wf_r17_m68k_move_w(&rt->m68k_regs,1,d1);
  if(route==0x1f45au&&(btest(t,0x35u,0)||(ts==5u&&tsel==9u))){
   if(ts==5u)x=(uint16_t)(x+((tsel==0x1au)?(face?-32:32):(face?32:-32)));
   y=(uint16_t)(y+(y<0x160u?16:-16));
  }else{
   if(wf_u8(a,WF_A_SELECTOR_LO)==0x3du)y=(uint16_t)(y+16u);
   else if(wf_u8(t,WF_A_STATE4_LO)!=8u&&tsel!=0x61u){y=(uint16_t)(y-24u);d0=d1;wf_r17_m68k_move_w(&rt->m68k_regs,0,d0);}
   x=(uint16_t)(x+(face?-d0:d0));
  }
  wf_r17_m68k_move_w(&rt->m68k_regs,1,x);wf_r17_m68k_move_w(&rt->m68k_regs,2,y);
 }
 wf_set_be16(a,WF_A_CPU_BE,x);wf_set_be16(a,WF_A_CPU_C0,y);wf_set_be16(a,WF_A_STATE,1);wf_set_be16(a,WF_A_AE,10);bclr(a,WF_A_CPU_B5,5);
 return wf_cpu_pursuit_1d5a2(rt,slot,tr);
}
/* $1D6E4 enters the same full $1F45A owner used by state5 callbacks. */
wf_status_t wf_state5_approach_1f45a(wf_runtime_t*rt,unsigned slot,wf_state5_trace_t*tr){
 if(!rt||slot>=WF_ACTOR_SLOTS)return WF_BAD_STATE;
 return approach(rt,slot,&rt->actors[slot],tr,0x1f45au);
}
static wf_status_t cb_1e5ae(wf_runtime_t *rt,unsigned slot,wf_actor_raw_t *a,wf_state5_trace_t *tr){
 if(!btest(a,WF_A_CPU_B5,4)){
  wf_actor_raw_t *t=target(rt,a,0);if(!t)return blocked(tr,0x001E5B8u);
  uint16_t x=wf_be16(t,WF_A_X);
  return approach(rt,slot,a,tr,(x>=0x0200u&&x<0x0300u)?0x001F51Eu:0x001F45Au);
 }
 /* $1E5D4 differs from $1E6E2: first entry also writes A0 into the
  * target's +$7A field before establishing the +$26 links. */
 if(bset(a,WF_A_SELECTOR,6))return WF_OK;
 unsigned ts=0;wf_actor_raw_t *t=target(rt,a,&ts);if(!t)return blocked(tr,0x001E5DEu);
 wf_set_be32(t,WF_A_TARGET,selfptr(slot));
 wf_set_be32(a,WF_A_LINK,selfptr(ts));
 if(wf_be32(t,WF_A_LINK)==0u)wf_set_be32(t,WF_A_LINK,selfptr(slot));
 return WF_OK;
}
static wf_status_t position_gate(wf_runtime_t *rt,unsigned slot,wf_actor_raw_t *a,wf_state5_trace_t *tr,uint16_t left,uint16_t right){
 if(btest(a,WF_A_CPU_B5,4))return reciprocal_link(rt,slot,a,tr);
 wf_actor_raw_t *t=target(rt,a,0);if(!t)return blocked(tr,0x001E600u);
 uint16_t x=wf_be16(t,WF_A_X);int face7=btest(t,WF_A_FACING,7);
 int enter_common=face7?(x<left):(x>=right);
 if(!enter_common)return approach(rt,slot,a,tr,0x1f4ceu);
 wf_set_be16(a,WF_A_SELECTOR,0x000Au); /* $1E624 exact WORD write */
 return approach(rt,slot,a,tr,0x1f45au);      /* $1E664 with +$B5.bit4 clear */
}
static wf_status_t cb_1e62c(wf_runtime_t *rt,unsigned slot,wf_actor_raw_t *a,wf_state5_trace_t *tr){
 if(rt->g.credited_route==0u)return reciprocal_link(rt,slot,a,tr);
 return position_gate(rt,slot,a,tr,0x0250u,0x02A0u);
}
static void state0_profile_prefix(wf_actor_raw_t *a,uint16_t bc){
 wf_set_be16(a,WF_A_CPU_BC,bc); (void)bset(a,WF_A_CPU_B5,3); (void)bset(a,WF_A_CPU_B7,2); (void)bset(a,WF_A_CPU_B7,3); wf_set_be16(a,WF_A_STATE,0u);
}
static wf_status_t state0_profile(wf_runtime_t *rt,unsigned slot,wf_actor_raw_t *a,uint8_t sel,wf_state5_trace_t *tr){
 if(bset(a,WF_A_CPU_B5,4)) return reciprocal_link(rt,slot,a,tr);
 uint16_t bc=(sel==0x01u)?0x0080u:((sel==0x06u||sel==0x2eu||sel==0x31u)?0x0060u:0x0070u);
 wf_set_be16(a,WF_A_CPU_BC,bc);
 if(sel==0x03u){
  wf_actor_raw_t *t=target(rt,a,0); if(!t) return blocked(tr,0x001E71Cu);
  uint8_t id=wf_u8(t,WF_A_INTERNAL_ID);
  if(id!=7u){wf_set_u8(a,WF_A_CPU_BD,(uint8_t)(wf_u8(a,WF_A_CPU_BD)-8u));if(id==8u)wf_set_u8(a,WF_A_CPU_BD,(uint8_t)(wf_u8(a,WF_A_CPU_BD)-8u));}
 }
 state0_profile_prefix(a,wf_be16(a,WF_A_CPU_BC));
 return wf_cpu_continue_1c5b6(rt,slot,tr);
}
static wf_status_t target_flag_transition(wf_runtime_t *rt,wf_actor_raw_t *a,uint8_t sel,wf_state5_trace_t *tr){
 if(bset(a,WF_A_CPU_B5,4)) return WF_OK;
 wf_actor_raw_t *t=target(rt,a,0); if(!t) return blocked(tr,sel==0x72u?0x001E2B2u:0x001E2FEu);
 if(btest(t,WF_A_FLAGS33,4)){
  state0_profile_prefix(a,0x0070u); return wf_cpu_continue_1c5b6(rt,(unsigned)(a-rt->actors),tr);
 }
 if(btest(t,WF_A_FLAGS33,6)){
  uint16_t limit=sel==0x72u?0x0028u:0x0030u;
  if(wf_be16(a,WF_A_CPU_DISTANCE)>=limit){wf_set_be16(a,WF_A_CPU_BC,limit);(void)bset(a,WF_A_CPU_B5,5);wf_set_be16(a,WF_A_STATE,1u);wf_set_be16(a,WF_A_AE,0x000au);}
 }
 return WF_OK;
}
static wf_status_t sustained(wf_actor_raw_t *a,uint8_t sel,wf_state5_trace_t *tr){
 unsigned mode; uint16_t next;
 switch(sel){case 0x5f:mode=2;next=0x005c;break;case 0x52:mode=0;next=0x0058;break;case 0x53:mode=1;next=0x0057;break;case 0x63:mode=4;next=0x0055;break;case 0x5d:mode=2;next=0x0067;break;case 0x60:mode=3;next=0x005b;break;case 0x61:case 0x77:mode=5;next=0x0059;break;case 0x62:mode=4;next=0x005a;break;default:return WF_BAD_STATE;}
 if(!bset(a,WF_A_SELECTOR,6)){
  uint8_t tier=wf_u8(a,WF_A_TIER_LO); if(tier>=3u)return blocked(tr,0x001F55Au);
  wf_set_be16(a,WF_A_CPU_BA,ba_timer[mode][tier]);wf_set_be16(a,WF_A_CPU_BC,next);
 }
 uint16_t v=(uint16_t)(wf_be16(a,WF_A_CPU_BA)-1u);wf_set_be16(a,WF_A_CPU_BA,v);if(v!=0u)return WF_OK;
 if((sel==0x52u||sel==0x53u||sel==0x61u)&&wf_be16(a,WF_A_TIMER44)!=0u){wf_set_be16(a,WF_A_CPU_BA,1u);return WF_OK;}
 wf_set_u8(a,WF_A_STATE,0u); /* CLR.B +$20: phase high byte only */
 wf_set_be16(a,WF_A_SELECTOR,wf_be16(a,WF_A_CPU_BC)); return WF_OK;
}
static uint8_t stamina_bucket(uint16_t s){if(!s)return 8u;if(s>=0x80u)return 0u;if(s>=0x65u)return 1u;if(s>=0x50u)return 2u;if(s>=0x40u)return 3u;if(s>=0x25u)return 4u;return 5u;}
static wf_status_t progress_completion(wf_actor_raw_t *a,uint8_t sel){
 if(!bset(a,WF_A_SELECTOR,6))wf_set_u8(a,WF_A_CPU_BD,stamina_bucket(wf_be16(a,WF_A_STAMINA)));
 if(wf_u8(a,WF_A_PROGRESS109)==wf_u8(a,WF_A_CPU_BD)){
  wf_set_u8(a,WF_A_STATE,0u); /* exact CLR.B +$20 */
  wf_set_be16(a,WF_A_SELECTOR,(sel==0x4au||sel==0x64u)?0x004bu:0x0056u);
 }
 return WF_OK;
}
static wf_status_t timer_setup(wf_runtime_t *rt,wf_actor_raw_t *a,uint8_t sel,wf_state5_trace_t *tr){
 wf_actor_raw_t *t=target(rt,a,0); if(!t)return blocked(tr,0x001E568u); uint8_t id=wf_u8(t,WF_A_INTERNAL_ID); if(id>=12u)return WF_BAD_STATE;
 if(sel==0x2du||sel==0x20u){wf_r17_m68k_moveq(&rt->m68k_regs,0,0);wf_r17_m68k_move_b(&rt->m68k_regs,0,id);}
 switch(sel){
  case 0x2d:wf_set_be16(a,WF_A_CPU_BC,0u);wf_set_u8(a,WF_A_CPU_BD,id_1e58e[id]);break;
  case 0x20:wf_set_be16(a,WF_A_CPU_BC,0u);wf_set_u8(a,WF_A_CPU_BD,id_1e582[id]);break;
  case 0x41:wf_set_be16(a,WF_A_CPU_BC,0x005cu);if(id==7u)wf_set_u8(a,WF_A_CPU_BD,(uint8_t)(wf_u8(a,WF_A_CPU_BD)+4u));break;
  case 0x2a:wf_set_be16(a,WF_A_CPU_BC,0x0060u);break;
  case 0x04:case 0x40:wf_set_be16(a,WF_A_CPU_BC,0x0070u);break;
  case 0x05:wf_set_be16(a,WF_A_CPU_BC,0x0078u);if(id==7u)wf_set_u8(a,WF_A_CPU_BD,0x80u);if(wf_u8(a,WF_A_INTERNAL_ID)==8u)wf_set_u8(a,WF_A_CPU_BD,(uint8_t)(wf_u8(a,WF_A_CPU_BD)-6u));break;
  case 0x45:wf_set_be16(a,WF_A_CPU_BC,0x00a8u);break;
  default:return WF_BAD_STATE;
 }
 if(!btest(t,WF_A_FLAGS33,4))wf_set_be16(a,WF_A_CPU_BC,(uint16_t)(wf_be16(a,WF_A_CPU_BC)-0x0022u));
 (void)bset(a,WF_A_FLAGS33,4);return WF_OK;
}
static wf_status_t cb_1e42a(wf_runtime_t *rt,wf_actor_raw_t *a,wf_state5_trace_t *tr){
 /* $10BE8 is pure orientation/facing update and is reproduced from WP03. */
 wf_actor_raw_t *t=target(rt,a,0);if(!t)return blocked(tr,0x0010BE8u);
 uint16_t tx=wf_be16(t,WF_A_X);uint8_t ts=wf_main_state(t);uint16_t off=0;
 if(ts==4u){uint8_t sub=wf_state4_subtype(t);if(sub==8u)off=0x40u;else if(sub==9u)off=0x10u;}
 else if(ts==5u){uint8_t s=wf_u8(t,WF_A_SELECTOR_LO);if(s==0x22u)off=0x30u;else if(s==0x52u)off=0x10u;}
 if(off)tx=(uint16_t)(tx+((wf_u8(t,WF_A_FACING)&0x80u)?(uint16_t)(0u-off):off));
 if(tx<wf_be16(a,WF_A_X))bclr(a,WF_A_FACING,7);else(void)bset(a,WF_A_FACING,7);
 if(!btest(t,WF_A_FLAGS33,4)||wf_u8(a,WF_A_FACING)==wf_u8(t,WF_A_FACING))wf_set_be16(a,WF_A_STATE,0u);
 return WF_OK;
}
static wf_status_t cb_link49(wf_runtime_t *rt,unsigned slot,wf_actor_raw_t *a,wf_state5_trace_t *tr){
 if(bset(a,WF_A_SELECTOR,6)) return WF_OK;
 unsigned ts;
 wf_actor_raw_t *t=target(rt,a,&ts);
 if(!t) return blocked(tr,0x001E682u);
 wf_set_be32(a,WF_A_LINK,selfptr(ts));
 if(wf_be32(t,WF_A_LINK)==0u) wf_set_be32(t,WF_A_LINK,selfptr(slot));
 else if(wf_be32(t,WF_A_SECONDARY_LINK)==0u) wf_set_be32(t,WF_A_SECONDARY_LINK,selfptr(slot));
 return WF_OK;
}
static wf_status_t distance_or_link(wf_runtime_t *rt,unsigned slot,wf_actor_raw_t *a,uint8_t sel,wf_state5_trace_t *tr){
 int old=bset(a,WF_A_CPU_B5,4);
 if(!old && wf_be16(a,WF_A_CPU_DISTANCE)>=0x0028u){wf_set_be16(a,WF_A_CPU_BC,0x0028u);wf_set_be16(a,WF_A_STATE,1u);wf_set_be16(a,WF_A_AE,0x000au);(void)bset(a,WF_A_CPU_B5,5);return wf_cpu_pursuit_1d5a2(rt,slot,tr);}
 if(bset(a,WF_A_SELECTOR,6))return WF_OK;
 uint32_t target_ptr=wf_be32(a,WF_A_TARGET);
 wf_actor_raw_t*t=target(rt,a,NULL);if(!t)return blocked(tr,0x001E7BEu);
 if((sel==0x30u||sel==0x2bu)&&wf_main_state(t)==4u&&wf_state4_subtype(t)==1u)wf_set_u8(a,WF_A_TIMER46,(uint8_t)(wf_u8(a,WF_A_TIMER46)+1u));
 wf_set_be32(a,WF_A_LINK,target_ptr);
 if(wf_be32(t,WF_A_LINK))return WF_OK;
 wf_set_be32(t,WF_A_LINK,selfptr(slot));
 if(sel==0x1du){
  if((rt->g.mode_0161&1u)==0u){if(btest(t,WF_A_FLAGS33,0))return WF_OK;wf_set_u8(a,WF_A_SELECTOR_LO,0x1cu);return WF_OK;}
  uint32_t candidate,dependency=0;int found;
  wf_status_t st=wf_r17_candidate_2187e(rt,slot,&candidate,&found,&dependency);
  if(st!=WF_OK)return blocked(tr,dependency);
  if(found){wf_actor_raw_t*c=wf_actor_from_rom_ptr(rt,candidate,NULL);wf_set_be32(c,WF_A_RESERVE,wf_be32(a,WF_A_LINK));wf_set_be32(a,WF_A_RESERVE,candidate);}
  else wf_set_u8(a,WF_A_SELECTOR_LO,0x1cu);
  return WF_OK;
 }
 return WF_OK;
}
/* $1E9A6: the special no-credit character restarts controller16;
 * every other route enters the shared $1F084 decision with bank9. */
static wf_status_t hold_tail_1e9a6(wf_runtime_t*rt,unsigned slot,wf_actor_raw_t*a,wf_state5_trace_t*tr){
 if(!rt->g.credited_route&&wf_u8(a,WF_A_INTERNAL_ID)==7u){
  bclr(a,WF_A_CPU_B5,4);wf_set_be16(a,WF_A_STATE,5u);wf_set_be16(a,WF_A_SELECTOR,0x16u);
  return wf_state5_execute(rt,slot,tr);
 }
 return wf_cpu_decide_1f084(rt,slot,9u,tr);
}
/* $24CC with a proven two-byte distribution. D1/D2 are MOVEM-preserved;
 * each rejected sample consumes the original live-register $21B4 call. */
static int controller15_weighted(wf_runtime_t*rt,const uint8_t weights[2]){
 uint32_t d1=rt->m68k_regs.d[1],d2=rt->m68k_regs.d[2];
 uint16_t sample=50u;unsigned accepted=0;
 if(!rt->m68k_regs.valid||!rt->g.rng_state_valid)return -1;
 for(int attempt=3;attempt>=0;attempt--){
  wf_r17_m68k_moveq(&rt->m68k_regs,1u,(int8_t)attempt);
  uint32_t value=wf_r17_rng21b4_live(&rt->g.rng_state,rt->g.frame_counter,&rt->m68k_regs);
  sample=(uint16_t)((value&255u)>>1);
  if(sample<100u){accepted=1;break;}
 }
 if(!accepted)sample=50u;
 if(!sample)sample=1u;
 wf_r17_m68k_move_l(&rt->m68k_regs,1u,d1);wf_r17_m68k_move_l(&rt->m68k_regs,2u,d2);
 unsigned choice=weights[0]>=sample?0u:1u;
 if(choice&&(unsigned)(uint8_t)(weights[0]+weights[1])<sample)return -1;
 wf_r17_m68k_move_l(&rt->m68k_regs,0u,choice);return (int)choice;
}
static wf_status_t controller15_credited(wf_runtime_t*rt,unsigned slot,wf_actor_raw_t*a,wf_state5_trace_t*tr){
 /* Exact ROM tables $1E9D6/$1E9FA, progression offsets $1EA1E and
  * special character5 distributions $1EA28. */
 static const uint8_t first[36]={15,85,10,90,5,95,20,80,15,85,10,90,25,75,20,80,15,85,25,75,20,80,15,85,25,75,25,75,20,80,30,70,25,75,25,75};
 static const uint8_t second[36]={5,95,15,85,20,80,15,85,20,80,30,70,20,80,25,75,35,65,20,80,25,75,35,65,25,75,35,65,40,60,20,80,30,70,35,65};
 static const uint8_t offsets[10]={0,6,12,18,24,30,30,30,30,30};
 static const uint8_t special[6]={0,100,20,80,100,0};
 wf_actor_raw_t*p=team(rt,a,0);if(!p)return blocked(tr,0x1e878u);
 if(!btest(p,WF_A_FLAGS32,0)||btest(p,WF_A_FLAGS34,0))return hold_tail_1e9a6(rt,slot,a,tr);
 if(wf_u8(a,WF_A_INTERNAL_ID)==5u){
  uint16_t base=btest(a,WF_A_FLAGS33,7)?0x2d0u:0x200u,x=wf_be16(a,WF_A_X);
  uint16_t distance=(uint16_t)(base-x);if(base<x)distance=(uint16_t)(0u-distance);
  wf_r17_m68k_move_w(&rt->m68k_regs,0u,distance);
  if(distance<0x94u){
   wf_actor_raw_t*l=wf_actor_from_rom_ptr(rt,wf_be32(a,WF_A_LINK),0);
   if(!l)return blocked(tr,0x1e8bcu);
   wf_r17_m68k_moveq(&rt->m68k_regs,0u,0);
   uint16_t tier=wf_be16(l,WF_A_TIER);if(rt->g.global_16a0<0x10u)tier=2u;
   wf_r17_m68k_move_w(&rt->m68k_regs,0u,tier);
   unsigned at=(uint16_t)(tier<<1);wf_r17_m68k_move_w(&rt->m68k_regs,0u,(uint16_t)at);
   if(at>sizeof special-2u)return blocked(tr,0x1e8d8u);
   int selected=controller15_weighted(rt,special+at);if(selected<0)return blocked(tr,0x24ccu);
   if(!selected){
    (void)bset(a,WF_A_CPU_B5,4);wf_set_be16(a,WF_A_STATE,5u);wf_set_be16(a,WF_A_SELECTOR,0x16u);
    return wf_state5_execute(rt,slot,tr);
   }
  }
 }
 unsigned position=wf_u8(a,0x31u),side=btest(a,WF_A_FLAGS33,7);
 const uint8_t*weights=side?((position==9u||position<7u)?second:first):((position==2u||position>=5u)?second:first);
 uint16_t progression=rt->g.progression_0162;
 wf_r17_m68k_move_w(&rt->m68k_regs,0u,progression);wf_r17_m68k_moveq(&rt->m68k_regs,1u,0);
 if(progression>=sizeof offsets)return blocked(tr,0x1e93eu);
 wf_r17_m68k_move_b(&rt->m68k_regs,1u,offsets[progression]);
 uint8_t tier_offset=(uint8_t)(wf_u8(a,WF_A_TIER_LO)<<1);
 wf_r17_m68k_move_b(&rt->m68k_regs,1u,wf_u8(a,WF_A_TIER_LO));
 wf_r17_m68k_move_b(&rt->m68k_regs,1u,tier_offset);
 unsigned at=(unsigned)offsets[progression]+tier_offset;
 if(at>sizeof first-2u)return blocked(tr,0x1e94cu);
 int selected=controller15_weighted(rt,weights+at);if(selected<0)return blocked(tr,0x24ccu);
 if(selected)return hold_tail_1e9a6(rt,slot,a,tr);
 bclr(a,WF_A_CPU_B5,4);wf_set_be16(a,WF_A_STATE,5u);wf_set_be16(a,WF_A_SELECTOR,0x16u);
 return wf_state5_execute(rt,slot,tr);
}
static wf_status_t controller15(wf_runtime_t*rt,unsigned slot,wf_actor_raw_t *a,wf_state5_trace_t *tr){
 if(!bset(a,WF_A_SELECTOR,6)){wf_set_be16(a,WF_A_CPU_BC,0x0010u);wf_set_be16(a,WF_A_TIMER46,0u);}
 uint16_t v=(uint16_t)(wf_be16(a,WF_A_CPU_BC)-1u);wf_set_be16(a,WF_A_CPU_BC,v);if(v)return WF_OK;
 /* $1E854-$1E99E: normal TAG route, first sequence bank. */
 if(rt->g.mode_0161&1u){
  /* $1E96E: Royal low-Y actors sample literal $1E9D4={60,40}.
   * The zero choice sets B6.bit3 before the original controller16 reentry. */
  static const uint8_t weights[2]={60u,40u};
  if(wf_be16(a,WF_A_Y)>=0x150u)return hold_tail_1e9a6(rt,slot,a,tr);
  int selected=controller15_weighted(rt,weights);if(selected<0)return blocked(tr,0x24ccu);
  if(selected)return hold_tail_1e9a6(rt,slot,a,tr);
  (void)bset(a,WF_A_CPU_B6,3u);wf_set_be16(a,WF_A_STATE,5u);wf_set_be16(a,WF_A_SELECTOR,0x16u);
  return wf_state5_execute(rt,slot,tr);
 }
 if(rt->g.mode_0161&2u)return hold_tail_1e9a6(rt,slot,a,tr);
 if(rt->g.credited_route&0x8000u)return controller15_credited(rt,slot,a,tr);
 if(rt->g.global_0166&255u)return hold_tail_1e9a6(rt,slot,a,tr);
 bclr(a,WF_A_CPU_B5,4);wf_set_be16(a,WF_A_STATE,5u);wf_set_be16(a,WF_A_SELECTOR,0x16u);
 return wf_state5_execute(rt,slot,tr);
}
static wf_status_t controller16(wf_runtime_t *rt,unsigned slot,wf_actor_raw_t *a,wf_state5_trace_t *tr){
 /* $1EB48 special path is fully numeric: when $1C007C==0 and internal ID 7,
  * first entry writes angle byte $40, then waits for X >= $0308 before
  * entering the shared decision tail at $1E9CC. */
 if(rt->g.credited_route==0u && wf_u8(a,WF_A_INTERNAL_ID)==7u){
  if(!bset(a,WF_A_SELECTOR,6)) wf_set_u8(a,WF_A_ANGLE,0x40u);
  if(wf_be16(a,WF_A_X)<0x0308u) return WF_OK;
  return wf_cpu_decide_1f084(rt,slot,9u,tr);
 }
 if(btest(a,WF_A_CPU_B6,3)){
  if(!btest(a,WF_A_SELECTOR,6)){(void)bset(a,WF_A_SELECTOR,6);wf_set_u8(a,WF_A_ANGLE,0u);wf_set_be16(a,WF_A_CPU_C0,(uint16_t)(wf_be16(a,WF_A_Y)+0x0030u));}
  uint16_t goal=wf_be16(a,WF_A_CPU_C0);wf_r17_m68k_move_w(&rt->m68k_regs,0u,goal);
  if(wf_be16(a,WF_A_Y)>goal){bclr(a,WF_A_CPU_B6,3);return hold_tail_1e9a6(rt,slot,a,tr);}return WF_OK;
 }
 if(!bset(a,WF_A_SELECTOR,6)){
  wf_set_be16(a,WF_A_CPU_BC,0x0080u);
  if(!btest(a,WF_A_CPU_B5,4)){if(btest(a,WF_A_FLAGS33,7)){wf_set_be16(a,WF_A_CPU_BE,0x02f8u);wf_set_be16(a,WF_A_CPU_C0,0x0128u);}else{wf_set_be16(a,WF_A_CPU_BE,0x0210u);wf_set_be16(a,WF_A_CPU_C0,0x0190u);}}
  else {if(btest(a,WF_A_FLAGS33,7)){wf_set_be16(a,WF_A_CPU_BE,0x02d0u);wf_set_be16(a,WF_A_CPU_C0,0x0178u);}else{wf_set_be16(a,WF_A_CPU_BE,0x0200u);wf_set_be16(a,WF_A_CPU_C0,0x0130u);}}
 }
 /* Reuse $1F15C/$20C8, including the original shared $15E4 scratch. */
 if(!wf_cpu_arrived_1f15c(rt,a)){
  uint16_t v=(uint16_t)(wf_be16(a,WF_A_CPU_BC)-1u);wf_set_be16(a,WF_A_CPU_BC,v);
  return v?WF_OK:hold_tail_1e9a6(rt,slot,a,tr);
 }
 wf_set_be16(a,WF_A_STATE,5u);
 if(!btest(a,WF_A_CPU_B5,4)){
  unsigned ps;wf_actor_raw_t*p=team(rt,a,&ps);if(btest(a,WF_A_FLAGS33,0)&&p&&btest(p,WF_A_FLAGS32,0)){wf_set_be16(a,WF_A_SELECTOR,0x0066u);return WF_OK;}return hold_tail_1e9a6(rt,slot,a,tr);
 }
 unsigned ps;wf_actor_raw_t*p=team(rt,a,&ps);if(!p)return blocked(tr,0x001EAEEu);
 if(btest(a,WF_A_FLAGS33,7)){wf_set_be16(a,WF_A_X,0x02d0u);wf_set_be16(a,WF_A_Y,0x0178u);}else{wf_set_be16(a,WF_A_X,0x0200u);wf_set_be16(a,WF_A_Y,0x0130u);}
 wf_set_be16(a,WF_A_SELECTOR,0x0089u);wf_set_be16(p,WF_A_STATE,5u);wf_set_be16(p,WF_A_SELECTOR,0x0088u);(void)ps;return WF_OK;
}

static wf_status_t controller3e(wf_runtime_t *rt,wf_actor_raw_t *a,wf_state5_trace_t *tr){
 wf_actor_raw_t *t=target(rt,a,0);
 if(!t) return blocked(tr,0x001EB62u);
 int qualifying=(wf_main_state(t)==4u&&wf_state4_subtype(t)==1u)||(wf_main_state(t)==5u&&(wf_u8(t,WF_A_SELECTOR_LO)==0x53u||wf_u8(t,WF_A_SELECTOR_LO)==0x52u));
 if(!qualifying){wf_set_be16(a,WF_A_STATE,0u);rt->g.global_1697|=1u;return WF_OK;}
 if(wf_be16(a,WF_A_CPU_DISTANCE)>=0x0080u){wf_set_be16(a,WF_A_STATE,2u);wf_set_u8(a,WF_A_ANGLE,btest(a,WF_A_FACING,7)?0x40u:0xc0u);bclr(a,WF_A_CPU_B7,1);return WF_OK;}
 wf_set_be16(a,WF_A_CONTACT,0u);
 wf_status_t status=wf_r17_resolver_280dc(rt,a);if(status!=WF_OK)return blocked(tr,0x280dcu);
 if(wf_u8(a,WF_A_CONTACT+1u)){wf_set_be16(a,WF_A_STATE,0);rt->g.global_1697|=1u;}return WF_OK;
}
static wf_status_t controller3d(wf_runtime_t *rt,unsigned slot,wf_actor_raw_t *a,wf_state5_trace_t *tr){
 if((rt->g.mode_0161&2u)!=0u && !btest(a,WF_A_REL_FLAGS,7) && !btest(a,WF_A_ROLE,5) && btest(a,WF_A_FLAGS33,0) && rt->g.global_169b>=0x0au){
  (void)bset(a,WF_A_CPU_B5,5);wf_set_be16(a,WF_A_SELECTOR,0x0069u);wf_set_be16(a,WF_A_CPU_BE,0x0310u);wf_set_be16(a,WF_A_CPU_C0,0x0138u);wf_set_be16(a,WF_A_STATE,1u);wf_set_be16(a,WF_A_AE,0x000au);return WF_OK;
 }
 wf_actor_raw_t *t=target(rt,a,0);if(!t)return blocked(tr,0x001EC24u);
 if(btest(t,WF_A_FLAGS32,4)){wf_set_be16(a,WF_A_STATE,0u);rt->g.global_1697|=1u;return WF_OK;}
 if(btest(a,WF_A_FLAGS34,1)){
  int carry=0;wf_status_t status=wf_cpu_tag_request_1c5fa(rt,slot,&carry,tr);
  if(status!=WF_OK||!carry)return status;
 }
 if(btest(a,WF_A_CPU_B5,6)){
  wf_actor_raw_t *p=team(rt,a,0);
  if(p && !btest(p,WF_A_FLAGS34,0) && btest(p,WF_A_FLAGS32,0)){
   if(wf_be16(t,WF_A_STAMINA)==0u)bclr(a,WF_A_CPU_B5,6);
   else {wf_set_be16(a,WF_A_STATE,1u);wf_set_be16(a,WF_A_AE,0x0008u);return WF_OK;}
  }
 }
 /* $1EC80 sends a defending owner directly to the existing state0 tail. */
 if(btest(a,WF_A_CPU_B5,7)){wf_set_be16(a,WF_A_STATE,0u);rt->g.global_1697|=1u;return WF_OK;}
 if(!btest(a,WF_A_CPU_B7,6)){
  if(!btest(a,WF_A_CPU_B5,4))return approach(rt,slot,a,tr,0x1f4ceu);
  if(wf_main_state(t)==4u&&(wf_state4_subtype(t)==8u||wf_state4_subtype(t)==9u))return WF_OK;
  wf_set_be16(a,WF_A_STATE,0u);rt->g.global_1697|=1u;return WF_OK;
 }
 /* $1EC94-$1EDC2: wait while the current target action still owns pursuit. */
 if(!btest(a,WF_A_FLAGS33,2)&&btest(t,WF_A_FLAGS33,2))return WF_OK;
 unsigned state=wf_main_state(t),tsel=wf_u8(t,WF_A_SELECTOR_LO);
 if(state==4u){unsigned sub=wf_state4_subtype(t);if(sub!=8u&&sub!=9u)return WF_OK;}
 else if(state==5u){
  if(tsel==0x4cu||tsel==0x4du||tsel==0x0du||tsel==0x0cu||tsel==0x12u||tsel==0x11u||tsel==0x0fu||tsel==0x1bu||btest(t,WF_A_FLAGS33,6))return WF_OK;
 }else if(state!=0u&&state!=1u){if(btest(t,WF_A_FLAGS33,6)||btest(t,WF_A_FLAGS32,1))return WF_OK;}
 wf_set_be16(a,WF_A_STATE,0u);rt->g.global_1697|=1u;return WF_OK;
}

/* Forced raw79 callback. $1DFE2 supplies D0=$1E27A, so $10C60
 * indexes $10CEA with signed WORD $A76B+tier and reads $B455..$B457.
 * These are original ROM bytes, not the usual small-argument recovery row. */
static wf_status_t cb_1e27a(wf_runtime_t*rt,wf_actor_raw_t*a,wf_state5_trace_t*tr){
 if(!btest(a,WF_A_CPU_B5,7u))return WF_OK;
 bclr(a,WF_A_FLAGS34,4u);wf_set_be16(a,WF_A_STATE,0u);
 wf_actor_raw_t*t=target(rt,a,NULL);if(!t)return blocked(tr,0x1e292u);
 /* $1E28E does not restore A0: $1C1E8 advances this target pointer. */
 if(tr)tr->returned_a0=wf_be32(a,WF_A_TARGET);
 wf_set_be16(t,0xaau,0u);
 unsigned tier=wf_be16(t,WF_A_TIER);
 if(tier>=3u){
  /* Preserve the in-callee MULU/ADD.W prefix at the first unowned read. */
  wf_r17_m68k_move_l(&rt->m68k_regs,0u,0x20000u|(uint16_t)(0xa76bu+tier));
  return blocked(tr,0x10ca8u);
 }
 static const uint8_t recovery_b455[3]={0x00u,0x60u,0x04u};
 wf_set_be16(t,0xaau,(uint16_t)((uint16_t)recovery_b455[tier]-1u));
 wf_set_be16(t,0x9au,0u);
 if(btest(t,0x56u,7u)&&(btest(t,WF_A_FLAGS33,5u)||btest(t,WF_A_FLAGS33,2u)))wf_set_be16(t,0x9au,0x30u);
 else wf_set_be16(t,0x9au,tier==2u?0xe0u:tier==1u?0x80u:0x30u);
 return WF_OK;
}

static wf_status_t execute_callback(wf_runtime_t *rt,unsigned slot,wf_actor_raw_t *a,uint8_t sel,uint32_t cb,wf_state5_trace_t *tr){
 switch(cb){
  case 0x01e254u:return WF_OK;
  case 0x01e256u:(void)bset(a,WF_A_CPU_B7,7);return WF_OK;
  case 0x01e25eu:{int old=bset(a,WF_A_SELECTOR,6);if(!old&&wf_u8(a,WF_A_TIMER44+1u)!=0u)wf_set_u8(a,WF_A_TIMER44+1u,1u);return WF_OK;}
  case 0x01e274u:wf_set_be16(a,0x108u,0u);return WF_OK;
  case 0x01e27au:return cb_1e27a(rt,a,tr);
  case 0x01e2a8u:case 0x01e2f6u:return target_flag_transition(rt,a,sel,tr);
  case 0x01e334u:case 0x01e34eu:case 0x01e368u:case 0x01e382u:case 0x01e39cu:case 0x01e3b2u:case 0x01e3c8u:case 0x01e3deu:return sustained(a,sel,tr);
  case 0x01e42au:return cb_1e42a(rt,a,tr);
  case 0x01e452u:return progress_completion(a,sel);
  case 0x01e4dau:case 0x01e4f6u:case 0x01e512u:case 0x01e52au:case 0x01e532u:case 0x01e53au:case 0x01e562u:return timer_setup(rt,a,sel,tr);
  case 0x01e59au:(void)bset(a,WF_A_CPU_B5,3);bclr(a,WF_A_CPU_B7,2);bclr(a,WF_A_CPU_B7,3);return WF_OK;
  case 0x01e5aeu:return cb_1e5ae(rt,slot,a,tr);
  case 0x01e5f6u:return position_gate(rt,slot,a,tr,0x0260u,0x0290u);
  case 0x01e62cu:return cb_1e62c(rt,slot,a,tr);
  case 0x01e664u:if(!btest(a,WF_A_CPU_B5,4))return approach(rt,slot,a,tr,0x1f45au);return reciprocal_link(rt,slot,a,tr);
  case 0x01e670u:if(!btest(a,WF_A_CPU_B5,4))return approach(rt,slot,a,tr,0x1f51eu);return reciprocal_link(rt,slot,a,tr);
  case 0x01e67cu:return cb_link49(rt,slot,a,tr);
  case 0x01e6a4u:{if(btest(a,WF_A_CPU_B5,4))return reciprocal_link(rt,slot,a,tr);wf_actor_raw_t*t=target(rt,a,0);if(!t)return blocked(tr,0x001E6AAu);if(!(((rt->g.mode_0161&2u)==0u)&&rt->g.credited_route!=0u&&btest(t,WF_A_FLAGS33,0)&&btest(a,WF_A_FLAGS33,0)))wf_set_u8(a,WF_A_SELECTOR_LO,0x0au);return approach(rt,slot,a,tr,0x1f45au);}
  case 0x01e6dcu:(void)bset(a,WF_A_CPU_B5,4);return reciprocal_link(rt,slot,a,tr);
  case 0x01e6e2u:return reciprocal_link(rt,slot,a,tr);
  case 0x01e6feu:case 0x01e70eu:case 0x01e73eu:case 0x01e74eu:return state0_profile(rt,slot,a,sel,tr);
  case 0x01e792u:return distance_or_link(rt,slot,a,sel,tr);
  case 0x01e83au:return controller15(rt,slot,a,tr);
  case 0x01ea2eu:return controller16(rt,slot,a,tr);
  case 0x01eb62u:return controller3e(rt,a,tr);
  case 0x01ebceu:return controller3d(rt,slot,a,tr);
  default:return blocked(tr,cb);
 }
}

wf_status_t wf_state5_execute(wf_runtime_t *rt,unsigned slot,wf_state5_trace_t *tr){
 if(tr) memset(tr,0,sizeof(*tr));
 if(!rt||slot>=WF_ACTOR_SLOTS) return WF_BAD_STATE;
 wf_actor_raw_t *a=&rt->actors[slot];
 bclr(a,WF_A_CPU_B9,1);
 uint8_t sel=wf_u8(a,WF_A_SELECTOR_LO);
 const wf_state5_map_t *m=wf_state5_lookup(sel);
 if(!m) return WF_BAD_STATE;
 /* $1DFE8-$1DFFC leaves the callback address, with bit 31 cleared, in D0. */
 wf_r17_m68k_move_l(&rt->m68k_regs,0u,m->callback_rom);
 if(tr){tr->selector=sel;tr->callback_rom=m->callback_rom;tr->forced=m->forced_bit31;}
 int call=m->forced_bit31||wf_u8(a,WF_A_STATE+1u)!=5u||!btest(a,WF_A_STATE,7);
 if(!call) return WF_OK;
 if(tr) tr->invoked=1u;
 return execute_callback(rt,slot,a,sel,m->callback_rom,tr);
}
