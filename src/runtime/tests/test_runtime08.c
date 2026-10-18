#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "wf_collision_dispatch.h"
#include "wf_royal.h"
#include "wf_pin_terminal.h"
#include "wf_victim65.h"
int main(void){
 wf_actor_raw_t a[WF_ACTOR_SLOTS];memset(a,0,sizeof a);wf_royal_scan_t rs;uint16_t marks[WF_ACTOR_SLOTS]={0};
 wf_set_u8(&a[0],WF_A_ACTIVE,0x80);assert(wf_royal_survivor_scan(a,&rs,marks)==WF_ROYAL_SCAN_WINNER);assert(rs.survivor_slot==0);
 wf_set_u8(&a[1],WF_A_ACTIVE,0x80);assert(wf_royal_survivor_scan(a,&rs,marks)==WF_ROYAL_SCAN_CONTINUE);
 wf_set_u8(&a[0],WF_A_FLAGS32,0x10);wf_set_u8(&a[1],WF_A_FLAGS32,0x10);assert(wf_royal_survivor_scan(a,&rs,marks)==WF_ROYAL_SCAN_ZERO);assert(marks[0]==0x3001&&marks[1]==0x3001);
 assert(wf_pin_phase7_route(WF_PIN_MODE_TAG)==WF_PIN_TERM_TAG_RESULT);assert(wf_pin_phase7_route(WF_PIN_MODE_ROYAL)==WF_PIN_TERM_ROYAL_ELIMINATION);
 wf_actor_raw_t s,d,l;memset(&s,0,sizeof s);memset(&d,0,sizeof d);memset(&l,0,sizeof l);wf_collision_pair_ctx_t c={&s,&d,&l,3,0,0};
 wf_set_be16(&s,WF_A_PROFILE,0x000Fu); /* profile 0F => FIRST6 */
 wf_set_be16(&d,WF_A_PROFILE,0x0000u);wf_set_u8(&d,WF_A_FLAGS33,0);assert(wf_collision_profile_pair_apply(&c)==WF_COLL_ACCEPT);assert(wf_main_state(&d)==4);assert(wf_state4_subtype(&d)==0x0C); /* F6 03 then S0 normalize 03->0C */
 memset(&d,0,sizeof d);wf_set_be16(&d,WF_A_STATE,4);wf_state4_write_word(&d,0x8009);wf_set_u8(&d,WF_A_ACTION_PHASE,0xFE);assert(wf_victim65_tick_transition(&d,0)==WF_V65_RETYPE);assert(wf_main_state(&d)==7);assert(wf_state4_subtype(&d)==9);assert((wf_state4_flags(&d)&0x80)==0);
 memset(&d,0,sizeof d);wf_set_be16(&d,WF_A_STATE,4);wf_state4_write_word(&d,0x001E);wf_set_u8(&d,WF_A_ACTION_PHASE,0xFE);assert(wf_victim65_tick_transition(&d,0)==WF_V65_RETYPE);assert(wf_be16(&d,WF_A_STATE4)==0x100F);
 puts("PASS test_runtime08");return 0;
}
