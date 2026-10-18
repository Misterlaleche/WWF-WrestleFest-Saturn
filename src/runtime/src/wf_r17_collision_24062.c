#include <string.h>
#include "wf_r17_collision_24062.h"
#include "wf_collision.h"
#include "wf_collision_runtime.h"
#include "wf_collision_dispatch.h"

static uint16_t abs16_delta(uint16_t a,uint16_t b){
 uint16_t d=(uint16_t)(a-b); if((d&0x8000u)!=0u)d=(uint16_t)(0u-d); return d;
}
static int coarse_pass(const wf_actor_raw_t*s,const wf_actor_raw_t*t,uint8_t target_profile){
 uint16_t sy=wf_be16(s,WF_A_Y),ty=wf_be16(t,WF_A_Y);
 if(target_profile==0x14u)ty=(uint16_t)(ty-8u);
 if(abs16_delta(ty,sy)>=0x000cu)return 0;
 if(abs16_delta(wf_be16(t,WF_A_X),wf_be16(s,WF_A_X))>=0x0050u)return 0;
 return 1;
}
static int fine_pass(const wf_actor_raw_t*s,const wf_collision_profile_t*sp,const wf_actor_raw_t*t,const wf_collision_profile_t*tp){
 if(sp->attack_box>=11u)return -1;
 const wf_box4_t*sb=&wf_source_boxes[sp->attack_box];
 if((tp->flags&0x40u)!=0u){
  if(tp->primary_box>=11u)return -1;
  const wf_box4_t*tb=&wf_target_boxes[tp->primary_box];
  if(wf_collision_axis06_overlap(s,sb,t,tb)&&wf_collision_axis0e_overlap(s,sb,t,tb))return 1;
 }
 if((tp->flags&0x20u)!=0u){
  if(tp->secondary_box>=11u)return -1;
  const wf_box4_t*tb=&wf_target_boxes[tp->secondary_box];
  if(wf_collision_axis06_overlap(s,sb,t,tb)&&wf_collision_axis0e_overlap(s,sb,t,tb))return 1;
 }
 return 0;
}
static int target_gate(const wf_runtime_t*rt,const wf_actor_raw_t*s,unsigned target_slot,const wf_actor_raw_t*t){
 if(wf_be16(t,WF_A_PROFILE)==0u)return 0;
 if((wf_u8(t,WF_A_STATE)&0x80u)==0u)return 0;
 if((wf_u8(t,WF_A_FLAGS32)&0x40u)!=0u)return 0;
 if(wf_be16(t,WF_A_LOCKOUT)!=0u)return 0;
 if((wf_u8(t,WF_A_FLAGS32)&0x04u)!=0u && (rt->g.mode_0161&0x02u)==0u)return 0;
 if(wf_be16(t,WF_A_MIRROR)==0xffffu)return 0;
 if((wf_be16(s,WF_A_COLL_CACHE)&(uint16_t)(0x8000u>>target_slot))!=0u)return 0;
 return 1;
}
static uint32_t blocked_callback(uint8_t sid,uint8_t tid){
 if(sid<34u){uint8_t f=wf_collision_profiles[sid].first_selector;if(f<15u)return wf_first_callbacks[f];}
 if(tid<34u){uint8_t s=wf_collision_profiles[tid].second_selector;if(s<13u)return wf_second_callbacks[s];}
 return 0x024062u;
}
wf_status_t wf_r17_collision_24062(wf_runtime_t *rt,wf_r17_collision_24062_trace_t *trace){
 if(!rt||!trace)return WF_BAD_STATE;
 memset(trace,0,sizeof(*trace));
 for(unsigned ss=0;ss<WF_ACTOR_SLOTS;ss++){
  wf_actor_raw_t*src=&rt->actors[ss];uint16_t profile_word=wf_be16(src,WF_A_PROFILE);
  if(profile_word==0u)continue;
  if((wf_u8(src,WF_A_STATE)&0x80u)==0u)continue;
  uint8_t sid=(uint8_t)profile_word;if(sid>=34u){trace->blocked_dependency_rom=0x0240bcu;return WF_BLOCKED;}
  const wf_collision_profile_t*sp=&wf_collision_profiles[sid];if((sp->flags&0x80u)==0u)continue;
  /* A2/source row is retained for the complete inner target scan, even after generic tail clears +$4C. */
  for(unsigned ts=0;ts<WF_ACTOR_SLOTS;ts++){
   if(ts==ss)continue;
   wf_actor_raw_t*dst=&rt->actors[ts];
   if(!target_gate(rt,src,ts,dst))continue;
   uint8_t tid=wf_collision_profile_id(dst);if(tid>=34u){trace->blocked_dependency_rom=0x0240bcu;return WF_BLOCKED;}
   const wf_collision_profile_t*tp=&wf_collision_profiles[tid];
   if(!coarse_pass(src,dst,tid))continue;
   int fp=fine_pass(src,sp,dst,tp);if(fp<0){trace->blocked_dependency_rom=0x0241e8u;return WF_BLOCKED;}if(!fp)continue;
   trace->source_pairs_tested++;
   wf_actor_raw_t*linked=wf_actor_from_rom_ptr(rt,wf_be32(dst,WF_A_LINK),0);
   wf_collision_pair_ctx_t pc={src,dst,linked,(uint16_t)(ts+1u),sid,tid};
   wf_actor_raw_t src_before=*src,dst_before=*dst,linked_before;int have_linked=linked!=0;if(have_linked)linked_before=*linked;
   wf_coll_result_t cr=wf_collision_profile_pair_apply_rows(sid,tid,&pc);
   if(cr==WF_COLL_REJECT)continue;
   if(cr==WF_COLL_BLOCKED){*src=src_before;*dst=dst_before;if(have_linked)*linked=linked_before;trace->blocked_dependency_rom=blocked_callback(sid,tid);return WF_BLOCKED;}
   wf_collision_success_tail(src,ss,dst,ts);trace->accepted_pairs++;
  }
 }
 return WF_OK;
}
