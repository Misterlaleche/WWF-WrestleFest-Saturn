#include <string.h>
#include "wf_cpu_runtime.h"
#include "arcade_cpu_final_bridge.h"
#include "wf_r17_resolver_280dc.h"
#include "../generated/wf_cpu_decision_data.h"
static wf_status_t block(wf_cpu_trace_t*t,uint32_t pc){if(t)t->blocked_dependency_rom=pc;return WF_BLOCKED;}
static wf_actor_raw_t *ptr(wf_runtime_t*rt,const wf_actor_raw_t*a,size_t off){return wf_actor_from_rom_ptr(rt,wf_be32(a,off),0);}
/* Direct-ROM $1C2CC fast routes reached by the fresh TAG participants. Other
 * branches remain explicit dependencies instead of guessing AI behavior. */
static wf_status_t knockdown_1cbf8(wf_runtime_t*,unsigned,wf_actor_raw_t*,wf_cpu_trace_t*);
wf_status_t wf_cpu_pursuit_1d5a2(wf_runtime_t*,unsigned,wf_state5_trace_t*);
static wf_status_t state0_tail(wf_runtime_t*,unsigned,wf_actor_raw_t*,wf_cpu_trace_t*);
static wf_status_t choose_1dea6(wf_runtime_t*,unsigned,wf_actor_raw_t*,uint32_t,unsigned,wf_cpu_trace_t*);
static wf_status_t decision_1de66(wf_runtime_t*,unsigned,wf_actor_raw_t*,unsigned,wf_cpu_trace_t*);
static wf_status_t assist_1f5b2(wf_runtime_t*,unsigned,wf_actor_raw_t*,wf_cpu_trace_t*);
static wf_status_t state0_defender_prefix(wf_runtime_t*,wf_actor_raw_t*,wf_cpu_trace_t*);
static wf_status_t state0(wf_runtime_t*rt,unsigned slot,wf_actor_raw_t*a,wf_cpu_trace_t*tr){
 if(!(wf_u8(a,WF_A_STATE)&0x80u))return WF_OK;
 if((wf_u8(a,WF_A_CPU_B5)&128u)||(wf_u8(a,WF_A_FLAGS34)&4u))return state0_defender_prefix(rt,a,tr);
 wf_actor_raw_t*t=ptr(rt,a,WF_A_TARGET);if(!t)return block(tr,0x00010be8u);
 uint8_t f33=wf_u8(a,WF_A_FLAGS33),tf33=wf_u8(t,WF_A_FLAGS33);
 uint8_t f34=wf_u8(a,WF_A_FLAGS34),b5=wf_u8(a,WF_A_CPU_B5),b7=wf_u8(a,WF_A_CPU_B7);
 if(rt->g.mode_0161||wf_u8(a,WF_A_CPU_B6)&0x10u||f34&0x14u||b7&0x91u||b5&0x80u||wf_u8(a,WF_A_CPU_B9)&1u||wf_u8(t,WF_A_FLAGS34)&0x10u)return block(tr,0x0001c2ccu);
 /* $10BE8: target position and special state can offset the facing test. */
 uint16_t x=wf_be16(t,WF_A_X),off=0;uint8_t ts=wf_u8(t,WF_A_STATE+1u);
 if(ts==4u){uint8_t sub=wf_u8(t,WF_A_STATE4_LO);off=sub==8u?0x40u:sub==9u?0x10u:0u;}
 if(ts==5u){uint8_t sel=wf_u8(t,WF_A_SELECTOR_LO);off=sel==0x22u?0x30u:sel==0x52u?0x10u:0u;}
 x=(uint16_t)(x+((wf_u8(t,WF_A_FACING)&0x80u)?-off:off));
 uint8_t facing=wf_u8(a,WF_A_FACING);wf_set_u8(a,WF_A_FACING,(uint8_t)((facing&0x7fu)|(x>=wf_be16(a,WF_A_X)?0x80u:0u)));
 wf_set_be16(a,WF_A_AE,0u);wf_set_be16(a,WF_A_TIMER44,0u);wf_set_be16(a,WF_A_TIMER46,0u);
 wf_set_be16(a,WF_A_FLAGS32,(uint16_t)(wf_be16(a,WF_A_FLAGS32)&0xfba7u));
 wf_set_u8(a,WF_A_CPU_B5,(uint8_t)(b5&0xcbu));
 wf_set_be16(a,WF_A_CPU_B6,(uint16_t)(wf_be16(a,WF_A_CPU_B6)&0xf1fdu));
 wf_set_u8(a,WF_A_CPU_B7,(uint8_t)(b7&~0x20u));
 if((f33&1u)&&!(tf33&1u)&&!(wf_u8(t,WF_A_FLAGS32)&1u)){
  rt->g.global_1697|=1u;return WF_OK;
 }
 if((f33&1u)&&(wf_u8(t,WF_A_FLAGS32)&1u)){
  rt->g.global_1697|=1u;return WF_OK;
 }
 if(b5&8u)return state0_tail(rt,slot,a,tr);
 if(f34&2u){
  if(f33&1u)return block(tr,0x0001c5fau);
  wf_set_be16(a,WF_A_STATE,1u);wf_set_be16(a,WF_A_AE,4u);return WF_OK;
 }
 if((f33&1u)&&!(wf_u8(t,WF_A_FLAGS32)&1u)&&!(wf_u8(a,WF_A_ROLE)&0x40u)&&
    !(f33&4u)&&!(b5&0x40u)){
  /* $1F3D6, $1C7E2 and $1C880: no tagged/ringside branch in stage zero. */
  if(wf_u8(a,WF_A_STATE+1u)==4u&&wf_u8(a,WF_A_STATE4_LO)==1u)return block(tr,0x0001f422u);
  wf_actor_raw_t*partner=ptr(rt,a,WF_A_TEAM_LINK);if(!partner)return block(tr,0x0001f43eu);
  uint16_t d0=(uint16_t)(wf_be16(a,WF_A_STAMINA)+12u);
  wf_r17_m68k_move_w(&rt->m68k_regs,0u,d0);
  if(d0<wf_be16(partner,WF_A_STAMINA))wf_set_u8(a,WF_A_CPU_B5,(uint8_t)(wf_u8(a,WF_A_CPU_B5)|0x40u));
  else wf_set_u8(a,WF_A_CPU_B5,(uint8_t)(wf_u8(a,WF_A_CPU_B5)&~0x40u));
  if(ts==4u&&!(wf_u8(t,WF_A_STATE4_LO)==6u||wf_u8(t,WF_A_STATE4_LO)==7u||wf_u8(t,WF_A_STATE4_LO)==8u||wf_u8(t,WF_A_STATE4_LO)==9u))
   wf_set_u8(a,WF_A_CPU_B6,(uint8_t)(wf_u8(a,WF_A_CPU_B6)&~1u));
  rt->g.global_1697|=1u;wf_set_be16(a,WF_A_SELECTOR,0u);wf_set_be16(a,WF_A_STATE,1u);wf_set_be16(a,WF_A_AE,0u);return WF_OK;
 }
 return state0_tail(rt,slot,a,tr);
}
/* $24CC calls $21B4 with D1=3,2,1,0, keeping D1/D2 from its caller.
 * The caller's live D registers are required by the ROM RNG. */
static int state1_weighted_24cc(wf_runtime_t*rt,const uint8_t *weights,size_t count){
 uint32_t d1=rt->m68k_regs.d[1],d2=rt->m68k_regs.d[2];
 uint16_t sample=50u;unsigned accepted=0;
 if(!rt->g.rng_state_valid||!rt->m68k_regs.valid)return -1;
 for(int n=3;n>=0;n--){
  wf_r17_m68k_moveq(&rt->m68k_regs,1u,(int8_t)n);
  uint32_t v=wf_r17_rng21b4_live(&rt->g.rng_state,
                                   rt->g.frame_counter,&rt->m68k_regs);
  sample=(uint16_t)((v&255u)>>1);
  if(sample<100u){accepted=1;break;}
 }
 if(!accepted)sample=50u;
 if(!sample)sample=1u;
 wf_r17_m68k_move_l(&rt->m68k_regs,1u,d1);
 wf_r17_m68k_move_l(&rt->m68k_regs,2u,d2);
 unsigned total=0,choice=0;
 for(;choice<count;choice++){
  total=(total+weights[choice])&255u;
  if(total>=sample)break;
 }
 if(choice==count)return -1;
 /* ANDI.L #$FF at $24D6 clears the upper word before MOVE.W D1,D0. */
 wf_r17_m68k_move_l(&rt->m68k_regs,0u,(uint32_t)choice);
 return (int)choice;
}
/* $1D7A0, used before the original state2 movement owner. */
static void run_angle_1d7a0(wf_actor_raw_t*a){wf_set_u8(a,WF_A_ANGLE,(wf_u8(a,WF_A_FACING)&128u)?0x40u:0xc0u);}
/* $1CE38: the aligned, nonzero-distance-class continuation of state1. */
static wf_status_t aligned_1ce38(wf_runtime_t*rt,unsigned slot,wf_actor_raw_t*a,wf_actor_raw_t*t,wf_cpu_trace_t*tr){
 uint8_t f34=wf_u8(a,WF_A_FLAGS34);unsigned distance=wf_be16(a,WF_A_CPU_DISTANCE);uint8_t ts=wf_main_state(t);
 if(f34&4u){
  wf_actor_raw_t*p=ptr(rt,a,WF_A_TEAM_LINK);if(!p)return block(tr,0x1cfc4u);uint16_t x=wf_be16(p,WF_A_X),ax=wf_be16(a,WF_A_X),dx=(uint16_t)(x-ax);if(x<ax)dx=(uint16_t)(0u-dx);wf_r17_m68k_move_w(&rt->m68k_regs,0,dx);
  if(dx>=160u){wf_set_be16(a,WF_A_STATE,2u);run_angle_1d7a0(a);wf_set_u8(a,WF_A_CPU_B7,(uint8_t)(wf_u8(a,WF_A_CPU_B7)&~2u));return WF_OK;}
  wf_set_u8(a,WF_A_FLAGS34,(uint8_t)(f34&~4u));if(wf_u8(a,WF_A_FLAGS33)&2u){wf_set_u8(a,WF_A_ROLE,(uint8_t)(wf_u8(a,WF_A_ROLE)&~64u));if(!(wf_u8(p,WF_A_FLAGS33)&2u))wf_set_u8(a,0x35u,(uint8_t)(wf_u8(a,0x35u)|8u));wf_set_be16(a,WF_A_STATE,0u);return WF_OK;}
  goto horizontal;
 }
 if(wf_u8(a,WF_A_REL_FLAGS)&128u)return block(tr,0x1d032u);
 if(wf_u8(a,WF_A_CPU_B4)!=1u){
  if(wf_u8(t,WF_A_FLAGS33)&16u){
   wf_r17_m68k_move_b(&rt->m68k_regs,0,wf_u8(a,WF_A_FACING));if(wf_u8(a,WF_A_FACING)==wf_u8(t,WF_A_FACING))goto parallel;
   if(rt->g.global_007e!=1u&&!(wf_u8(a,WF_A_FLAGS33)&4u)){
    static const uint8_t chance[2]={70u,30u};int n=state1_weighted_24cc(rt,chance,2u);if(n<0)return block(tr,0x24ccu);
    if(n){wf_set_be16(a,WF_A_STATE,2u);run_angle_1d7a0(a);wf_set_u8(a,WF_A_FLAGS32,(uint8_t)(wf_u8(a,WF_A_FLAGS32)|4u));wf_set_u8(a,WF_A_ANGLE,(uint8_t)(wf_u8(a,WF_A_ANGLE)^128u));wf_set_u8(a,WF_A_FACING,(uint8_t)(wf_u8(a,WF_A_FACING)^128u));wf_set_u8(a,WF_A_CPU_B7,(uint8_t)(wf_u8(a,WF_A_CPU_B7)&~2u));return WF_OK;}
   }
   goto attack;
  }
  if(ts==4u&&wf_u8(t,WF_A_STATE4_LO)==1u){
   wf_r17_m68k_move_b(&rt->m68k_regs,0,wf_u8(a,WF_A_FACING));if(wf_u8(a,WF_A_FACING)==wf_u8(t,WF_A_FACING))goto parallel;
   wf_r17_m68k_moveq(&rt->m68k_regs,2,6);return decision_1de66(rt,slot,a,6u,tr);
  }
  if(!(wf_u8(t,WF_A_FLAGS32)&1u)){
   /* $1F310 returns Z only after it has already changed state to2. */
   static const uint8_t forward[12][2]={{8,92},{14,86},{5,95},{7,93},{16,84},{10,90},{3,97},{12,88},{7,93},{8,92},{10,90},{12,88}};
   static const uint8_t retreat[12][2]={{20,80},{30,70},{15,85},{20,80},{35,65},{25,75},{10,90},{25,75},{12,88},{18,82},{25,75},{22,78}};
   unsigned id=wf_u8(a,WF_A_INTERNAL_ID);wf_r17_m68k_moveq(&rt->m68k_regs,0,0);wf_r17_m68k_move_w(&rt->m68k_regs,0,(uint16_t)(id*2u));int try_roll=0,back=distance>=224u;unsigned mask=back?4u:2u;
   if(back){if(!(wf_u8(a,WF_A_FLAGS33)&4u)&&!(wf_u8(a,WF_A_CPU_B6)&mask))try_roll=1;}
   else if(distance<200u&&!(wf_u8(a,WF_A_CPU_B6)&mask))try_roll=1;
   if(back&&!(wf_u8(a,WF_A_FLAGS33)&4u))wf_set_u8(a,WF_A_CPU_B6,(uint8_t)(wf_u8(a,WF_A_CPU_B6)|mask));
   if(!back&&distance<200u)wf_set_u8(a,WF_A_CPU_B6,(uint8_t)(wf_u8(a,WF_A_CPU_B6)|mask));
   if(try_roll){if(id>=12u)return block(tr,0x1f338u);int n=state1_weighted_24cc(rt,back?retreat[id]:forward[id],2u);if(n<0)return block(tr,0x24ccu);if(!n){wf_set_be16(a,WF_A_STATE,2u);wf_set_u8(a,WF_A_CPU_B7,(uint8_t)(wf_u8(a,WF_A_CPU_B7)&~2u));run_angle_1d7a0(a);if(back){wf_set_u8(a,WF_A_FLAGS32,(uint8_t)(wf_u8(a,WF_A_FLAGS32)|4u));wf_set_u8(a,WF_A_ANGLE,(uint8_t)(wf_u8(a,WF_A_ANGLE)^128u));wf_set_u8(a,WF_A_FACING,(uint8_t)(wf_u8(a,WF_A_FACING)^128u));}wf_r17_m68k_moveq(&rt->m68k_regs,0,0);return WF_OK;}}
   wf_r17_m68k_moveq(&rt->m68k_regs,0,1);
  }
 }
 if(!(wf_u8(t,WF_A_FLAGS33)&16u))goto parallel;
 wf_r17_m68k_move_b(&rt->m68k_regs,0,wf_u8(a,WF_A_FACING));if(wf_u8(a,WF_A_FACING)==wf_u8(t,WF_A_FACING))goto parallel;
attack:
 if(distance<104u){
  wf_r17_m68k_moveq(&rt->m68k_regs,2,0);unsigned s=wf_u8(t,WF_A_SELECTOR_LO);
  if(ts!=5u||s==5u||s==0x2au||s==0x2du||s==0x20u||s==0x41u)return choose_1dea6(rt,slot,a,0x2307cu,0u,tr);
  goto guard;
 }
 if(distance<176u){wf_r17_m68k_moveq(&rt->m68k_regs,2,0);unsigned s=wf_u8(t,WF_A_SELECTOR_LO);if(ts==5u){if(s==0x2du||s==0x20u||s==0x41u)return decision_1de66(rt,slot,a,0u,tr);goto guard;}}
 wf_set_be16(a,WF_A_STATE,5u);wf_set_be16(a,WF_A_SELECTOR,0x3cu);return WF_OK;
guard:
 wf_set_u8(a,WF_A_MOTION_MODE,0u);wf_set_be16(a,WF_A_STATE,5u);wf_set_be16(a,WF_A_SELECTOR,0x75u);return WF_OK;
parallel:
 if(wf_u8(a,WF_A_FLAGS34)&2u){
  if(!(wf_u8(a,WF_A_FLAGS33)&1u)){wf_set_be16(a,WF_A_STATE,1u);wf_set_be16(a,WF_A_AE,4u);return WF_OK;}
  wf_actor_raw_t*p=ptr(rt,a,WF_A_TEAM_LINK);if(!p)return block(tr,0x1c608u);wf_set_u8(a,WF_A_FLAGS34,(uint8_t)(wf_u8(a,WF_A_FLAGS34)&~2u));wf_set_u8(p,WF_A_FLAGS34,(uint8_t)(wf_u8(p,WF_A_FLAGS34)|2u));if(wf_u8(a,WF_A_FLAGS33)&128u)rt->g.global_1684=wf_be32(a,WF_A_TEAM_LINK);else rt->g.global_1688=wf_be32(a,WF_A_TEAM_LINK);
 }
horizontal:
 wf_set_u8(a,WF_A_ANGLE,(wf_u8(a,WF_A_CPU_B5)&1u)?0x40u:0xc0u);return WF_OK;
}
/* Live stage-zero route through $1C93E/$1CA82/$1DE66. Other jump-table
 * branches remain explicit ROM dependencies. */
