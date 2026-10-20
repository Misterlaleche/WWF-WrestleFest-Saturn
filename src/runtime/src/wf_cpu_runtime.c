#include <string.h>
#include "wf_cpu_runtime.h"
#include "arcade_cpu_final_bridge.h"
static wf_status_t block(wf_cpu_trace_t*t,uint32_t pc){if(t)t->blocked_dependency_rom=pc;return WF_BLOCKED;}
static wf_actor_raw_t *ptr(wf_runtime_t*rt,const wf_actor_raw_t*a,size_t off){return wf_actor_from_rom_ptr(rt,wf_be32(a,off),0);}
/* Direct-ROM $1C2CC fast routes reached by the fresh TAG participants. Other
 * branches remain explicit dependencies instead of guessing AI behavior. */
static wf_status_t state0(wf_runtime_t*rt,wf_actor_raw_t*a,wf_cpu_trace_t*tr){
 if(!(wf_u8(a,WF_A_STATE)&0x80u))return WF_OK;
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
 if(f34&2u){
  if(f33&1u)return block(tr,0x0001c5fau);
  wf_set_be16(a,WF_A_STATE,1u);wf_set_be16(a,WF_A_AE,4u);return WF_OK;
 }
 if((f33&1u)&&!(wf_u8(t,WF_A_FLAGS32)&1u)&&!(wf_u8(a,WF_A_ROLE)&0x40u)&&
    !(f33&4u)&&!(b5&0x40u)&&!(wf_u8(a,WF_A_FLAGS34)&0x80u)){
  /* $1F3D6, $1C7E2 and $1C880: no tagged/ringside branch in stage zero. */
  if(wf_u8(a,WF_A_STATE+1u)==4u&&wf_u8(a,WF_A_STATE4_LO)==1u)return block(tr,0x0001f422u);
  wf_actor_raw_t*partner=ptr(rt,a,WF_A_TEAM_LINK);if(!partner)return block(tr,0x0001f43eu);
  uint16_t d0=(uint16_t)(wf_be16(a,WF_A_STAMINA)+12u);
  wf_r17_m68k_move_w(&rt->m68k_regs,0u,d0);
  if(d0<wf_be16(partner,WF_A_STAMINA))wf_set_u8(a,WF_A_CPU_B5,(uint8_t)(wf_u8(a,WF_A_CPU_B5)|0x40u));
  else wf_set_u8(a,WF_A_CPU_B5,(uint8_t)(wf_u8(a,WF_A_CPU_B5)&~0x40u));
  if(ts!=4u||!(wf_u8(t,WF_A_STATE4_LO)==6u||wf_u8(t,WF_A_STATE4_LO)==7u||wf_u8(t,WF_A_STATE4_LO)==8u||wf_u8(t,WF_A_STATE4_LO)==9u))
   wf_set_u8(a,WF_A_CPU_B6,(uint8_t)(wf_u8(a,WF_A_CPU_B6)&~1u));
  rt->g.global_1697|=1u;wf_set_be16(a,WF_A_SELECTOR,0u);wf_set_be16(a,WF_A_STATE,1u);wf_set_be16(a,WF_A_AE,0u);return WF_OK;
 }
 return block(tr,0x0001c3fcu);
}
/* $219B4 -> per-character table -> tier-zero lane. Each terminal table has
 * two selector bytes {00,FF}; these three weight pairs are the adjacent
 * two-byte rows selected by $1DED6..$1DEE0. */
