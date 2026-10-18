#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "wf_ground.h"
#include "wf_pin.h"
#include "wf_collision_runtime.h"
#include "wf_runtime.h"
int main(void){
 wf_actor_raw_t a,b;memset(&a,0,sizeof a);memset(&b,0,sizeof b);
 wf_set_be16(&b,WF_A_STATE,4);wf_state4_write_word(&b,0x8008);assert(wf_ground_linked_retype(0x0A,&b)==WF_GROUND_ADVANCE);assert(wf_be16(&b,WF_A_STATE4)==0x8006);
 wf_pin_assert_trigger(&a);assert(wf_pin_trigger(&a));wf_ref_count_t r;wf_ref_count_enter(&r,0);assert(r.phase==0&&r.timer==20&&r.audio_cmd==0x3162);assert(!wf_ref_prelock_cancel(&r,&a));wf_set_u8(&a,WF_A_PINFLAGS,0);assert(wf_ref_prelock_cancel(&r,&a));r.phase=6;assert(!wf_ref_prelock_cancel(&r,&a));
 memset(&a,0,sizeof a);memset(&b,0,sizeof b);wf_set_be16(&a,WF_A_X,100);wf_set_be16(&b,WF_A_X,100);wf_set_be16(&a,WF_A_Z,100);wf_set_be16(&b,WF_A_Z,100);assert(wf_collision_axis06_overlap(&a,&wf_source_boxes[0],&b,&wf_target_boxes[0]));assert(wf_collision_axis0e_overlap(&a,&wf_source_boxes[0],&b,&wf_target_boxes[0]));
 wf_collision_write_profile_damage(&b,14);assert(wf_be16(&b,WF_A_PENDING_DAMAGE)==14);wf_set_be16(&b,WF_A_TIER,2);wf_collision_success_tail(&a,0,&b,1);assert((wf_be16(&a,WF_A_COLL_CACHE)&0x4000u)!=0);assert(wf_be32(&a,WF_A_RECIP_LINK)==0x001C06BCu);assert(wf_be32(&b,WF_A_RECIP_LINK)==0x001C05B0u);assert((wf_state4_flags(&b)&0x80u)!=0);
 memset(&b,0,sizeof b);wf_set_be16(&b,WF_A_STAMINA,30);wf_set_be16(&b,WF_A_STAMINA_MAX,100);wf_set_be16(&b,WF_A_PENDING_DAMAGE,7);wf_apply_pending_damage_and_tier(&b);assert(wf_be16(&b,WF_A_STAMINA)==23);assert(wf_be16(&b,WF_A_TIER)==2);assert(wf_be16(&b,WF_A_VISIBLE_DELTA)==(uint16_t)-7);
 wf_set_be16(&b,WF_A_PENDING_DAMAGE,5);wf_set_be16(&b,WF_A_LOCKOUT,1);wf_apply_pending_damage_and_tier(&b);assert(wf_be16(&b,WF_A_PENDING_DAMAGE)==5);
 puts("PASS test_runtime06");return 0;
}