static wf_status_t state1_sub0(wf_runtime_t*rt,unsigned slot,
                                wf_actor_raw_t*a,wf_cpu_trace_t*tr){
 wf_actor_raw_t*t=ptr(rt,a,WF_A_TARGET);
 wf_set_u8(a,WF_A_CPU_B7,(uint8_t)(wf_u8(a,WF_A_CPU_B7)&~0x80u));
 if(!t)return block(tr,0x00010be8u);
 uint16_t x=wf_be16(t,WF_A_X),off=0u;
 uint8_t ts=wf_main_state(t);
 if(ts==4u){uint8_t sub=wf_u8(t,WF_A_STATE4_LO);off=sub==8u?0x40u:sub==9u?0x10u:0u;}
 if(ts==5u){uint8_t sel=wf_u8(t,WF_A_SELECTOR_LO);off=sel==0x22u?0x30u:sel==0x52u?0x10u:0u;}
 x=(uint16_t)(x+((wf_u8(t,WF_A_FACING)&0x80u)?-off:off));
 wf_set_u8(a,WF_A_FACING,(uint8_t)((wf_u8(a,WF_A_FACING)&0x7fu)|
    (x>=wf_be16(a,WF_A_X)?0x80u:0u)));
 if(rt->g.mode_0161&1u)return block(tr,0x0001c958u);
 if(wf_u8(a,WF_A_FLAGS33)&4u)return block(tr,0x0001ca2eu);
 if(wf_u8(t,WF_A_FLAGS33)&4u)return block(tr,0x0001c9b2u);
 wf_set_u8(a,WF_A_CPU_B6,(uint8_t)(wf_u8(a,WF_A_CPU_B6)&~0x20u));
 if(wf_u8(t,WF_A_FLAGS32)&2u)return block(tr,0x0001ccf0u);
 if(wf_u8(a,WF_A_CPU_B4)||wf_be16(a,WF_A_CPU_DISTANCE_Y)>=12u){
  /* $1CD5C/$1CDD0: approach an opponent in a different lane. */
  if(wf_be16(a,WF_A_CPU_DISTANCE_Y)<12u)return aligned_1ce38(rt,slot,a,t,tr);
  if(!wf_u8(a,WF_A_CPU_B4)||wf_u8(a,WF_A_CPU_B4)==1u){
   if(ts==5u&&wf_u8(t,WF_A_SELECTOR_LO)==0x4cu){wf_set_u8(a,WF_A_CPU_B7,(uint8_t)(wf_u8(a,WF_A_CPU_B7)|64u));wf_set_be16(a,WF_A_STATE,5u);wf_set_be16(a,WF_A_SELECTOR,0x3du);return WF_OK;}
   if((rt->g.mode_0161&1u)&&!(wf_u8(a,WF_A_CPU_B5)&128u)&&(wf_u8(t,WF_A_FLAGS33)&64u)){wf_set_be16(a,WF_A_STATE,0u);wf_set_u8(a,WF_A_CPU_B6,(uint8_t)(wf_u8(a,WF_A_CPU_B6)|128u));wf_set_u8(a,WF_A_CPU_B9,(uint8_t)(wf_u8(a,WF_A_CPU_B9)|1u));return WF_OK;}
  }
  uint8_t flags=wf_u8(a,WF_A_CPU_B5),angle;
  if(wf_u8(t,WF_A_FLAGS33)&16u)angle=flags&1u?0x40u:0xc0u;
  else if(wf_be16(a,WF_A_CPU_DISTANCE)<12u)angle=flags&2u?0u:0x80u;
  else angle=flags&2u?(flags&1u?0x20u:0xe0u):(flags&1u?0x60u:0xa0u);
  wf_set_u8(a,WF_A_ANGLE,angle);return WF_OK;
 }
 uint8_t f34=wf_u8(a,WF_A_FLAGS34);
 wf_set_u8(a,WF_A_FLAGS34,(uint8_t)(f34&~4u));
 if((f34&4u)&&(wf_u8(a,WF_A_FLAGS33)&2u))
  return block(tr,0x0001cffau);
 if(wf_u8(a,WF_A_REL_FLAGS)&0x80u)return block(tr,0x0001cd2cu);
 if(wf_u8(a,WF_A_CPU_B5)&0x80u)return assist_1f5b2(rt,slot,a,tr);
 if(wf_u8(a,WF_A_CPU_B9)&2u){
  /* $1CAD6..$1CB2A: the latched stand-up wait inspects the target state. */
  int idle=(ts==0u)||(ts==1u&&wf_u8(t,WF_A_CPU_SUBSTATE)==0u)||
           (ts==4u&&wf_u8(t,WF_A_STATE4_LO)==1u);
  if(idle){
   if(wf_u8(a,WF_A_FLAGS34)&2u)return block(tr,0x0001c5fau);
   wf_set_u8(a,WF_A_ANGLE,(wf_u8(a,WF_A_CPU_B5)&1u)?0x40u:0xc0u);
   return WF_OK;
  }
  wf_set_u8(a,WF_A_CPU_B9,(uint8_t)(wf_u8(a,WF_A_CPU_B9)&~2u));
  if(rt->g.mode_0161&1u){
   wf_set_be16(a,WF_A_STATE,0u);
   wf_set_u8(a,WF_A_CPU_B6,(uint8_t)(wf_u8(a,WF_A_CPU_B6)|0x80u));
  }
  return WF_OK;
 }
 wf_r17_m68k_moveq(&rt->m68k_regs,2u,0);
 if(ts==5u){
  /* $1CB42-$1CBEA: react to the target's current action. */
  uint8_t selector=wf_u8(t,WF_A_SELECTOR_LO);
  if(wf_u8(t,WF_A_FLAGS33)&16u){
   wf_r17_m68k_move_b(&rt->m68k_regs,0u,wf_u8(a,WF_A_FACING));
   if(wf_u8(a,WF_A_FACING)==wf_u8(t,WF_A_FACING))return WF_OK;
   if(selector==5u||selector==0x2au||selector==0x2du||selector==0x20u||selector==0x41u)
    return choose_1dea6(rt,slot,a,0x2307cu,0u,tr);
   wf_set_u8(a,WF_A_MOTION_MODE,0u);wf_set_be16(a,WF_A_STATE,5u);wf_set_be16(a,WF_A_SELECTOR,0x75u);return WF_OK;
  }
  if(selector==0x4cu)return block(tr,0x1ccb4u);
  if((wf_u8(t,WF_A_FLAGS33)&64u)&&(rt->g.mode_0161&1u)){
   wf_set_u8(a,WF_A_CPU_B6,(uint8_t)(wf_u8(a,WF_A_CPU_B6)|128u));
   wf_set_u8(a,WF_A_CPU_B9,(uint8_t)(wf_u8(a,WF_A_CPU_B9)|1u));wf_set_be16(a,WF_A_STATE,0u);return WF_OK;
  }
  wf_set_u8(a,WF_A_CPU_B5,(uint8_t)(wf_u8(a,WF_A_CPU_B5)&~16u));
  wf_set_be16(a,WF_A_STATE,5u);wf_set_be16(a,WF_A_SELECTOR,0u);
  return wf_state5_execute(rt,slot,tr?&tr->state5:NULL);
 }
 if(ts!=0u&&ts!=1u){
  if(ts==4u)return knockdown_1cbf8(rt,slot,a,tr);
  if(ts==7u)return decision_1de66(rt,slot,a,0u,tr);
  /* $1CCD0: running/other target states use the same pursuit choices. */
  if(wf_u8(t,WF_A_FLAGS33)&16u){
   wf_r17_m68k_move_b(&rt->m68k_regs,0u,wf_u8(a,WF_A_FACING));
   if(wf_u8(a,WF_A_FACING)!=wf_u8(t,WF_A_FACING))
    return choose_1dea6(rt,slot,a,0x2307cu,0u,tr);
   if(wf_u8(a,WF_A_FLAGS34)&2u)return block(tr,0x1c5fau);
   wf_set_u8(a,WF_A_ANGLE,(wf_u8(a,WF_A_CPU_B5)&1u)?0x40u:0xc0u);return WF_OK;
  }
  wf_set_u8(a,WF_A_CPU_B7,(uint8_t)(wf_u8(a,WF_A_CPU_B7)|64u));
  wf_set_be16(a,WF_A_STATE,5u);wf_set_be16(a,WF_A_SELECTOR,0x3du);return WF_OK;
 }
 /* $1CB30 branches to the shared $1DE66 owner. Its ASL row offset
  * accepts tier1 and uses the exact ROM data, rather than a tier/2 table. */
 return decision_1de66(rt,slot,a,0u,tr);
}
/* ROM $1D398/$1D3A2: the active partner tracks the partner's Y and may
 * transfer pursuit to that partner's target when their positions converge. */
