#include "wf_runtime.h"
/* Exact central damage phase implementation. Other phase owners bind independently. */
static void damage_one(wf_runtime_t *rt,unsigned slot,wf_actor_raw_t *a,void *u){
 (void)rt;(void)slot;(void)u;
 wf_apply_pending_damage_and_tier(a);
 /* $24E92..$24EB4 runs even with no pending damage or a damage lockout. */
 uint16_t timer=wf_be16(a,0x54u);
 if(timer==0u){wf_set_be16(a,0x54u,0u);wf_set_be16(a,WF_A_TIMER52,0u);}
 else wf_set_be16(a,0x54u,(uint16_t)(timer-1u));
 if((wf_u8(a,0x32u)&8u)!=0u){
  timer=wf_be16(a,WF_A_TIMER_D0);
  if(timer!=0u){
   wf_set_be16(a,WF_A_TIMER_D0,(uint16_t)(timer-1u));
   if(timer==1u)wf_set_u8(a,0x32u,(uint8_t)(wf_u8(a,0x32u)&0xf7u));
  }
 }
}
wf_status_t wf_phase_damage(wf_runtime_t *rt,uint32_t addr,void *user){(void)addr;(void)user;wf_for_each_active(rt,damage_one,0);return WF_OK;}
wf_status_t wf_phase_vblank_stub(wf_runtime_t *rt,uint32_t addr,void *user){(void)addr;(void)user;rt->vblank_ack=1;return WF_OK;}