static const uint8_t state1_219b4[12][3][2]={
 {{35,65},{30,70},{25,75}},{{40,60},{35,65},{30,70}},
 {{35,65},{25,75},{10,90}},{{35,65},{30,70},{25,75}},
 {{30,70},{25,75},{20,80}},{{30,70},{25,75},{20,80}},
 {{35,65},{30,70},{25,75}},{{30,70},{25,75},{20,80}},
 {{35,65},{30,70},{25,75}},{{30,70},{25,75},{20,80}},
 {{35,65},{30,70},{25,75}},{{35,65},{30,70},{25,75}}
};
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
  total+=weights[choice];
  if(total>=sample)break;
 }
 if(choice==count)return -1;
 wf_r17_m68k_move_w(&rt->m68k_regs,0u,(uint16_t)choice);
 return (int)choice;
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
 if(wf_u8(a,WF_A_CPU_B4))return block(tr,0x0001cd5cu);
 if(wf_be16(a,WF_A_CPU_DISTANCE_Y)>=12u)return block(tr,0x0001cdd0u);
 uint8_t f34=wf_u8(a,WF_A_FLAGS34);
 wf_set_u8(a,WF_A_FLAGS34,(uint8_t)(f34&~4u));
 if((f34&4u)&&(wf_u8(a,WF_A_FLAGS33)&2u))
  return block(tr,0x0001cffau);
 if(wf_u8(a,WF_A_REL_FLAGS)&0x80u)return block(tr,0x0001cd2cu);
 if(wf_u8(a,WF_A_CPU_B5)&0x80u)return block(tr,0x0001f5b2u);
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
 if(ts!=0u&&ts!=1u)return block(tr,0x0001cb42u);
 if(!(wf_u8(a,WF_A_FLAGS33)&1u)&&(wf_u8(t,WF_A_FLAGS32)&1u)){
  wf_set_u8(a,WF_A_FLAGS34,(uint8_t)(wf_u8(a,WF_A_FLAGS34)|2u));
  wf_set_be16(a,WF_A_STATE,0u);
  return WF_OK;
 }
 if(rt->g.credited_route==0u){
  wf_set_be16(a,WF_A_STATE,1u);
  wf_set_u8(a,WF_A_CPU_B9,(uint8_t)(wf_u8(a,WF_A_CPU_B9)|2u));
  wf_set_be16(a,WF_A_AE,0u);wf_set_be16(a,WF_A_SELECTOR,0u);
  return WF_OK;
 }
 uint8_t id=wf_u8(a,WF_A_INTERNAL_ID);
 uint16_t tier=rt->g.global_16a0<0x10u?2u:wf_be16(t,WF_A_TIER);
 if(id>=12u||tier>4u||tier&1u)return block(tr,0x0001dea0u);
 wf_r17_m68k_moveq(&rt->m68k_regs,0u,0);
 wf_r17_m68k_moveq(&rt->m68k_regs,1u,0);
 wf_r17_m68k_move_b(&rt->m68k_regs,0u,id);
 wf_r17_m68k_move_w(&rt->m68k_regs,0u,(uint16_t)(id<<2));
 wf_r17_m68k_move_b(&rt->m68k_regs,1u,2u);
 int choice=state1_weighted_24cc(rt,state1_219b4[id][tier>>1],2u);
 if(choice<0)return block(tr,0x000024ccu);
 wf_set_u8(a,WF_A_SELECTOR_LO,choice?0xffu:0u);
 if(choice){
  wf_set_be16(a,WF_A_STATE,1u);
  wf_set_u8(a,WF_A_CPU_B9,(uint8_t)(wf_u8(a,WF_A_CPU_B9)|2u));
  wf_set_be16(a,WF_A_AE,0u);wf_set_be16(a,WF_A_SELECTOR,0u);
  return WF_OK;
 }
 wf_set_u8(a,WF_A_SELECTOR,0u);
 wf_set_be16(a,WF_A_STATE,5u);
 wf_set_u8(a,WF_A_CPU_B5,(uint8_t)(wf_u8(a,WF_A_CPU_B5)&~0x10u));
 wf_set_u8(a,WF_A_CPU_B7,(uint8_t)(wf_u8(a,WF_A_CPU_B7)&~0x40u));
 wf_set_u8(a,WF_A_CPU_B9,(uint8_t)(wf_u8(a,WF_A_CPU_B9)&~2u));
 return wf_state5_execute(rt,slot,tr?&tr->state5:0);
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
/* $1C8F4 dispatch, $1D1F0 substate four. */
static wf_status_t state1(wf_runtime_t*rt,unsigned slot,wf_actor_raw_t*a,wf_cpu_trace_t*tr){
 uint8_t sub=wf_u8(a,WF_A_CPU_SUBSTATE);
 if(sub>=13u)return block(tr,0x0001c8f4u);
 wf_r17_m68k_moveq(&rt->m68k_regs,0u,0);
 wf_r17_m68k_move_b(&rt->m68k_regs,0u,(uint8_t)(sub<<2));
 if(sub==0u)return state1_sub0(rt,slot,a,tr);
 if(sub==1u)return state1_sub1(rt,slot,a,tr);
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
static wf_status_t state12(wf_actor_raw_t*a,wf_cpu_trace_t*tr){
 uint8_t bd=wf_u8(a,WF_A_CPU_BD);
 if(bd<0x30u){wf_set_u8(a,WF_A_CPU_BD,(uint8_t)(bd+1u));return WF_OK;}
 wf_set_be16(a,WF_A_CPU_B6,0u);
 wf_set_u8(a,WF_A_CPU_B9,(uint8_t)(wf_u8(a,WF_A_CPU_B9)&~2u));
 return block(tr,0x0001f7f0u);
}
wf_status_t wf_cpu_actor_step(wf_runtime_t *rt,unsigned slot,wf_cpu_trace_t *tr){
 if(tr) memset(tr,0,sizeof(*tr));
 if(!rt||slot>=WF_ACTOR_SLOTS) return WF_BAD_STATE;
 wf_actor_raw_t *a=&rt->actors[slot];
 uint8_t st=wf_main_state(a);
 uint32_t h=arcade_cpu_main_handler(st);
 if(tr){tr->state=st;tr->handler_rom=h;}
 if(!h) return WF_BAD_STATE;
 switch(st){case 0u:return state0(rt,a,tr);case 1u:return state1(rt,slot,a,tr);
 case 5u:return wf_state5_execute(rt,slot,tr?&tr->state5:0);case 6u:return state6(rt,slot,tr);case 7u:return WF_OK;case 8u:wf_set_u8(a,WF_A_CPU_BD,0u);return WF_OK;case 9u:return state9(rt,slot,tr);case 0x0au:return WF_OK;case 11u:return state11(rt,a,tr);case 12u:return state12(a,tr);default:return block(tr,h);}
}