static wf_status_t state1_sub1(wf_runtime_t*rt,unsigned slot,
                               wf_actor_raw_t*a,wf_cpu_trace_t*tr){
 if(!(wf_u8(a,WF_A_STATE)&0x80u))return WF_OK;
 wf_actor_raw_t*partner=ptr(rt,a,WF_A_TEAM_LINK);
 if(!partner)return block(tr,0x0001d3aau);
 uint16_t ay=wf_be16(a,WF_A_Y),py=wf_be16(partner,WF_A_Y);
 uint16_t dy=ay>py?(uint16_t)(ay-py):(uint16_t)(py-ay);
 uint8_t side=(uint8_t)(wf_u8(a,WF_A_FLAGS33)&0x80u);
 wf_set_u8(a,WF_A_ANGLE,ay>py?(side?0x78u:0x88u):(side?0xf8u:8u));
 if(dy<8u){
  wf_set_u8(a,WF_A_ANGLE,0u);
  if(!(wf_u8(a,WF_A_FLAGS34)&1u)){
   uint32_t link=wf_be32(partner,WF_A_LINK);
   wf_actor_raw_t*linked=link?ptr(rt,partner,WF_A_LINK):NULL;
   if(link&&!linked)return block(tr,0x0001d3f4u);
   /* With A2=0 the arcade reads ROM byte $21=$FF, so the state-5
    * comparison at $1D3FA is false and pursuit continues. */
   if(linked&&wf_main_state(linked)==5u&&wf_u8(linked,WF_A_SELECTOR_LO)==0x53u){
    uint16_t ax=wf_be16(a,WF_A_X),lx=wf_be16(linked,WF_A_X);
    uint16_t linked_dx=ax>lx?(uint16_t)(ax-lx):(uint16_t)(lx-ax);
    if(linked_dx<0x40u){
     wf_set_be16(a,WF_A_STATE,5u);
     wf_set_be16(a,WF_A_SELECTOR,0x39u);
     return WF_OK;
    }
   }
  }
 }
 if(rt->g.credited_route==0u)return WF_OK;
 wf_actor_raw_t*target=ptr(rt,partner,WF_A_TARGET);
 if(!target)return block(tr,0x0001d436u);
 uint8_t state=wf_main_state(target);
 if(state!=0u&&state!=1u&&
    !(state==4u&&(wf_u8(target,WF_A_STATE4_LO)==1u||
                   wf_u8(target,WF_A_STATE4_LO)==0u||
                   wf_u8(target,WF_A_STATE4_LO)==10u)))return WF_OK;
 uint16_t ty=wf_be16(target,WF_A_Y),tx=wf_be16(target,WF_A_X),ax=wf_be16(a,WF_A_X);
 uint16_t yd=ay>ty?(uint16_t)(ay-ty):(uint16_t)(ty-ay);
 uint16_t xd=ax>tx?(uint16_t)(ax-tx):(uint16_t)(tx-ax);
 if(yd>=0x10u||xd>=0x40u){
  wf_set_u8(a,WF_A_CPU_B7,(uint8_t)(wf_u8(a,WF_A_CPU_B7)&~2u));
  return WF_OK;
 }
 uint8_t old=wf_u8(a,WF_A_CPU_B7);
 wf_set_u8(a,WF_A_CPU_B7,(uint8_t)(old|2u));
 if(old&2u)return WF_OK;
 static const uint8_t weights[10][3]={
  {25,15,60},{30,20,50},{30,20,50},{35,25,40},{40,30,30},
  {40,30,30},{40,30,30},{40,30,30},{40,30,30},{45,35,20}
 };
 if(rt->g.progression_0162>=10u)return block(tr,0x0001d4a0u);
 int choice=state1_weighted_24cc(rt,weights[rt->g.progression_0162],3u);
 if(choice<0)return block(tr,0x000024ccu);
 if(choice==2)return WF_OK;
 if(choice==0){
  wf_set_be32(a,WF_A_TARGET,wf_actor_rom_base((unsigned)(target-rt->actors)));
  wf_set_be16(a,WF_A_STATE,5u);wf_set_be16(a,WF_A_SELECTOR,0x39u);
  return wf_state5_execute(rt,slot,tr?&tr->state5:0);
 }
 if(wf_be32(target,WF_A_LINK))return WF_OK;
 wf_set_u8(a,WF_A_CPU_B5,(uint8_t)(wf_u8(a,WF_A_CPU_B5)|0x10u));
 wf_set_u8(a,WF_A_CPU_B7,(uint8_t)(wf_u8(a,WF_A_CPU_B7)&~2u));
 wf_set_be16(a,WF_A_STATE,5u);wf_set_be16(a,WF_A_SELECTOR,0x1cu);
 return wf_state5_execute(rt,slot,tr?&tr->state5:0);
}
/* $1F15C/$20C8, preserving the original work-RAM scratch layout. */
static int cpu_arrived_1f15c(wf_runtime_t*rt,wf_actor_raw_t*a){
 uint16_t x=wf_be16(a,6),y=wf_be16(a,10),tx=wf_be16(a,0xbe),ty=wf_be16(a,0xc0);
 uint16_t ax=(uint16_t)(x-tx),ay=(uint16_t)(y-ty);if(x<tx)ax=(uint16_t)-ax;if(y<ty)ay=(uint16_t)-ay;
 if(ax<8u&&ay<8u){wf_r17_m68k_moveq(&rt->m68k_regs,0,0);return 1;}
 uint16_t dx=(uint16_t)(tx-x),dy=(uint16_t)(ty-y);unsigned q=0;
 if(dx&0x8000u){q|=1u;dx=(uint16_t)-dx;}if(dy&0x8000u){q|=2u;dy=(uint16_t)-dy;}
 if(dx>255u)dx=255u;if(dy>255u)dy=255u;
 static const uint8_t base[4]={0,192,64,128};unsigned sum=dx+dy,component=(q==0u||q==3u)?dx:dy;
 uint8_t angle=(uint8_t)(base[q]+component*64u/(sum?sum:1u)),*scratch=rt->g.direction_scratch_15e4;
 scratch[0]=angle;scratch[1]=0x1c;scratch[2]=(uint8_t)(dx>>8);scratch[3]=(uint8_t)dx;scratch[4]=(uint8_t)(dy>>8);scratch[5]=(uint8_t)dy;scratch[6]=(uint8_t)(ty>>8);scratch[7]=(uint8_t)ty;
 wf_set_u8(a,WF_A_ANGLE,angle);wf_r17_m68k_moveq(&rt->m68k_regs,0,1);return 0;
}
static void cpu_face_10be8(wf_actor_raw_t*a,const wf_actor_raw_t*t){
 uint16_t x=wf_be16(t,WF_A_X),off=0;unsigned state=wf_main_state(t);
 if(state==4u)off=wf_u8(t,WF_A_STATE4_LO)==8u?64u:wf_u8(t,WF_A_STATE4_LO)==9u?16u:0u;
 else if(state==5u)off=wf_u8(t,WF_A_SELECTOR_LO)==0x22u?48u:wf_u8(t,WF_A_SELECTOR_LO)==0x52u?16u:0u;
 x=(uint16_t)(x+((wf_u8(t,WF_A_FACING)&128u)?-off:off));
 wf_set_u8(a,WF_A_FACING,(uint8_t)((wf_u8(a,WF_A_FACING)&127u)|(x>=wf_be16(a,WF_A_X)?128u:0u)));
}
/* $1D206: request a TAG, face the opponent, then approach the corner. */
static wf_status_t state1_tag_1d206(wf_runtime_t*rt,unsigned slot,wf_actor_raw_t*a,wf_cpu_trace_t*tr){
 wf_actor_raw_t*t=ptr(rt,a,WF_A_TARGET);if(!t)return block(tr,0x10be8u);
 unsigned state=wf_main_state(t);cpu_face_10be8(a,t);
 unsigned old=wf_u8(a,WF_A_AE);wf_set_u8(a,WF_A_AE,(uint8_t)(old|32u));
 wf_actor_raw_t*p=NULL;
 if(!(old&32u)){
  p=ptr(rt,a,WF_A_TEAM_LINK);if(!p)return block(tr,0x1d214u);
  wf_set_be16(p,WF_A_STATE,1);wf_set_be16(p,WF_A_AE,2);
  wf_set_be16(a,0xbe,(wf_u8(a,WF_A_FLAGS33)&128u)?0x308u:0x200u);
  wf_set_be16(a,0xc0,(wf_u8(a,WF_A_FLAGS33)&128u)?0x128u:0x190u);
 }
 state=wf_main_state(t); /* The team link can alias the current target. */
 if(wf_be16(a,WF_A_CPU_DISTANCE_Y)<8u){
  if(!wf_u8(a,WF_A_CPU_B4)){
   wf_r17_m68k_moveq(&rt->m68k_regs,2,0);
   if(state==0u||state==1u)return decision_1de66(rt,slot,a,0,tr);
   wf_r17_m68k_moveq(&rt->m68k_regs,2,4);
   if(state==2u||state==6u)return decision_1de66(rt,slot,a,4,tr);
   if(state==5u&&(wf_u8(t,WF_A_FLAGS33)&16u)){
    wf_r17_m68k_move_b(&rt->m68k_regs,0,wf_u8(a,WF_A_FACING));
    if(wf_u8(a,WF_A_FACING)==wf_u8(t,WF_A_FACING))return WF_OK;
    wf_r17_m68k_moveq(&rt->m68k_regs,2,0);
    unsigned sel=wf_u8(t,WF_A_SELECTOR_LO);
    if(sel==0x2du||sel==0x20u||sel==0x41u)return choose_1dea6(rt,slot,a,0x2307cu,0,tr);
    wf_set_u8(a,WF_A_MOTION_MODE,0);wf_set_be16(a,WF_A_STATE,5);wf_set_be16(a,WF_A_SELECTOR,0x75);return WF_OK;
   }
  }else if(wf_u8(t,WF_A_FLAGS33)&16u){wf_set_be16(a,WF_A_STATE,5);wf_set_be16(a,WF_A_SELECTOR,0x3c);return WF_OK;}
 }
 if(cpu_arrived_1f15c(rt,a)){
  p=ptr(rt,a,WF_A_TEAM_LINK);if(!p)return block(tr,0x1d2feu);
  wf_set_u8(a,WF_A_MOTION_MODE,0);wf_set_u8(a,0x23,0x60);
  if(wf_u8(p,WF_A_AE)&128u){wf_set_u8(a,WF_A_CPU_B5,(uint8_t)(wf_u8(a,WF_A_CPU_B5)&~64u));wf_set_be16(a,WF_A_STATE,5);wf_set_be16(a,WF_A_SELECTOR,0x4c);}
 }
 return WF_OK;
}
/* $1D31E/$EECE: approach and reserve an arcade corner. */
static wf_status_t state1_corner_1d31e(wf_runtime_t*rt,wf_actor_raw_t*a,wf_cpu_trace_t*tr){
 wf_actor_raw_t*t=ptr(rt,a,WF_A_TARGET);if(!t)return block(tr,0x10be8u);cpu_face_10be8(a,t);
 unsigned old=wf_u8(a,WF_A_AE),corner=wf_be16(a,WF_A_CPU_BC);wf_set_u8(a,WF_A_AE,(uint8_t)(old|32u));
 if(!(old&32u)){
  static const uint16_t coords[4][2]={{504,416},{768,416},{464,288},{800,288}};
  if(corner>=4u)return block(tr,0x1d338u);
  wf_set_be16(a,0xbe,coords[corner][0]);wf_set_be16(a,0xc0,coords[corner][1]);
 }
 wf_r17_m68k_moveq(&rt->m68k_regs,0,0);wf_r17_m68k_move_w(&rt->m68k_regs,0,(uint16_t)corner);
 wf_r17_m68k_move_l(&rt->m68k_regs,1,rt->g.global_1670);
 if((rt->g.global_1670&(1u<<(corner&31u)))||wf_u8(t,WF_A_STATE+1u)!=4u){wf_set_be16(a,WF_A_STATE,0);return WF_OK;}
 if(!cpu_arrived_1f15c(rt,a))return WF_OK;
 wf_r17_m68k_moveq(&rt->m68k_regs,0,0);wf_r17_m68k_move_w(&rt->m68k_regs,0,(uint16_t)corner);
 if(!wf_be16(a,0xacu))wf_set_be16(a,0xacu,0x8004u);
 rt->g.global_1670|=1u<<(corner&7u);wf_r17_m68k_move_l(&rt->m68k_regs,1,rt->g.global_1670);
 wf_set_be16(a,WF_A_PROFILE,0);wf_set_be16(a,WF_A_TIMER44,(uint16_t)corner);wf_set_be16(a,WF_A_STATE,8);return WF_OK;
}
/* $1C8F4 dispatch, $1D1F0 substate four. */
static wf_status_t state1(wf_runtime_t*rt,unsigned slot,wf_actor_raw_t*a,wf_cpu_trace_t*tr){
 uint8_t sub=wf_u8(a,WF_A_CPU_SUBSTATE);
 if(sub>=13u)return block(tr,0x0001c8f4u);
 wf_r17_m68k_moveq(&rt->m68k_regs,0u,0);
 wf_r17_m68k_move_b(&rt->m68k_regs,0u,(uint8_t)(sub<<2));
 if(sub==0u)return state1_sub0(rt,slot,a,tr);
 if(sub==1u)return state1_sub1(rt,slot,a,tr);
 if(sub==8u)return state1_tag_1d206(rt,slot,a,tr);
 if(sub==9u)return state1_corner_1d31e(rt,a,tr);
 /* Both CPU jump-table entries $1C910/$1C914 point to RTS at $1D524. */
 if(sub==2u||sub==3u)return WF_OK;
 if(sub==10u)return wf_cpu_pursuit_1d5a2(rt,slot,tr?&tr->state5:NULL);
 if(sub!=4u)return block(tr,arcade_cpu_state1_handler(sub));
 if(wf_u8(a,WF_A_FLAGS33)&1u){
  wf_set_u8(a,WF_A_FLAGS34,(uint8_t)(wf_u8(a,WF_A_FLAGS34)&~2u));
  wf_set_be16(a,WF_A_STATE,0u);
 }
 return WF_OK;
}

