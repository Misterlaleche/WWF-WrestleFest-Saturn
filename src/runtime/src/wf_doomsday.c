#include "wf_doomsday.h"
int wf_doomsday_partner_ready(const wf_actor_raw_t *a){ return wf_u8(a,0x21)==5u && wf_u8(a,WF_A_SELECTOR_LO)==0x89u; }
void wf_doomsday_launch(wf_actor_raw_t *h,int neg){
 int16_t vx=0x0400,vy=0x0100; if(neg){vx=(int16_t)-vx;vy=(int16_t)-vy;}
 wf_set_be16(h,WF_A_VX,(uint16_t)vx);wf_set_be16(h,WF_A_VY,(uint16_t)vy);wf_set_be16(h,WF_A_VZ,0x0200);wf_set_be16(h,WF_A_VZ_DEC,0x0038);
}
int wf_doomsday_threshold_ready(const wf_actor_raw_t *h){ uint16_t q=wf_be16(h,0x0E); return q>0x0160u || ((wf_u8(h,WF_A_FLAGS33)&0x04u)!=0 && q>0x0120u); }
int wf_doomsday_commit(wf_actor_raw_t *h,wf_actor_raw_t *a,wf_actor_raw_t *v){
 (void)h;
 if(wf_u8(v,WF_A_ACTION_PHASE)!=1u || wf_be16(v,WF_A_ANIM_COUNTER)!=0u || (wf_u8(v,WF_A_FLAGS32)&0x02u)==0u) return 0;
 wf_set_be16(a,WF_A_STATE,5);wf_set_be16(a,WF_A_SELECTOR,0x8006);
 wf_set_be16(v,WF_A_STATE,4);wf_state4_write_word(v,0x0019);wf_set_be16(v,WF_A_PENDING_DAMAGE,0x0020);
 return 1;
}
