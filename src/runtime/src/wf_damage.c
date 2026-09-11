#include "wf_runtime.h"
/* $24E58 -> $24EC2. +$FE nonzero defers pending unchanged. */
void wf_apply_pending_damage_and_tier(wf_actor_raw_t *a){
 if(wf_be16(a,WF_A_LOCKOUT)!=0u) return;
 uint16_t live=wf_be16(a,WF_A_STAMINA), pending=wf_be16(a,WF_A_PENDING_DAMAGE);
 uint16_t next=pending>=live?0u:(uint16_t)(live-pending);
 wf_set_be16(a,WF_A_STAMINA,next);
 if(pending){wf_set_be16(a,WF_A_VISIBLE_DELTA,(uint16_t)(0u-pending));}
 wf_set_be16(a,WF_A_PENDING_DAMAGE,0);
 uint16_t max=wf_be16(a,WF_A_STAMINA_MAX); if(max==0u)return;
 uint16_t tier=next<=24u?2u:(next<=(uint16_t)(2u*(max/3u))?1u:0u); wf_set_be16(a,WF_A_TIER,tier);
}
