#include <string.h>
#include "wf_cpu_runtime.h"
#include "arcade_cpu_final_bridge.h"
static wf_status_t block(wf_cpu_trace_t*t,uint32_t pc){if(t)t->blocked_dependency_rom=pc;return WF_BLOCKED;}
static wf_actor_raw_t *ptr(wf_runtime_t*rt,const wf_actor_raw_t*a,size_t off){return wf_actor_from_rom_ptr(rt,wf_be32(a,off),0);}

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
wf_status_t wf_cpu_actor_step(wf_runtime_t *rt,unsigned slot,wf_cpu_trace_t *tr){
 if(tr) memset(tr,0,sizeof(*tr));
 if(!rt||slot>=WF_ACTOR_SLOTS) return WF_BAD_STATE;
 wf_actor_raw_t *a=&rt->actors[slot];
 uint8_t st=wf_main_state(a);
 uint32_t h=arcade_cpu_main_handler(st);
 if(tr){tr->state=st;tr->handler_rom=h;}
 if(!h) return WF_BAD_STATE;
 switch(st){case 5u:return wf_state5_execute(rt,slot,tr?&tr->state5:0);case 6u:return state6(rt,slot,tr);case 7u:return WF_OK;case 8u:wf_set_u8(a,WF_A_CPU_BD,0u);return WF_OK;case 9u:return state9(rt,slot,tr);case 0x0au:return WF_OK;default:return block(tr,h);}
}
