#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "wf_ringout.h"
#include "wf_match_rules.h"
#include "wf_doomsday.h"
int main(void){
 wf_actor_raw_t a,b,h; memset(&a,0,sizeof a);memset(&b,0,sizeof b);memset(&h,0,sizeof h);
 wf_ringout_toss_pair(&a,&b); assert(wf_be16(&a,WF_A_STATE)==4); assert(wf_be16(&a,WF_A_STATE4)==0x000E); assert(wf_be16(&a,WF_A_PENDING_DAMAGE)==8); assert(wf_be16(&b,WF_A_SELECTOR)==0xC006);
 wf_set_u8(&a,WF_A_FLAGS33,0xA5);wf_ringexit_clear_inside(&a);assert(wf_u8(&a,WF_A_FLAGS33)==0xA1);wf_ringentry_set_inside(&a);assert(wf_u8(&a,WF_A_FLAGS33)==0xA5);
 wf_outside_count_t c;wf_outside_count_reset(&c);for(int i=0;i<1599;i++) assert(!wf_outside_count_tick(&c));assert(wf_outside_count_tick(&c));assert(c.count==20);
 memset(&a,0,sizeof a);memset(&b,0,sizeof b);wf_tag_countout_roles(&a,&b,1,1);assert(wf_be16(&a,WF_A_RESULT_ROLE)==0x8005&&wf_be16(&b,WF_A_RESULT_ROLE)==0x8005);wf_tag_result_markers_from_roles(&a,&b);assert((wf_be16(&a,WF_A_RESULT)&0xC000)==0x8000);assert(!wf_tag_has_terminal_marker(&h,&h));assert(wf_tag_has_terminal_marker(&a,&b));
 memset(&a,0,sizeof a);memset(&b,0,sizeof b);wf_tag_countout_roles(&a,&b,1,0);wf_tag_result_markers_from_roles(&a,&b);assert((wf_be16(&a,WF_A_RESULT)&0xC000)==0x8000);assert((wf_be16(&b,WF_A_RESULT)&0xC000)==0x4000);
 wf_royal_pin_credit(&a);assert(wf_u8(&a,WF_A_CREDIT)==1);
 memset(&a,0,sizeof a);memset(&b,0,sizeof b);memset(&h,0,sizeof h);wf_set_u8(&a,0x21,5);wf_set_u8(&a,WF_A_SELECTOR_LO,0x89);assert(wf_doomsday_partner_ready(&a));wf_doomsday_launch(&h,0);assert(wf_be16(&h,WF_A_VX)==0x0400&&wf_be16(&h,WF_A_VZ)==0x0200);wf_set_be16(&h,0x0E,0x0161);assert(wf_doomsday_threshold_ready(&h));
 wf_set_u8(&b,WF_A_ACTION_PHASE,1);wf_set_be16(&b,WF_A_ANIM_COUNTER,0);wf_set_u8(&b,WF_A_FLAGS32,0x02);assert(wf_doomsday_commit(&h,&a,&b));assert(wf_be16(&a,WF_A_SELECTOR)==0x8006);assert(wf_be16(&b,WF_A_STATE4)==0x0019);assert(wf_be16(&b,WF_A_PENDING_DAMAGE)==0x0020);
 puts("PASS test_runtime07");return 0;
}