static int weighted_runtime(wf_runtime_t *rt,const uint8_t *weights,size_t count,int *choice){
 if(!rt||!weights||!choice||!rt->g.rng_state_valid||!rt->g.rng_register_context_valid)return 0;
 arcade_cpu_rng_t r={rt->g.rng_state,rt->g.frame_counter,rt->g.rng_d2,rt->g.rng_d3,rt->g.rng_d4,rt->g.rng_d5,rt->g.rng_d6,rt->g.rng_d7};
 int c=arcade_cpu_weighted_select(&r,weights,count,0);
 if(c<0)return 0;
 rt->g.rng_state=r.seed;
 *choice=c;
 return 1;
}
static wf_status_t state6(wf_runtime_t*rt,unsigned slot,wf_cpu_trace_t*tr){
 wf_actor_raw_t*a=&rt->actors[slot];
 /* $1EDC4: disabled when $1C007C==0 or +$45==0. */
 if(rt->g.credited_route==0u||wf_u8(a,WF_A_TIMER44+1u)==0u)return WF_OK;
 /* Initialized/high-phase path: BCLR +$B5.bit4; only an old set bit commits 5/3F. */
 if((wf_u8(a,WF_A_STATE)&0x80u)!=0u){
  uint8_t b5=wf_u8(a,WF_A_CPU_B5);
  wf_set_u8(a,WF_A_CPU_B5,(uint8_t)(b5&~0x10u));
  if((b5&0x10u)==0u)return WF_OK;
  wf_set_be16(a,WF_A_TIMER44,0u);
  wf_set_be16(a,WF_A_STATE,5u);
  wf_set_be16(a,WF_A_SELECTOR,0x003fu);
  return wf_state5_execute(rt,slot,&tr->state5);
 }
 /* First phase: clear bit4, choose exact two-byte row by $1C0162 and tier. */
 wf_set_u8(a,WF_A_CPU_B5,(uint8_t)(wf_u8(a,WF_A_CPU_B5)&~0x10u));
 if(rt->g.progression_0162>=10u||wf_u8(a,WF_A_TIER_LO)>=3u)return WF_BAD_STATE;
 const uint8_t *w=arcade_cpu_state6_weight_pair((uint8_t)rt->g.progression_0162,wf_u8(a,WF_A_TIER_LO));
 if(!w)return WF_BAD_STATE;
 int c;
 if(!weighted_runtime(rt,w,2u,&c))return block(tr,0x000024ccu);
 if(c==0){wf_set_u8(a,WF_A_CPU_B5,(uint8_t)(wf_u8(a,WF_A_CPU_B5)|0x10u));return WF_OK;}
 wf_set_be16(a,WF_A_CPU_BC,0u);
 wf_set_u8(a,WF_A_CPU_B7,(uint8_t)(wf_u8(a,WF_A_CPU_B7)&~0x02u));
 return WF_OK;
}
static wf_status_t state9(wf_runtime_t*rt,unsigned slot,wf_cpu_trace_t*tr){
 wf_actor_raw_t*a=&rt->actors[slot];uint8_t bd=(uint8_t)(wf_u8(a,WF_A_CPU_BD)+1u);wf_set_u8(a,WF_A_CPU_BD,bd);if(bd<8u)return WF_OK;
 wf_actor_raw_t*p=ptr(rt,a,WF_A_TEAM_LINK);if(p&&wf_main_state(p)==5u&&wf_u8(p,WF_A_SELECTOR_LO)==0x89u){wf_set_be16(a,WF_A_STATE,5u);wf_set_be16(a,WF_A_SELECTOR,0x0004u);return wf_state5_execute(rt,slot,&tr->state5);}
 wf_actor_raw_t*t=ptr(rt,a,WF_A_TARGET);if(!t)return block(tr,0x001EEDAu);
 int special=(wf_main_state(t)==5u&&wf_u8(t,WF_A_SELECTOR_LO)==0x61u)||(wf_main_state(t)==4u&&(wf_state4_subtype(t)==8u||wf_state4_subtype(t)==9u));
 uint8_t id=wf_u8(a,WF_A_INTERNAL_ID);if(id>=12u)return WF_BAD_STATE;
 uint8_t sel=arcade_cpu_state9_selector(id,special!=0);
 /* $1EF12 MOVE.B (A3,D0.W),+$61: low-byte selector write first. */
 wf_set_u8(a,WF_A_SELECTOR_LO,sel);
 /* The $24CC override is reached only when the target is state5, selector
  * !=$7B, and the actor ID is 04/07/0B.  Other routes go directly to
  * $1EF74 and must not be blocked merely because of actor ID. */
 if(wf_main_state(t)==5u&&wf_u8(t,WF_A_SELECTOR_LO)!=0x7bu&&(id==4u||id==7u||id==0x0bu))
  return block(tr,0x000024CCu);
 /* $1EF74: CLR.B +$60 preserves the table-selected low byte; BCLR
  * +$B5.bit4; state5; BSET +$B6.bit0; then jump to $1DFE2. */
 wf_set_u8(a,WF_A_SELECTOR,0u);
 wf_set_u8(a,WF_A_CPU_B5,(uint8_t)(wf_u8(a,WF_A_CPU_B5)&(uint8_t)~0x10u));
 wf_set_be16(a,WF_A_STATE,5u);
 wf_set_u8(a,WF_A_CPU_B6,(uint8_t)(wf_u8(a,WF_A_CPU_B6)|1u));
 return wf_state5_execute(rt,slot,&tr->state5);
}
/* $1EFAA-$1F048: a linked CPU actor pauses, then chooses the $29 action.
 * $1F04A supplies the shorter wait when the linked actor is human. */
static wf_status_t state11_wait(wf_runtime_t*rt,wf_actor_raw_t*a,
                                wf_actor_raw_t*linked,wf_cpu_trace_t*tr){
 static const uint8_t human_wait[10]={7u,7u,6u,6u,3u,5u,5u,4u,4u,3u};
 uint8_t wait=32u,role=wf_u8(linked,WF_A_ROLE);
 if(!(role&0xc0u)){
  if(rt->g.progression_0162>=10u)return block(tr,0x0001f04au);
  wait=human_wait[rt->g.progression_0162];
 }
 if(!rt->g.rng_state_valid||!rt->m68k_regs.valid)
  return block(tr,0x000021b4u);
 uint32_t random=wf_r17_rng21b4_live(&rt->g.rng_state,
                                      rt->g.frame_counter,&rt->m68k_regs);
 wf_r17_m68k_move_w(&rt->m68k_regs,0u,(uint16_t)(random&3u));
 wf_set_u8(a,WF_A_CPU_BD,(uint8_t)(wait+(random&3u)));
 return WF_OK;
}
static wf_status_t state11(wf_runtime_t*rt,wf_actor_raw_t*a,
                            wf_cpu_trace_t*tr){
 wf_actor_raw_t*linked=ptr(rt,a,WF_A_LINK);
 if(!linked)return block(tr,0x0001efc0u);
 if(!(wf_u8(a,WF_A_STATE)&0x80u)){
  wf_set_be16(a,WF_A_CPU_B6,0u);
  wf_set_u8(a,WF_A_CPU_B9,(uint8_t)(wf_u8(a,WF_A_CPU_B9)&~2u));
  wf_status_t s=state11_wait(rt,a,linked,tr);
  if(s!=WF_OK)return s;
 }
 uint8_t remaining=(uint8_t)(wf_u8(a,WF_A_CPU_BD)-1u);
 wf_set_u8(a,WF_A_CPU_BD,remaining);
 if(remaining)return WF_OK;
 wf_status_t s=state11_wait(rt,a,linked,tr);
 if(s!=WF_OK)return s;
 if(wf_u8(a,WF_A_TIMER44)&0x40u)return WF_OK;
 static const uint8_t weights[2]={50u,50u};
 int choice=state1_weighted_24cc(rt,weights,2u);
 if(choice<0)return block(tr,0x000024ccu);
 if(choice)return WF_OK;
 wf_set_be16(a,WF_A_STATE,5u);
 wf_set_be16(a,WF_A_SELECTOR,0x29u);
 wf_set_u8(linked,WF_A_TIMER44,(uint8_t)(wf_u8(linked,WF_A_TIMER44)|0x40u));
 return WF_OK;
}
/* $1F06A: the just-transferred hold waits 48 CPU turns before considering
 * the $1F7F0 target arbitration. */
