#include "wf_ringout.h"
void wf_ringout_toss_pair(wf_actor_raw_t *v,wf_actor_raw_t *a){
 wf_set_be16(v,WF_A_STATE,4); wf_state4_write_word(v,0x000E); wf_set_be16(v,WF_A_PENDING_DAMAGE,0x0008);
 wf_set_be16(a,WF_A_STATE,5); wf_set_be16(a,WF_A_SELECTOR,0xC006);
}
void wf_ringexit_clear_inside(wf_actor_raw_t *a){ wf_set_u8(a,WF_A_FLAGS33,(uint8_t)(wf_u8(a,WF_A_FLAGS33)&~0x04u)); }
void wf_ringentry_set_inside(wf_actor_raw_t *a){ wf_set_u8(a,WF_A_FLAGS33,(uint8_t)(wf_u8(a,WF_A_FLAGS33)|0x04u)); }
void wf_outside_count_reset(wf_outside_count_t *c){ c->subcount=0;c->count=0; }
int wf_outside_count_tick(wf_outside_count_t *c){
 c->subcount=(uint8_t)(c->subcount+1u);
 if(c->subcount!=0x50u) return 0;
 c->subcount=0; c->count=(uint8_t)(c->count+1u);
 return c->count==0x14u;
}
