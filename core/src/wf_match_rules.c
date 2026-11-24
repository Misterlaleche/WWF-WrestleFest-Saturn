#include "wf_match_rules.h"
int wf_tag_has_terminal_marker(const wf_actor_raw_t *a0,const wf_actor_raw_t *a2){ return ((wf_be16(a0,WF_A_RESULT)|wf_be16(a2,WF_A_RESULT))&0xC000u)!=0; }
void wf_tag_countout_roles(wf_actor_raw_t *a,wf_actor_raw_t *b,int oa,int ob){
 if(oa&&ob){ wf_set_be16(a,WF_A_RESULT_ROLE,0x8005); wf_set_be16(b,WF_A_RESULT_ROLE,0x8005); return; }
 if(oa){ wf_set_be16(a,WF_A_RESULT_ROLE,0x8002); wf_set_be16(b,WF_A_RESULT_ROLE,0x8003); return; }
 if(ob){ wf_set_be16(b,WF_A_RESULT_ROLE,0x8002); wf_set_be16(a,WF_A_RESULT_ROLE,0x8003); }
}
void wf_tag_result_markers_from_roles(wf_actor_raw_t *a,wf_actor_raw_t *b){
 uint16_t ar=wf_be16(a,WF_A_RESULT_ROLE), br=wf_be16(b,WF_A_RESULT_ROLE);
 uint16_t av=(uint16_t)(wf_be16(a,WF_A_RESULT)&0x000Fu), bv=(uint16_t)(wf_be16(b,WF_A_RESULT)&0x000Fu);
 if((ar&0x00FFu)==0x03u) av|=0x4000u; else if((ar&0x00FFu)==0x02u || (ar&0x00FFu)==0x05u) av|=0x8000u;
 if((br&0x00FFu)==0x03u) bv|=0x4000u; else if((br&0x00FFu)==0x02u || (br&0x00FFu)==0x05u) bv|=0x8000u;
 wf_set_be16(a,WF_A_RESULT,av); wf_set_be16(b,WF_A_RESULT,bv);
}
void wf_royal_pin_credit(wf_actor_raw_t *pinned){ wf_set_u8(pinned,WF_A_CREDIT,(uint8_t)(wf_u8(pinned,WF_A_CREDIT)+1u)); }
