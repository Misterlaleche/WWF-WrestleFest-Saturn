#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "wf_collision.h"
#include "wf_victim65.h"
int main(void){
 assert(wf_collision_profiles[0x17].first_selector==0x0B);
 assert(wf_first_callbacks[0x0B]==0x0247ECu);
 assert(wf_collision_normalize_low(0x02)==0x0B);
 assert(wf_collision_normalize_low(0x22)==0x0C);
 assert(wf_collision_normalize_low(0x24)==0x24);
 wf_actor_raw_t s,d; memset(&s,0,sizeof(s)); memset(&d,0,sizeof(d));
 wf_set_u8(&s,WF_A_FACING,0x80); wf_set_u8(&d,WF_A_FACING,0x80); wf_state4_write_word(&d,0x8002); wf_collision_normalize_receiver(&s,&d); assert(wf_be16(&d,WF_A_STATE4)==0x800B);
 memset(&d,0,sizeof(d)); wf_set_be16(&d,WF_A_STATE,4); wf_state4_write_word(&d,0x0005); wf_set_u8(&d,WF_A_TIER_LO,2); wf_victim65_first_entry(&d); wf_set_u8(&d,WF_A_CONTACT+1,0x10); assert(wf_victim65_tick_transition(&d,0)==WF_V65_RETYPE); assert(wf_be16(&d,WF_A_STATE4)==0x8009);
 memset(&d,0,sizeof(d)); wf_set_be16(&d,WF_A_STATE,4); wf_state4_write_word(&d,0x000B); assert(wf_victim65_tick_transition(&d,0)==WF_V65_RETYPE); assert(wf_state4_subtype(&d)==2);
 puts("PASS test_collision_victim"); return 0;
}
