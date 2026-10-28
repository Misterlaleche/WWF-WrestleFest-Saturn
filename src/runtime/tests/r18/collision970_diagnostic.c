/* Read-only diagnostic composition; no production source change. */
#include <stdio.h>
#include "wf_collision_dispatch.h"
#include "wf_collision.h"
static wf_coll_result_t observe_pair(uint8_t,uint8_t,wf_collision_pair_ctx_t*);
#define wf_collision_profile_pair_apply_rows observe_pair
#include "../../src/wf_r17_collision_24062.c"
#undef wf_collision_profile_pair_apply_rows
static wf_coll_result_t observe_pair(uint8_t ps,uint8_t pt,wf_collision_pair_ctx_t*c){
 wf_coll_result_t result=wf_collision_profile_pair_apply_rows(ps,pt,c);
 if(result==WF_COLL_BLOCKED){
  wf_actor_raw_t src=*c->src,dst=*c->dst,linked;int have=c->linked!=NULL;if(have)linked=*c->linked;
  uint8_t first=wf_collision_profiles[ps].first_selector,second=wf_collision_profiles[pt].second_selector;
  wf_coll_result_t a=wf_collision_first_apply(first,c),b=WF_COLL_BLOCKED;if(a==WF_COLL_ACCEPT)b=wf_collision_second_apply(second,c);
  printf("EXACT COLLISION srcID=%u dstID=%u profiles=%u/%u first=%u/%06x result=%u second=%u/%06x result=%u facing=%02x/%02x incoming=%04x timer52=%04x link=%08x\n",wf_be16(&src,2),wf_be16(&dst,2),ps,pt,first,wf_first_callbacks[first],a,second,wf_second_callbacks[second],b,wf_u8(&src,WF_A_FACING),wf_u8(&dst,WF_A_FACING),wf_be16(&dst,WF_A_STATE4),wf_be16(&dst,WF_A_TIMER52),wf_be32(&dst,WF_A_LINK));
  *c->src=src;*c->dst=dst;if(have)*c->linked=linked;
 }
 return result;
}