/* Data-only ROM windows; no instruction interpretation is linked. */
static const uint8_t *cpu_data(uint32_t at,size_t n){
#define SPAN(base) if(at>=0x##base##u&&at-0x##base##u<=sizeof cpu_data_##base&&n<=sizeof cpu_data_##base-(at-0x##base##u))return cpu_data_##base+(at-0x##base##u)
 SPAN(1f150);SPAN(1f866);SPAN(219b4);
#undef SPAN
 return NULL;
}
static int cpu_pointer(uint32_t at,uint32_t*out){
 const uint8_t*p=cpu_data(at,4);if(!p)return 0;
 *out=((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|(p[2]<<8)|p[3];return 1;
}
/* $1F7F0 returns carry separately; WF_BLOCKED only denotes inaccessible data. */
static wf_status_t arbitrate_1f7f0(wf_runtime_t*rt,wf_actor_raw_t*a,int*carry,wf_cpu_trace_t*tr){
 *carry=1;
 if(rt->g.credited_route||(rt->g.mode_0161&1u))return WF_OK;
 wf_actor_raw_t*t=ptr(rt,a,WF_A_TARGET);if(!t)return block(tr,0x1f802u);
 uint16_t health=wf_be16(t,WF_A_STAMINA);
 wf_r17_m68k_move_w(&rt->m68k_regs,0,health);
 if(health>=wf_be16(a,WF_A_STAMINA))return WF_OK;
 unsigned id=wf_u8(a,WF_A_INTERNAL_ID);uint32_t table;
 wf_r17_m68k_moveq(&rt->m68k_regs,0,0);
 wf_r17_m68k_move_w(&rt->m68k_regs,0,(uint16_t)(id*4u));
 if(!cpu_pointer(((rt->g.global_0166&255u)?0x1f87au:0x1f866u)+id*4u,&table))return block(tr,0x1f82cu);
 uint8_t index=wf_u8(a,0x103u);wf_r17_m68k_move_b(&rt->m68k_regs,0,index);
 const uint8_t*p=cpu_data(table+(uint16_t)rt->m68k_regs.d[0],1);
 if(!p)return block(tr,0x1f834u);
 if(*p==255u)return WF_OK;
 wf_set_u8(a,0x101u,(uint8_t)(*p&127u));
 if(*p&128u)wf_set_u8(a,WF_A_FLAGS34,(uint8_t)(wf_u8(a,WF_A_FLAGS34)|16u));
 wf_set_u8(a,0x103u,(uint8_t)(index+1u));*carry=0;return WF_OK;
}
static wf_status_t state12(wf_runtime_t*rt,unsigned slot,wf_actor_raw_t*a,wf_cpu_trace_t*tr){
 uint8_t bd=wf_u8(a,WF_A_CPU_BD);
 if(bd<0x30u){wf_set_u8(a,WF_A_CPU_BD,(uint8_t)(bd+1u));return WF_OK;}
 wf_set_be16(a,WF_A_CPU_B6,0u);
 wf_set_u8(a,WF_A_CPU_B9,(uint8_t)(wf_u8(a,WF_A_CPU_B9)&~2u));
 wf_r17_m68k_moveq(&rt->m68k_regs,2,1);
 int carry;wf_status_t status=arbitrate_1f7f0(rt,a,&carry,tr);if(status!=WF_OK)return status;
 if(!carry)wf_set_be16(a,WF_A_SELECTOR,wf_be16(a,0x100u));
 else{
  uint8_t id=wf_u8(a,WF_A_INTERNAL_ID);
  if(wf_u8(a,WF_A_CPU_B5)&128u){
   const uint8_t*p=cpu_data(0x1f150u+id,1);if(!p)return block(tr,0x1f0a0u);
   wf_r17_m68k_moveq(&rt->m68k_regs,0,0);wf_r17_m68k_move_b(&rt->m68k_regs,0,id);
   wf_set_u8(a,WF_A_SELECTOR_LO,*p);
  }else{
   uint32_t table;unsigned face=(wf_u8(a,WF_A_FACING)&128u)?4u:0u;
   unsigned spatial=wf_u8(a,0x31u),lane=spatial<3u?0u:spatial>=9u?8u:4u;
   if(id>=12u||!cpu_pointer(0x219b4u+id*4u,&table)||!cpu_pointer(table+4u,&table)||!cpu_pointer(table+face,&table)||!cpu_pointer(table+lane,&table))return block(tr,0x1f0eau);
   const uint8_t*count=cpu_data(table,1);if(!count||!*count)return block(tr,0x1f0eeu);
   wf_actor_raw_t*linked=ptr(rt,a,WF_A_LINK);if(!linked)return block(tr,0x1f0f4u);
   uint16_t tier=rt->g.global_16a0<0x10u?2u:wf_be16(linked,WF_A_TIER);
   wf_r17_m68k_moveq(&rt->m68k_regs,0,0);wf_r17_m68k_moveq(&rt->m68k_regs,1,0);wf_r17_m68k_moveq(&rt->m68k_regs,2,(int8_t)lane);
   wf_r17_m68k_move_w(&rt->m68k_regs,0,tier);wf_r17_m68k_move_b(&rt->m68k_regs,1,*count);
   uint16_t offset=0;
   if(tier&255u){unsigned shift=(tier>>1)&63u;offset=shift>=16u?0u:(uint16_t)((unsigned)*count<<shift);wf_r17_m68k_move_w(&rt->m68k_regs,0,(uint16_t)(tier>>1));wf_r17_m68k_move_w(&rt->m68k_regs,1,offset);}
   const uint8_t*weights=cpu_data(table+1u+*count+offset,*count),*selectors=cpu_data(table+1u,*count);
   if(!weights||!selectors)return block(tr,0x1f116u);
   int choice=state1_weighted_24cc(rt,weights,*count);if(choice<0)return block(tr,0x24ccu);
   wf_set_be16(a,WF_A_SELECTOR,selectors[choice]);
  }
 }
 wf_set_be16(a,WF_A_STATE,5u);wf_set_u8(a,WF_A_CPU_B5,(uint8_t)(wf_u8(a,WF_A_CPU_B5)|16u));
 wf_set_u8(a,WF_A_CPU_B7,(uint8_t)(wf_u8(a,WF_A_CPU_B7)|2u));
 wf_set_be16(a,WF_A_TIMER44,0u);wf_set_be16(a,WF_A_TIMER46,0u);
 return wf_state5_execute(rt,slot,tr?&tr->state5:0);
}
/* $24CC walks bytes until its accumulated weight reaches the sample.
 * The clamped counter 5 can deliberately cross a two-byte row ($23E76). */
static size_t state4_weight_count(const uint8_t *p){
 uintptr_t at=(uintptr_t)p,base=(uintptr_t)cpu_data_219b4;
 if(at>=base&&at-base<sizeof cpu_data_219b4)return sizeof cpu_data_219b4-(at-base);
 return 2u;
}
/* $1DA7E-$1DE46: CPU victim initialization, tag request and hold reversal. */
static wf_status_t state4(wf_runtime_t*rt,unsigned slot,wf_actor_raw_t*a,wf_cpu_trace_t*tr){
 if(!(wf_u8(a,WF_A_STATE)&128u)){
  wf_set_be16(a,WF_A_AE,0);wf_set_be16(a,WF_A_CPU_BC,0);
  wf_set_u8(a,WF_A_CPU_B5,(uint8_t)(wf_u8(a,WF_A_CPU_B5)&0xc7u));wf_set_u8(a,WF_A_CPU_B7,(uint8_t)(wf_u8(a,WF_A_CPU_B7)&0x7cu));wf_set_u8(a,WF_A_FLAGS34,(uint8_t)(wf_u8(a,WF_A_FLAGS34)&~4u));
  if(wf_u8(a,WF_A_FLAGS33)&2u)wf_set_u8(a,WF_A_ROLE,(uint8_t)(wf_u8(a,WF_A_ROLE)&~64u));return WF_OK;
 }
 unsigned sub=wf_u8(a,WF_A_STATE4_LO);
 if(sub==1u){
  wf_set_u8(a,WF_A_CPU_B9,(uint8_t)(wf_u8(a,WF_A_CPU_B9)&~2u));wf_set_u8(a,WF_A_CPU_B5,(uint8_t)(wf_u8(a,WF_A_CPU_B5)&0x3fu));wf_set_u8(a,WF_A_CPU_B7,2);
  /* $1F3D6: the state-4/1 caller always reaches its two-byte $1F458 row. */
  if(!(wf_u8(a,WF_A_ROLE)&64u)&&rt->g.credited_route&&!(rt->g.mode_0161&3u)&&!(wf_u8(a,WF_A_FLAGS33)&4u)&&(wf_u8(a,WF_A_FLAGS33)&1u)){
   static const uint8_t weights[2]={30,70};int choice=state1_weighted_24cc(rt,weights,2);if(choice<0)return block(tr,0x24ccu);
   if(!choice)wf_set_u8(a,WF_A_CPU_B5,(uint8_t)(wf_u8(a,WF_A_CPU_B5)|64u));
  }
  return WF_OK;
 }
 if(sub!=8u&&sub!=9u)return WF_OK;
 wf_set_u8(a,WF_A_CPU_B9,(uint8_t)(wf_u8(a,WF_A_CPU_B9)&~2u));
 if(!rt->g.credited_route&&!(rt->g.mode_0161&1u)){
  if(sub==8u){wf_actor_raw_t*t=ptr(rt,a,WF_A_TARGET);if(!t)return block(tr,0x1db02u);
   if(!(wf_u8(t,WF_A_INTERNAL_ID)==9u&&(wf_u8(t,0x103u)==7u||wf_u8(t,0x103u)==8u)))wf_set_be16(a,0x9au,0x1000u);
  }return WF_OK;
 }
 if(!wf_be32(a,WF_A_LINK))return WF_OK;
 wf_actor_raw_t*l=ptr(rt,a,WF_A_LINK);if(!l)return block(tr,0x1db30u);
 unsigned sel=wf_u8(l,WF_A_SELECTOR_LO),old;
 uint32_t table=0;const uint8_t*w=NULL;int choice;
 static const uint8_t royal_weights[6]={75,25,45,55,25,75}; /* $1DE48/$1DE4E */
 if(sub==8u&&wf_main_state(l)==5u&&sel==8u){
  old=wf_u8(a,WF_A_CPU_B7);wf_set_u8(a,WF_A_CPU_B7,(uint8_t)(old|2u));if(old&2u)return WF_OK;
  if(rt->g.mode_0161&1u){if(wf_be16(a,WF_A_TIER)>=3u)return block(tr,0x1db72u);w=royal_weights+wf_be16(a,WF_A_TIER)*2u;}
  else{wf_r17_m68k_move_w(&rt->m68k_regs,0,(uint16_t)(rt->g.progression_0162*4u));if(!cpu_pointer(0x23d7eu+(uint16_t)rt->m68k_regs.d[0],&table))return block(tr,0x1db6eu);w=cpu_data(table+wf_be16(a,WF_A_TIER)*2u,2);}
  wf_r17_m68k_moveq(&rt->m68k_regs,1,0);wf_r17_m68k_move_w(&rt->m68k_regs,1,(uint16_t)(wf_be16(a,WF_A_TIER)*2u));if(!w)return block(tr,0x1db7cu);
  choice=state1_weighted_24cc(rt,w,state4_weight_count(w));if(choice<0)return block(tr,0x24ccu);if(choice)return WF_OK;
  wf_set_be16(a,WF_A_SELECTOR,0x38);wf_set_be16(a,WF_A_STATE,5);return wf_state5_execute(rt,slot,tr?&tr->state5:NULL);
 }
 if(sub==8u&&wf_main_state(l)==5u&&(sel==0x10u||sel==0x46u||sel==0xbu||sel==9u||sel==0x22u)){
  old=wf_u8(a,WF_A_CPU_B7);wf_set_u8(a,WF_A_CPU_B7,(uint8_t)(old|2u));if(old&2u)goto finish;
  if(rt->g.mode_0161&1u){if(wf_be16(a,WF_A_TIER)>=3u)return block(tr,0x1dbf8u);w=royal_weights+wf_be16(a,WF_A_TIER)*2u;}
  else{wf_r17_m68k_move_w(&rt->m68k_regs,0,(uint16_t)(rt->g.progression_0162*4u));if(!cpu_pointer((sel==9u||sel==0x22u?0x23fc4u:0x23f78u)+(uint16_t)rt->m68k_regs.d[0],&table))return block(tr,0x1dbf4u);w=cpu_data(table+wf_be16(a,WF_A_TIER)*2u,2);}
  wf_r17_m68k_moveq(&rt->m68k_regs,0,0);wf_r17_m68k_move_w(&rt->m68k_regs,0,(uint16_t)(wf_be16(a,WF_A_TIER)*2u));if(!w)return block(tr,0x1dc02u);
  choice=state1_weighted_24cc(rt,w,state4_weight_count(w));if(choice<0)return block(tr,0x24ccu);if(choice)return WF_OK;
  wf_set_be16(a,WF_A_CPU_BC,(sel==9u||sel==0x22u)?2u:16u);goto reversal;
 }
 if(wf_main_state(l)!=5u)return WF_OK;
 if(sel==0x11u||sel==0xcu||sel==0x12u||sel==0xfu){
  old=wf_u8(a,WF_A_CPU_B7);wf_set_u8(a,WF_A_CPU_B7,(uint8_t)(old|2u));if(old&2u)goto finish;
  wf_r17_m68k_moveq(&rt->m68k_regs,0,0);wf_r17_m68k_move_w(&rt->m68k_regs,0,(uint16_t)(rt->g.progression_0162*4u));if(!cpu_pointer(0x23dd0u+(uint16_t)rt->m68k_regs.d[0],&table))return block(tr,0x1ddb2u);
  wf_r17_m68k_move_w(&rt->m68k_regs,0,(uint16_t)(wf_be16(a,WF_A_TIER)*2u));w=cpu_data(table+rt->m68k_regs.d[0],2);if(!w)return block(tr,0x1ddbeu);
  choice=state1_weighted_24cc(rt,w,state4_weight_count(w));if(choice<0)return block(tr,0x24ccu);if(choice)goto finish;
  if(sel==0xfu&&!(wf_u8(l,0x35u)&1u)){wf_set_be16(a,WF_A_SELECTOR,0x54);wf_set_be16(a,WF_A_CPU_BC,32);wf_set_u8(a,WF_A_CPU_B5,(uint8_t)(wf_u8(a,WF_A_CPU_B5)|16u));goto finish;}
  wf_set_be16(a,WF_A_CPU_BC,16);goto reversal;
 }
 /* $1DCA0 scans exactly seven selectors at $1DE54. DBRA keeps its low-word wrap. */
 static const uint8_t selectors[7]={0x10,0x0b,0x13,0x14,0x0e,0x35,0x23};
 wf_r17_m68k_moveq(&rt->m68k_regs,0,6);wf_r17_m68k_moveq(&rt->m68k_regs,2,0);wf_r17_m68k_move_b(&rt->m68k_regs,1,(uint8_t)sel);
 unsigned index=0;for(;index<7u&&selectors[index]!=sel;index++){wf_r17_m68k_move_l(&rt->m68k_regs,2,rt->m68k_regs.d[2]+1u);wf_r17_m68k_move_w(&rt->m68k_regs,0,(uint16_t)(rt->m68k_regs.d[0]-1u));}
 if(index==7u&&sel!=0xau)return WF_OK;
 old=wf_u8(a,WF_A_CPU_B7);wf_set_u8(a,WF_A_CPU_B7,(uint8_t)(old|2u));if(old&2u)goto finish;
 if(index==7u){
  wf_r17_m68k_moveq(&rt->m68k_regs,3,0);wf_r17_m68k_move_b(&rt->m68k_regs,2,wf_u8(a,0xe5u));
  if(wf_u8(a,0xe5u)<5u){wf_r17_m68k_move_w(&rt->m68k_regs,3,(uint16_t)(rt->g.progression_0162*4u));if(!cpu_pointer(0x23e76u+(uint16_t)rt->m68k_regs.d[3],&table))return block(tr,0x1dce6u);wf_r17_m68k_move_w(&rt->m68k_regs,2,(uint16_t)(rt->m68k_regs.d[2]*2u));w=cpu_data(table+rt->m68k_regs.d[2],2);if(!w)return block(tr,0x1dceeu);choice=state1_weighted_24cc(rt,w,state4_weight_count(w));if(choice<0)return block(tr,0x24ccu);if(choice)return WF_OK;}
  wf_set_be16(a,WF_A_CPU_BC,2);goto reversal;
 }
 wf_r17_m68k_move_w(&rt->m68k_regs,2,(uint16_t)(index*4u));if(!cpu_pointer(0x23e16u+index*4u,&table))return block(tr,0x1dd32u);
 wf_r17_m68k_move_w(&rt->m68k_regs,2,(uint16_t)(index*2u));wf_r17_m68k_moveq(&rt->m68k_regs,3,0);wf_r17_m68k_move_l(&rt->m68k_regs,3,0xd2u+index*2u);wf_r17_m68k_move_w(&rt->m68k_regs,4,wf_be16(a,0xd2u+index*2u));
 if((rt->m68k_regs.d[4]&255u)>=5u)wf_r17_m68k_moveq(&rt->m68k_regs,4,5);
 wf_r17_m68k_move_w(&rt->m68k_regs,4,(uint16_t)(rt->m68k_regs.d[4]*2u));w=cpu_data(table+rt->m68k_regs.d[4],2);if(!w)return block(tr,0x1dd50u);
 choice=state1_weighted_24cc(rt,w,state4_weight_count(w));if(choice<0)return block(tr,0x24ccu);if(choice)return WF_OK;
 wf_set_be16(a,WF_A_CPU_BC,16);
 if(sel==0xeu&&!(wf_u8(l,0x35u)&1u)){wf_set_be16(a,WF_A_SELECTOR,0x54);wf_set_u8(a,WF_A_CPU_B5,(uint8_t)(wf_u8(a,WF_A_CPU_B5)|16u));goto finish;}
reversal:
 wf_set_be16(a,WF_A_SELECTOR,sub==8u?0x38u:0x78u);wf_set_u8(a,WF_A_CPU_B5,(uint8_t)(wf_u8(a,WF_A_CPU_B5)|16u));
finish:
 if(!(wf_u8(a,WF_A_CPU_B5)&16u)||wf_main_state(l)!=5u)return WF_OK;
 if(sel==0x23u||sel==0x35u||sel==0xeu){if(!(wf_u8(l,WF_A_SELECTOR)&16u))return WF_OK;}
 else{uint16_t remaining=(uint16_t)(wf_be16(a,WF_A_CPU_BC)-1u);wf_set_be16(a,WF_A_CPU_BC,remaining);if(remaining)return WF_OK;}
 wf_set_be16(a,WF_A_STATE,5);return WF_OK;
}
static wf_status_t choose_1dea6(wf_runtime_t*rt,unsigned slot,wf_actor_raw_t*a,uint32_t base,unsigned lane,wf_cpu_trace_t*tr){
 unsigned id=wf_u8(a,WF_A_INTERNAL_ID);uint32_t table;
 if(id>=12u||!cpu_pointer(base+id*4u,&table)||!cpu_pointer(table+lane*4u,&table))return block(tr,0x1deb6u);
 const uint8_t*n=cpu_data(table,1);wf_actor_raw_t*t=ptr(rt,a,WF_A_TARGET);
 if(!n||!*n||!t)return block(tr,0x1dec0u);
 uint16_t tier=rt->g.global_16a0<0x10u?2u:wf_be16(t,WF_A_TIER),offset=0;
 wf_r17_m68k_moveq(&rt->m68k_regs,0,0);wf_r17_m68k_moveq(&rt->m68k_regs,1,0);
 wf_r17_m68k_move_w(&rt->m68k_regs,2,(uint16_t)(lane*4u));
 wf_r17_m68k_move_b(&rt->m68k_regs,1,*n);wf_r17_m68k_move_w(&rt->m68k_regs,0,tier);
 if(tier&255u){unsigned shift=(tier>>1)&63u;offset=shift>=16u?0u:(uint16_t)((unsigned)*n<<shift);wf_r17_m68k_move_w(&rt->m68k_regs,0,(uint16_t)(tier>>1));wf_r17_m68k_move_w(&rt->m68k_regs,1,offset);}
 const uint8_t*w=cpu_data(table+1u+*n+offset,*n),*choices=cpu_data(table+1u,*n);
 if(!w||!choices)return block(tr,0x1dee2u);
 int index=state1_weighted_24cc(rt,w,*n);if(index<0)return block(tr,0x24ccu);
 uint8_t sel=choices[index];wf_set_u8(a,WF_A_SELECTOR_LO,sel);
 if(sel==255u){wf_set_be16(a,WF_A_STATE,1);wf_set_u8(a,WF_A_CPU_B9,(uint8_t)(wf_u8(a,WF_A_CPU_B9)|2u));wf_set_be16(a,WF_A_AE,0);wf_set_be16(a,WF_A_SELECTOR,0);return WF_OK;}
 if(sel==253u){wf_set_be16(a,WF_A_SELECTOR,0);return WF_OK;}
 if(sel==254u){
  unsigned x=wf_be16(t,WF_A_X);
  if((wf_u8(t,WF_A_FACING)&128u)?x<0x270u:x>=0x2a0u){wf_set_be16(a,WF_A_STATE,5);wf_set_be16(a,WF_A_SELECTOR,0x3e);wf_r17_m68k_moveq(&rt->m68k_regs,0,1);return wf_state5_execute(rt,slot,tr?&tr->state5:NULL);}
  wf_set_u8(a,WF_A_CPU_B5,(uint8_t)(wf_u8(a,WF_A_CPU_B5)&~32u));wf_set_be16(a,WF_A_STATE,1);wf_set_be16(a,WF_A_AE,5);wf_r17_m68k_moveq(&rt->m68k_regs,0,0);return WF_OK;
 }
 if(sel==0x3eu){
  uint16_t x=wf_be16(a,WF_A_X),distance=wf_be16(a,WF_A_CPU_DISTANCE),delta=(uint16_t)(0xc0u-distance);
  wf_r17_m68k_move_w(&rt->m68k_regs,0,x);wf_r17_m68k_move_w(&rt->m68k_regs,1,distance);wf_r17_m68k_move_w(&rt->m68k_regs,2,delta);
  wf_set_be16(a,WF_A_X,(uint16_t)(x+((wf_u8(a,WF_A_FACING)&128u)?-delta:delta)));
  wf_status_t status=wf_r17_resolver_280dc(rt,a);wf_set_be16(a,WF_A_X,x);if(status!=WF_OK)return block(tr,0x280dcu);
  if(wf_u8(a,WF_A_CONTACT+1u)){wf_set_u8(a,WF_A_CPU_B5,(uint8_t)(wf_u8(a,WF_A_CPU_B5)&~32u));wf_set_be16(a,WF_A_STATE,1);wf_set_be16(a,WF_A_AE,5);wf_r17_m68k_moveq(&rt->m68k_regs,0,1);return WF_OK;}
  wf_r17_m68k_moveq(&rt->m68k_regs,0,0);
 }
 wf_set_u8(a,WF_A_SELECTOR,0);wf_set_be16(a,WF_A_STATE,5);
 wf_set_u8(a,WF_A_CPU_B5,(uint8_t)(wf_u8(a,WF_A_CPU_B5)&~16u));wf_set_u8(a,WF_A_CPU_B7,(uint8_t)(wf_u8(a,WF_A_CPU_B7)&~64u));
 return wf_state5_execute(rt,slot,tr?&tr->state5:NULL);
}
/* $1DE66 and the state-4 target branches at $1CBF8. */
static wf_status_t decision_1de66(wf_runtime_t*rt,unsigned slot,wf_actor_raw_t*a,unsigned lane,wf_cpu_trace_t*tr){
 wf_actor_raw_t*t=ptr(rt,a,WF_A_TARGET);if(!t)return block(tr,0x1de74u);
 if(!lane&&!(wf_u8(a,WF_A_FLAGS33)&1u)&&(wf_u8(t,WF_A_FLAGS32)&1u)){
  wf_set_u8(a,WF_A_FLAGS34,(uint8_t)(wf_u8(a,WF_A_FLAGS34)|2u));wf_set_be16(a,WF_A_STATE,0);return WF_OK;
 }
 if(!rt->g.credited_route&&!(rt->g.mode_0161&1u)){
  wf_set_be16(a,WF_A_STATE,1);wf_set_u8(a,WF_A_CPU_B9,(uint8_t)(wf_u8(a,WF_A_CPU_B9)|2u));wf_set_be16(a,WF_A_AE,0);wf_set_be16(a,WF_A_SELECTOR,0);return WF_OK;
 }
 return choose_1dea6(rt,slot,a,0x219b4u,lane,tr);
}
static wf_status_t corner_1f1ac(wf_runtime_t*rt,unsigned slot,wf_actor_raw_t*a,unsigned lane,wf_cpu_trace_t*tr){
 wf_actor_raw_t*t=ptr(rt,a,WF_A_TARGET);if(!t)return block(tr,0x1f1c2u);
 if(!(rt->g.mode_0161&2u)&&!(wf_u8(a,WF_A_CPU_B6)&1u)&&wf_be16(t,WF_A_TIER)&&!(wf_u8(a,WF_A_ROLE)&64u)){
  unsigned x=wf_be16(t,WF_A_X),y=wf_be16(t,WF_A_Y),face=(wf_u8(t,WF_A_FACING)&128u)!=0;
  unsigned left=face?0x288u:0x210u,right=face?0x2d8u:0x260u;
  int corner=-1;
  if(x<left){if(y>=0x170u)corner=0;else if(x<(face?0x278u:0x200u)&&y<0x150u)corner=2;}
  else if(x>=right){if(y>=0x170u)corner=1;else if(x>=(face?0x2e0u:0x268u)&&y<0x150u)corner=3;}
  if(corner>=0){
   static const uint8_t weights[12][4]={
    {10,90,30,70},{20,80,40,60},{0,100,20,80},{20,80,45,55},
    {30,70,60,40},{10,90,35,65},{0,100,15,85},{10,90,25,75},
    {15,85,35,65},{25,75,55,45},{15,85,55,45},{10,90,40,60}};
   unsigned id=wf_u8(a,WF_A_INTERNAL_ID);if(id>=12u)return block(tr,0x1f202u);
   wf_set_be16(a,WF_A_CPU_BC,(uint16_t)corner);wf_r17_m68k_moveq(&rt->m68k_regs,0,0);wf_r17_m68k_move_w(&rt->m68k_regs,0,(uint16_t)(id*4u));
   int choice=state1_weighted_24cc(rt,weights[id]+(wf_u8(t,WF_A_TIER_LO)==1u?0u:2u),2u);if(choice<0)return block(tr,0x24ccu);
   if(!choice){wf_set_be16(t,0x9au,(uint16_t)(wf_be16(t,0x9au)+0x100u));wf_set_u8(t,0xabu,(uint8_t)(wf_u8(t,0xabu)+20u));wf_set_be16(a,WF_A_STATE,1);wf_set_be16(a,WF_A_AE,9);return WF_OK;}
  }
 }
 return decision_1de66(rt,slot,a,lane,tr);
}
static wf_status_t knockdown_1cbf8(wf_runtime_t*rt,unsigned slot,wf_actor_raw_t*a,wf_cpu_trace_t*tr){
 wf_actor_raw_t*t=ptr(rt,a,WF_A_TARGET);if(!t)return block(tr,0x1cbf8u);
 unsigned sub=wf_u8(t,WF_A_STATE4_LO),lane=0;
 if(sub==0u||sub==10u)return decision_1de66(rt,slot,a,0,tr);
 lane=sub==8u?2u:3u;wf_r17_m68k_moveq(&rt->m68k_regs,2,(int8_t)lane);
 if(sub==8u||sub==9u){
  int carry;wf_status_t s=arbitrate_1f7f0(rt,a,&carry,tr);if(s!=WF_OK)return s;
  if(carry){
   if((rt->g.mode_0161&2u)||wf_be16(t,WF_A_STAMINA))return corner_1f1ac(rt,slot,a,lane,tr);
   wf_set_be16(a,WF_A_SELECTOR,0x48);wf_set_be16(a,WF_A_STATE,5);wf_set_u8(a,WF_A_CPU_B5,(uint8_t)(wf_u8(a,WF_A_CPU_B5)&~16u));return wf_state5_execute(rt,slot,tr?&tr->state5:NULL);
  }
  wf_set_be16(a,WF_A_SELECTOR,wf_be16(a,0x100));wf_set_be16(a,WF_A_STATE,5);wf_set_u8(a,WF_A_CPU_B5,(uint8_t)(wf_u8(a,WF_A_CPU_B5)&~16u));wf_set_u8(a,WF_A_CPU_B7,(uint8_t)(wf_u8(a,WF_A_CPU_B7)&~64u));return wf_state5_execute(rt,slot,tr?&tr->state5:NULL);
 }
 if(sub==1u){
  wf_r17_m68k_move_b(&rt->m68k_regs,0,wf_u8(a,WF_A_FACING));
  if(wf_u8(a,WF_A_FACING)!=wf_u8(t,WF_A_FACING)){wf_r17_m68k_moveq(&rt->m68k_regs,2,5);return decision_1de66(rt,slot,a,5,tr);}
  int carry;wf_status_t s=arbitrate_1f7f0(rt,a,&carry,tr);if(s!=WF_OK)return s;
  if(carry){wf_r17_m68k_moveq(&rt->m68k_regs,2,7);return decision_1de66(rt,slot,a,7,tr);}
  wf_set_be16(a,WF_A_SELECTOR,wf_be16(a,0x100));wf_set_be16(a,WF_A_STATE,5);wf_set_u8(a,WF_A_CPU_B5,(uint8_t)(wf_u8(a,WF_A_CPU_B5)&~16u));return wf_state5_execute(rt,slot,tr?&tr->state5:NULL);
 }
 wf_set_u8(a,WF_A_CPU_B7,(uint8_t)(wf_u8(a,WF_A_CPU_B7)|64u));wf_set_be16(a,WF_A_STATE,5);wf_set_be16(a,WF_A_SELECTOR,0x3d);return WF_OK;
}
/* $1D5A2 approach continuation shared with state-5 callbacks. */
wf_status_t wf_cpu_pursuit_1d5a2(wf_runtime_t*rt,unsigned slot,wf_state5_trace_t*tr){
 wf_actor_raw_t*a=&rt->actors[slot],*t=ptr(rt,a,WF_A_TARGET);
 if(!t){if(tr)tr->blocked_dependency_rom=0x1d5b2u;return WF_BLOCKED;}
 if(wf_u8(a,WF_A_FLAGS34)&2u){if(tr)tr->blocked_dependency_rom=0x1c5fau;return WF_BLOCKED;}
 uint16_t x=wf_be16(t,WF_A_X),offset=0;unsigned ts=wf_main_state(t),sub=wf_u8(t,WF_A_STATE4_LO),sel=wf_u8(a,WF_A_SELECTOR_LO);
 if(ts==4u)offset=sub==8u?64u:sub==9u?16u:0u;
 if(ts==5u)offset=wf_u8(t,WF_A_SELECTOR_LO)==0x22u?48u:wf_u8(t,WF_A_SELECTOR_LO)==0x52u?16u:0u;
 x=(uint16_t)(x+((wf_u8(t,WF_A_FACING)&128u)?-offset:offset));
 wf_set_u8(a,WF_A_FACING,(uint8_t)((wf_u8(a,WF_A_FACING)&127u)|(x>=wf_be16(a,WF_A_X)?128u:0u)));
 if(sel==0x70u){if(tr)tr->blocked_dependency_rom=0x1d6eeu;return WF_BLOCKED;}
 if(sel!=0x69u){
  if(wf_u8(a,WF_A_CPU_B5)&32u){
   if(wf_u8(t,WF_A_FLAGS33)&16u){if(tr)tr->blocked_dependency_rom=0x1d5e2u;return WF_BLOCKED;}
   if(!(wf_u8(a,WF_A_CPU_B7)&32u)){
    if((sel==0x1cu||sel==0x1du)&&!(ts==4u&&sub==1u)){
     wf_set_be16(a,WF_A_CPU_BC,0);wf_set_u8(a,WF_A_CPU_B5,(uint8_t)(wf_u8(a,WF_A_CPU_B5)&~32u));wf_set_u8(a,WF_A_CPU_B7,(uint8_t)(wf_u8(a,WF_A_CPU_B7)&~32u));wf_set_u8(a,WF_A_CPU_B6,(uint8_t)(wf_u8(a,WF_A_CPU_B6)|128u));rt->g.global_1697|=1u;wf_set_be16(a,WF_A_STATE,0);wf_set_be16(a,WF_A_AE,0);return WF_OK;
    }
    wf_set_u8(a,WF_A_ANGLE,(wf_u8(a,WF_A_CPU_B5)&1u)?0x40u:0xc0u);
    uint16_t distance=wf_be16(a,WF_A_CPU_DISTANCE);wf_r17_m68k_move_w(&rt->m68k_regs,0,distance);
    if(distance>=wf_be16(a,WF_A_CPU_BC))return WF_OK;
    wf_set_be16(a,WF_A_CPU_BC,0);wf_set_u8(a,WF_A_CPU_B5,(uint8_t)(wf_u8(a,WF_A_CPU_B5)&~32u));wf_set_u8(a,WF_A_CPU_B7,(uint8_t)(wf_u8(a,WF_A_CPU_B7)&~32u));wf_set_be16(a,WF_A_STATE,5);wf_set_u8(a,WF_A_SELECTOR,0);return wf_state5_execute(rt,slot,tr);
   }
  }else{
   unsigned tsel=wf_u8(t,WF_A_SELECTOR_LO);
   if(!(wf_u8(t,0x35)&3u)&&!(ts==5u&&(tsel==0x61u||tsel==0x4au))&&!(ts==4u&&(sub==8u||sub==9u))){
    wf_set_u8(a,WF_A_CPU_B5,(uint8_t)(wf_u8(a,WF_A_CPU_B5)&~32u));wf_set_u8(a,WF_A_CPU_B7,(uint8_t)(wf_u8(a,WF_A_CPU_B7)&~32u));wf_set_be16(a,WF_A_STATE,0);return WF_OK;
   }
   if(wf_u8(a,WF_A_CONTACT+1u)){
    wf_set_u8(a,WF_A_CONTACT+1u,0);
    if(sel==0x3du)goto arrived;
    if(rt->g.credited_route){if(tr)tr->blocked_dependency_rom=0x1d6e4u;return WF_BLOCKED;}
   }
  }
 }
 /* $1F15C preserves D1-D7; $20C8 emits the quadrant angle. */
 uint16_t ax=wf_be16(a,WF_A_X),ay=wf_be16(a,WF_A_Y),px=wf_be16(a,WF_A_CPU_BE),py=wf_be16(a,WF_A_CPU_C0);
 unsigned dx=ax>=px?ax-px:px-ax,dy=ay>=py?ay-py:py-ay;
 if(dx>=8u||dy>=8u){
  uint16_t sx=(uint16_t)(px-ax),sy=(uint16_t)(py-ay);unsigned quadrant=0;
  if(sx&0x8000u){quadrant|=1u;sx=(uint16_t)-sx;}if(sy&0x8000u){quadrant|=2u;sy=(uint16_t)-sy;}
  if(sx>255u)sx=255u;if(sy>255u)sy=255u;
  static const uint8_t base[4]={0,192,64,128};unsigned sum=sx+sy,component=(quadrant==0u||quadrant==3u)?sx:sy;
  wf_set_u8(a,WF_A_ANGLE,(uint8_t)(base[quadrant]+component*64u/(sum?sum:1u)));wf_r17_m68k_moveq(&rt->m68k_regs,0,1);return WF_OK;
 }
 wf_r17_m68k_moveq(&rt->m68k_regs,0,0);
arrived:
 wf_set_be16(a,WF_A_STATE,5);wf_set_u8(a,WF_A_CPU_B5,(uint8_t)((wf_u8(a,WF_A_CPU_B5)|16u)&~32u));wf_set_u8(a,WF_A_CPU_B7,(uint8_t)(wf_u8(a,WF_A_CPU_B7)&~32u));wf_set_u8(a,WF_A_SELECTOR,0);return wf_state5_execute(rt,slot,tr);
}
/* $1F5B2: scan all sixteen opponent actions before choosing a defender action. */
static wf_status_t assist_1f5b2(wf_runtime_t*rt,unsigned slot,wf_actor_raw_t*a,wf_cpu_trace_t*tr){
 static const uint8_t selectors[16]={0x53u,0x61u,9u,0x22u,0x3bu,0x36u,0x1cu,0x25u,0x1du,0x48u,0xfu,0xeu,0x1au,0x23u,0x77u,0x37u};
 wf_set_u8(a,WF_A_CPU_B9,(uint8_t)(wf_u8(a,WF_A_CPU_B9)&~2u));wf_r17_m68k_moveq(&rt->m68k_regs,0,0);wf_r17_m68k_moveq(&rt->m68k_regs,2,15);
 wf_actor_raw_t*t=ptr(rt,a,WF_A_TARGET);if(!t)return block(tr,0x1f5bcu);
 unsigned index=16u;
 if(wf_main_state(t)==5u){
  uint8_t target_sel=wf_u8(t,WF_A_SELECTOR_LO);
  if(target_sel==0x4au){if(rt->g.mode_0161&1u)return block(tr,0x1f5dau);}
  else for(unsigned i=0;i<16u;i++){
   wf_r17_m68k_move_b(&rt->m68k_regs,3,target_sel);
   if(target_sel==selectors[i]){index=i;break;}
   wf_r17_m68k_move_b(&rt->m68k_regs,0,(uint8_t)((uint8_t)rt->m68k_regs.d[0]+1u));wf_r17_m68k_move_w(&rt->m68k_regs,2,(uint16_t)((uint16_t)rt->m68k_regs.d[2]-1u));
  }
 }
 if(index==16u){rt->g.global_1697|=1u;wf_set_u8(a,WF_A_CPU_B6,(uint8_t)(wf_u8(a,WF_A_CPU_B6)|128u));wf_set_u8(a,WF_A_CPU_B5,(uint8_t)(wf_u8(a,WF_A_CPU_B5)&127u));wf_set_be16(a,WF_A_STATE,0u);return WF_OK;}
 unsigned id=wf_u8(a,WF_A_INTERNAL_ID);uint32_t table;if(id>=12u)return block(tr,0x1f624u);
 wf_r17_m68k_moveq(&rt->m68k_regs,1,0);wf_r17_m68k_move_b(&rt->m68k_regs,1,(uint8_t)id);wf_r17_m68k_move_w(&rt->m68k_regs,1,(uint16_t)(id*4u));
 if(!cpu_pointer(0x233f0u+(uint16_t)rt->m68k_regs.d[1],&table))return block(tr,0x1f624u);
 wf_r17_m68k_move_w(&rt->m68k_regs,0,(uint16_t)(index*4u));if(!cpu_pointer(table+(uint16_t)rt->m68k_regs.d[0],&table))return block(tr,0x1f62au);
 const uint8_t*n=cpu_data(table,1u);if(!n||!*n)return block(tr,0x1f62eu);wf_r17_m68k_move_b(&rt->m68k_regs,1,*n);
 uint32_t weights_at=table+1u+rt->m68k_regs.d[1];uint16_t tier=wf_be16(t,WF_A_TIER);wf_r17_m68k_move_w(&rt->m68k_regs,0,tier);
 if(tier&255u){unsigned shift=(tier>>1)&63u;wf_r17_m68k_move_w(&rt->m68k_regs,0,(uint16_t)(tier>>1));uint16_t offset=shift>=16u?0u:(uint16_t)((uint16_t)rt->m68k_regs.d[1]<<shift);wf_r17_m68k_move_w(&rt->m68k_regs,1,offset);weights_at+=rt->m68k_regs.d[1];}
 const uint8_t*choices=cpu_data(table+1u,*n),*weights=cpu_data(weights_at,*n);if(!choices||!weights)return block(tr,0x1f648u);
 int choice=state1_weighted_24cc(rt,weights,*n);if(choice<0)return block(tr,0x24ccu);wf_set_u8(a,WF_A_SELECTOR_LO,choices[choice]);
 if(choices[choice]==0x3eu){uint16_t x=wf_be16(a,WF_A_X);wf_r17_m68k_move_w(&rt->m68k_regs,0,x);wf_set_be16(a,WF_A_X,(uint16_t)(x+((wf_u8(a,WF_A_FACING)&128u)?-184:184)));wf_status_t st=wf_r17_resolver_280dc(rt,a);wf_set_be16(a,WF_A_X,x);if(st!=WF_OK)return block(tr,0x280dcu);if(wf_u8(a,WF_A_CONTACT+1u))wf_set_u8(a,WF_A_SELECTOR_LO,0u);}
 wf_set_u8(a,WF_A_SELECTOR,0u);uint8_t target_sel=wf_u8(t,WF_A_SELECTOR_LO);int same=wf_u8(a,WF_A_FACING)==wf_u8(t,WF_A_FACING);
 if(target_sel==0x1du||target_sel==0x1cu||target_sel==0x53u){
  wf_r17_m68k_move_b(&rt->m68k_regs,0,wf_u8(a,WF_A_FACING));
  if((target_sel==0x53u)?same:!same){
   uint16_t x=wf_be16(a,WF_A_X),y=wf_be16(a,WF_A_Y),tx=(uint16_t)(wf_be16(t,WF_A_X)+32u);wf_r17_m68k_move_w(&rt->m68k_regs,0,x);wf_r17_m68k_move_w(&rt->m68k_regs,1,y);wf_r17_m68k_move_w(&rt->m68k_regs,2,tx);wf_set_be16(a,WF_A_X,tx);
   if(!(wf_u8(a,WF_A_CPU_B5)&1u)){tx=(uint16_t)(tx-64u);wf_r17_m68k_move_w(&rt->m68k_regs,2,tx);wf_set_be16(a,WF_A_X,tx);}wf_set_be16(a,WF_A_CONTACT,0u);wf_status_t st=wf_r17_resolver_280dc(rt,a);wf_set_be16(a,WF_A_X,x);wf_set_be16(a,WF_A_Y,y);if(st!=WF_OK)return block(tr,0x280dcu);
   if(!wf_u8(a,WF_A_CONTACT+1u)){wf_set_u8(a,WF_A_CPU_B5,(uint8_t)(wf_u8(a,WF_A_CPU_B5)|32u));wf_set_be16(a,WF_A_STATE,1u);wf_set_be16(a,WF_A_AE,5u);return WF_OK;}
  }
 }
 wf_set_u8(a,WF_A_CPU_B5,(uint8_t)(wf_u8(a,WF_A_CPU_B5)&~16u));wf_set_be16(a,WF_A_STATE,5u);return wf_state5_execute(rt,slot,tr?&tr->state5:NULL);
}

/* $1C228: unsigned WORD differences, including the target-pose offsets.
 * D0/D1 are deliberately caller-visible and feed the live ROM RNG. */
static void defender_relation_1c228(wf_runtime_t*rt,wf_actor_raw_t*a,const wf_actor_raw_t*t){
 wf_set_u8(a,WF_A_CPU_B5,(uint8_t)(wf_u8(a,WF_A_CPU_B5)&0xfcu));
 uint16_t x=wf_be16(t,WF_A_X),offset=0u;
 wf_r17_m68k_move_w(&rt->m68k_regs,0,x);
 if(wf_main_state(t)==4u){unsigned sub=wf_u8(t,WF_A_STATE4_LO);offset=sub==8u?64u:sub==9u?40u:0u;}
 if(offset){wf_r17_m68k_move_w(&rt->m68k_regs,1,offset);x=(uint16_t)(x+((wf_u8(t,WF_A_FACING)&128u)?-offset:offset));}
 uint16_t own=wf_be16(a,WF_A_X),dx=(uint16_t)(x-own);if(x<own)dx=(uint16_t)(0u-dx);else wf_set_u8(a,WF_A_CPU_B5,(uint8_t)(wf_u8(a,WF_A_CPU_B5)|1u));
 wf_r17_m68k_move_w(&rt->m68k_regs,0,dx);wf_set_be16(a,WF_A_CPU_DISTANCE,dx);
 uint16_t y=wf_be16(a,WF_A_Y);if(wf_main_state(t)==5u&&wf_u8(t,WF_A_SELECTOR_LO)==0x22u)y=(uint16_t)(y+8u);
 uint16_t ty=wf_be16(t,WF_A_Y),dy=(uint16_t)(y-ty);if(y<ty){dy=(uint16_t)(0u-dy);wf_set_u8(a,WF_A_CPU_B5,(uint8_t)(wf_u8(a,WF_A_CPU_B5)|2u));}
 wf_r17_m68k_move_w(&rt->m68k_regs,1,dy);wf_set_be16(a,WF_A_CPU_DISTANCE_Y,dy);wf_set_u8(a,WF_A_CPU_B4,dx<80u?0u:dx<176u?1u:2u);
}
static wf_status_t defender_fallback_1c880(wf_runtime_t*rt,wf_actor_raw_t*a,wf_cpu_trace_t*tr){
 wf_actor_raw_t*t=ptr(rt,a,WF_A_TARGET);if(!t)return block(tr,0x1c880u);
 if(wf_main_state(t)==4u){unsigned sub=wf_u8(t,WF_A_STATE4_LO);if(sub!=6u&&sub!=7u&&sub!=8u&&sub!=9u)wf_set_u8(a,WF_A_CPU_B6,(uint8_t)(wf_u8(a,WF_A_CPU_B6)&~1u));}
 if((rt->g.mode_0161&1u)&&(!(wf_u8(t,WF_A_ACTIVE)&128u)||(wf_u8(t,WF_A_FLAGS32)&16u))){wf_set_u8(a,WF_A_CPU_B6,(uint8_t)(wf_u8(a,WF_A_CPU_B6)|128u));return WF_OK;}
 rt->g.global_1697|=1u;wf_set_be16(a,WF_A_SELECTOR,0u);wf_set_be16(a,WF_A_STATE,1u);wf_set_be16(a,WF_A_AE,0u);return WF_OK;
}
static wf_status_t defender_1c7ec(wf_runtime_t*rt,wf_actor_raw_t*a,wf_cpu_trace_t*tr){
 uint32_t requested=wf_be32(a,0x7eu);
 if(requested){
  wf_actor_raw_t*t=ptr(rt,a,0x7eu);if(!t)return block(tr,0x1c7f4u);
  int reject=(wf_u8(t,WF_A_FLAGS32)&16u)||!(wf_u8(t,WF_A_ACTIVE)&128u);
  if(!reject&&(wf_u8(t,0x35u)&1u)&&(wf_u8(a,WF_A_ROLE)&128u)){
   unsigned stage=rt->g.progression_0162,tier=wf_be16(a,WF_A_TIER);uint32_t table;if(stage>=10u||tier>=3u)return block(tr,0x1c828u);
   wf_r17_m68k_moveq(&rt->m68k_regs,0,0);wf_r17_m68k_move_w(&rt->m68k_regs,0,(uint16_t)(stage*4u));
   if(!cpu_pointer(0x24010u+(uint16_t)rt->m68k_regs.d[0],&table))return block(tr,0x1c828u);
   wf_r17_m68k_move_w(&rt->m68k_regs,0,(uint16_t)(tier*2u));const uint8_t*w=cpu_data(table+rt->m68k_regs.d[0],2u);if(!w)return block(tr,0x1c832u);
   int choice=state1_weighted_24cc(rt,w,2u);if(choice<0)return block(tr,0x24ccu);reject=choice!=0;
  }
  if(reject){wf_set_be32(a,0x7eu,0u);wf_set_u8(a,WF_A_CPU_B5,(uint8_t)(wf_u8(a,WF_A_CPU_B5)&127u));return WF_OK;}
  wf_set_be32(a,WF_A_TARGET,requested);defender_relation_1c228(rt,a,t);cpu_face_10be8(a,t);
  if((wf_u8(a,WF_A_FLAGS33)&4u)||!(wf_u8(t,WF_A_FLAGS33)&4u)){wf_set_be16(a,WF_A_SELECTOR,0u);wf_set_be16(a,WF_A_STATE,1u);wf_set_be16(a,WF_A_AE,0u);}return WF_OK;
 }
 return defender_fallback_1c880(rt,a,tr);
}
/* $1C2CC prefix must run before any special defender route is selected. */
static wf_status_t state0_defender_prefix(wf_runtime_t*rt,wf_actor_raw_t*a,wf_cpu_trace_t*tr){
 wf_actor_raw_t*t=ptr(rt,a,WF_A_TARGET);if(!t)return block(tr,0x10be8u);cpu_face_10be8(a,t);
 wf_set_be16(a,WF_A_AE,0u);wf_set_be16(a,WF_A_TIMER44,0u);wf_set_be16(a,WF_A_TIMER46,0u);wf_set_be16(a,WF_A_FLAGS32,(uint16_t)(wf_be16(a,WF_A_FLAGS32)&0xfba7u));wf_set_u8(a,WF_A_CPU_B5,(uint8_t)(wf_u8(a,WF_A_CPU_B5)&0xcbu));wf_set_be16(a,WF_A_CPU_B6,(uint16_t)(wf_be16(a,WF_A_CPU_B6)&0xf1fdu));wf_set_u8(a,WF_A_CPU_B7,(uint8_t)(wf_u8(a,WF_A_CPU_B7)&~32u));
 if(rt->g.mode_0161&1u){uint8_t old=wf_u8(a,WF_A_CPU_B6);wf_set_u8(a,WF_A_CPU_B6,(uint8_t)(old&~64u));if((old&64u)&&!(wf_u8(a,WF_A_CPU_B5)&128u)){wf_set_u8(a,WF_A_CPU_B6,(uint8_t)(wf_u8(a,WF_A_CPU_B6)|128u));return WF_OK;}}
 if(wf_u8(a,WF_A_CPU_B6)&16u){if(rt->g.mode_0161&2u){wf_set_u8(a,WF_A_CPU_B6,(uint8_t)(wf_u8(a,WF_A_CPU_B6)&~16u));wf_set_be16(a,WF_A_STATE,5u);wf_set_be16(a,WF_A_SELECTOR,0x6cu);}return WF_OK;}
 if((wf_u8(a,WF_A_FLAGS34)&16u)&&!(wf_u8(a,WF_A_CPU_B5)&8u)){wf_set_be16(a,WF_A_STATE,5u);wf_set_be16(a,WF_A_SELECTOR,0x79u);return WF_OK;}
 if(!(wf_u8(a,WF_A_FLAGS34)&4u)){
  uint8_t old=wf_u8(a,WF_A_CPU_B7);wf_set_u8(a,WF_A_CPU_B7,(uint8_t)(old&~1u));if(old&1u){wf_set_be16(a,WF_A_STATE,5u);return WF_OK;}
  old=wf_u8(a,WF_A_CPU_B7);wf_set_u8(a,WF_A_CPU_B7,(uint8_t)(old&~16u));if(old&16u){wf_set_be16(a,WF_A_SELECTOR,wf_be16(a,WF_A_CPU_BC));wf_set_be16(a,WF_A_STATE,5u);return WF_OK;}
  if(wf_u8(a,WF_A_CPU_B7)&128u)return block(tr,0x1c390u);
 }
 return (wf_u8(a,WF_A_CPU_B5)&128u)?defender_1c7ec(rt,a,tr):defender_fallback_1c880(rt,a,tr);
}

/* $1C5EC normal TAG continuation after the running-target branch. */
static wf_status_t state0_normal(wf_runtime_t*rt,wf_actor_raw_t*a,wf_cpu_trace_t*tr){
 wf_actor_raw_t*t=ptr(rt,a,WF_A_TARGET);if(!t)return block(tr,0x1c648u);
 if(wf_u8(a,WF_A_FLAGS34)&2u){
  if(!(wf_u8(a,WF_A_FLAGS33)&1u)){wf_set_be16(a,WF_A_STATE,1);wf_set_be16(a,WF_A_AE,4);return WF_OK;}
  wf_actor_raw_t*p=ptr(rt,a,WF_A_TEAM_LINK);if(!p)return block(tr,0x1c608u);
  wf_set_u8(a,WF_A_FLAGS34,(uint8_t)(wf_u8(a,WF_A_FLAGS34)&~2u));wf_set_u8(p,WF_A_FLAGS34,(uint8_t)(wf_u8(p,WF_A_FLAGS34)|2u));
  if(wf_u8(a,WF_A_FLAGS33)&128u)rt->g.global_1684=wf_be32(a,WF_A_TEAM_LINK);else rt->g.global_1688=wf_be32(a,WF_A_TEAM_LINK);
 }
 if(wf_u8(a,WF_A_CPU_B5)&64u){
  if(!wf_be16(t,WF_A_STAMINA))wf_set_u8(a,WF_A_CPU_B5,(uint8_t)(wf_u8(a,WF_A_CPU_B5)&~64u));
  else if(!(wf_main_state(t)==4u&&wf_u8(t,WF_A_STATE4_LO)==1u)){
   wf_actor_raw_t*p=ptr(rt,a,WF_A_TEAM_LINK);if(!p)return block(tr,0x1c66au);
   if((wf_main_state(p)==1u&&(wf_u8(p,WF_A_CPU_SUBSTATE)==1u||wf_u8(p,WF_A_CPU_SUBSTATE)==2u))||(wf_main_state(p)==5u&&wf_u8(p,WF_A_SELECTOR_LO)==0x71u)){
    if(wf_main_state(p)==5u){wf_set_u8(p,0xc6u,(uint8_t)(wf_u8(p,0xc6u)&~64u));wf_set_u8(p,WF_A_FLAGS33,(uint8_t)(wf_u8(p,WF_A_FLAGS33)|32u));}
    wf_set_be16(a,WF_A_STATE,1);wf_set_be16(a,WF_A_AE,8);return WF_OK;
   }
  }
 }else{
  int request=0;
  if(!(wf_u8(a,WF_A_ROLE)&64u)&&rt->g.credited_route&&!(rt->g.mode_0161&3u)&&!(wf_u8(a,WF_A_FLAGS33)&4u)&&(wf_u8(a,WF_A_FLAGS33)&1u)){
   wf_actor_raw_t*p=ptr(rt,a,WF_A_TEAM_LINK);if(!p)return block(tr,0x1f43eu);
   uint16_t stamina=(uint16_t)(wf_be16(a,WF_A_STAMINA)+12u);wf_r17_m68k_move_w(&rt->m68k_regs,0,stamina);request=stamina<wf_be16(p,WF_A_STAMINA);
  }
  wf_set_u8(a,WF_A_CPU_B5,(uint8_t)((wf_u8(a,WF_A_CPU_B5)&~64u)|(request?64u:0u)));
 }
 if(rt->g.mode_0161&2u)return block(tr,0x1c6b6u);
 if((wf_u8(a,WF_A_CPU_B5)&128u)&&wf_be32(a,0x7eu))return block(tr,0x1c7ecu);
 if(wf_main_state(t)==4u){unsigned sub=wf_u8(t,WF_A_STATE4_LO);if(sub!=6u&&sub!=7u&&sub!=8u&&sub!=9u)wf_set_u8(a,WF_A_CPU_B6,(uint8_t)(wf_u8(a,WF_A_CPU_B6)&~1u));}
 if((rt->g.mode_0161&1u)&&(!(wf_u8(t,0)&128u)||(wf_u8(t,WF_A_FLAGS32)&16u))){wf_set_u8(a,WF_A_CPU_B6,(uint8_t)(wf_u8(a,WF_A_CPU_B6)|128u));return WF_OK;}
 rt->g.global_1697|=1u;wf_set_be16(a,WF_A_SELECTOR,0);wf_set_be16(a,WF_A_STATE,1);wf_set_be16(a,WF_A_AE,0);return WF_OK;
}
static wf_status_t state0_tail(wf_runtime_t*rt,unsigned slot,wf_actor_raw_t*a,wf_cpu_trace_t*tr){
 wf_actor_raw_t*t=ptr(rt,a,WF_A_TARGET);if(!t)return block(tr,0x1c404u);
 if(wf_u8(a,WF_A_CPU_B9)&1u){
  if(wf_u8(t,WF_A_FLAGS33)&64u){wf_set_u8(a,WF_A_CPU_B6,(uint8_t)(wf_u8(a,WF_A_CPU_B6)|128u));return WF_OK;}
  wf_set_u8(a,WF_A_CPU_B9,(uint8_t)(wf_u8(a,WF_A_CPU_B9)&~1u));
 }
 if(wf_u8(t,WF_A_FLAGS34)&16u){wf_set_u8(a,WF_A_CPU_B6,(uint8_t)(wf_u8(a,WF_A_CPU_B6)|128u));return WF_OK;}
 if(!(wf_u8(a,WF_A_CPU_B5)&8u))return state0_normal(rt,a,tr);
 if(!(wf_u8(t,WF_A_FLAGS33)&16u))return block(tr,0x1c458u);
 if(rt->g.credited_route&&!(wf_u8(a,WF_A_FLAGS33)&4u)&&!(wf_u8(a,WF_A_CPU_B7)&4u)){
  wf_set_u8(a,WF_A_CPU_B7,(uint8_t)(wf_u8(a,WF_A_CPU_B7)|4u));
  unsigned spatial=wf_u8(a,0x31u);
  if((spatial<3u&&(wf_u8(a,WF_A_FACING)&128u))||(spatial>=9u&&!(wf_u8(a,WF_A_FACING)&128u))){
   static const uint8_t weights[2]={20u,80u}; /* ROM $1F3BE */
   int choice=state1_weighted_24cc(rt,weights,2u);if(choice<0)return block(tr,0x24ccu);
   if(!choice){
    wf_set_u8(a,WF_A_CPU_B5,(uint8_t)(wf_u8(a,WF_A_CPU_B5)&~8u));wf_set_be16(a,WF_A_STATE,2u);
    wf_set_u8(a,WF_A_ANGLE,(wf_u8(a,WF_A_FACING)&128u)?0x40u:0xc0u);
    wf_set_u8(a,WF_A_FLAGS32,(uint8_t)(wf_u8(a,WF_A_FLAGS32)|4u));
    wf_set_u8(a,WF_A_ANGLE,(uint8_t)(wf_u8(a,WF_A_ANGLE)^128u));wf_set_u8(a,WF_A_FACING,(uint8_t)(wf_u8(a,WF_A_FACING)^128u));
    wf_set_u8(a,WF_A_CPU_B7,(uint8_t)(wf_u8(a,WF_A_CPU_B7)&~2u));return WF_OK;
   }
  }
 }
 if(!(wf_u8(a,WF_A_CPU_B7)&8u)){
  int carry;wf_status_t st=arbitrate_1f7f0(rt,a,&carry,tr);if(st!=WF_OK)return st;
  if(!carry)return block(tr,0x1c4f4u);
  wf_r17_m68k_move_b(&rt->m68k_regs,0,wf_u8(a,WF_A_FACING));
  if(wf_u8(a,WF_A_FACING)==wf_u8(t,WF_A_FACING))return WF_OK;
  wf_set_u8(a,WF_A_CPU_B7,(uint8_t)(wf_u8(a,WF_A_CPU_B7)|8u));
  unsigned distance=wf_be16(a,WF_A_CPU_DISTANCE),lane=distance<0x68u?0u:distance<0xb0u?1u:2u;
  wf_r17_m68k_moveq(&rt->m68k_regs,2,(int8_t)lane);
  if(!lane)wf_set_u8(a,WF_A_CPU_B5,(uint8_t)(wf_u8(a,WF_A_CPU_B5)&~8u));
  else wf_set_u8(a,WF_A_FLAGS33,(uint8_t)(wf_u8(a,WF_A_FLAGS33)&~16u));
  st=choose_1dea6(rt,slot,a,0x2307cu,lane,tr);if(st!=WF_OK)return st;
  if(lane){
   wf_set_u8(a,WF_A_SELECTOR,0);wf_set_be16(a,WF_A_STATE,0);
   if(wf_u8(a,WF_A_FLAGS33)&16u){wf_set_be16(a,WF_A_STATE,2);wf_set_u8(a,WF_A_ANGLE,(wf_u8(a,WF_A_FACING)&128u)?0x40u:0xc0u);wf_set_u8(a,WF_A_CPU_B7,(uint8_t)(wf_u8(a,WF_A_CPU_B7)&~2u));return WF_OK;}
  }
 }
 wf_r17_m68k_move_b(&rt->m68k_regs,0,wf_u8(a,WF_A_FACING));
 if(wf_u8(a,WF_A_FACING)==wf_u8(t,WF_A_FACING))return WF_OK;
 uint16_t distance=wf_be16(a,WF_A_CPU_DISTANCE);wf_r17_m68k_move_w(&rt->m68k_regs,0,distance);
 if(distance>=wf_be16(a,WF_A_CPU_BC))return WF_OK;
 wf_set_u8(a,WF_A_CPU_B5,(uint8_t)((wf_u8(a,WF_A_CPU_B5)&~8u)|16u));wf_set_u8(a,WF_A_SELECTOR,0);wf_set_be16(a,WF_A_STATE,5);
 return wf_state5_execute(rt,slot,tr?&tr->state5:NULL);
}
wf_status_t wf_cpu_continue_1c5b6(wf_runtime_t*rt,unsigned slot,wf_state5_trace_t*tr){
 if(!rt||slot>=WF_ACTOR_SLOTS)return WF_BAD_STATE;
 wf_actor_raw_t*a=&rt->actors[slot],*t=ptr(rt,a,WF_A_TARGET);
 if(!t){if(tr)tr->blocked_dependency_rom=0x1c5b6u;return WF_BLOCKED;}
 wf_r17_m68k_move_b(&rt->m68k_regs,0,wf_u8(a,WF_A_FACING));
 if(wf_u8(a,WF_A_FACING)==wf_u8(t,WF_A_FACING))return WF_OK;
 uint16_t distance=wf_be16(a,WF_A_CPU_DISTANCE);
 wf_r17_m68k_move_w(&rt->m68k_regs,0,distance);
 if(distance>=wf_be16(a,WF_A_CPU_BC))return WF_OK;
 wf_set_u8(a,WF_A_CPU_B5,(uint8_t)((wf_u8(a,WF_A_CPU_B5)&~8u)|16u));
 wf_set_u8(a,WF_A_SELECTOR,0);wf_set_be16(a,WF_A_STATE,5);
 return wf_state5_execute(rt,slot,tr);
}
wf_status_t wf_cpu_actor_step(wf_runtime_t *rt,unsigned slot,wf_cpu_trace_t *tr){
 if(tr) memset(tr,0,sizeof(*tr));
 if(!rt||slot>=WF_ACTOR_SLOTS) return WF_BAD_STATE;
 wf_actor_raw_t *a=&rt->actors[slot];
 uint8_t st=wf_main_state(a);
 uint32_t h=arcade_cpu_main_handler(st);
 if(tr){tr->state=st;tr->handler_rom=h;}
 if(!h) return WF_BAD_STATE;
 switch(st){case 0u:return state0(rt,slot,a,tr);case 1u:return state1(rt,slot,a,tr);
 case 4u:return state4(rt,slot,a,tr);case 5u:return wf_state5_execute(rt,slot,tr?&tr->state5:0);case 6u:return state6(rt,slot,tr);case 7u:return WF_OK;case 8u:wf_set_u8(a,WF_A_CPU_BD,0u);return WF_OK;case 9u:return state9(rt,slot,tr);case 0x0au:return WF_OK;case 11u:return state11(rt,a,tr);case 12u:return state12(rt,slot,a,tr);default:return block(tr,h);}
}
