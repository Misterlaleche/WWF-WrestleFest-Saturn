#include <string.h>
#include "wf_r18_match_result.h"
void wf_match_result_oracle(uint8_t *actors,uint32_t *regs,unsigned config,unsigned stage,unsigned result){
 wf_runtime_t rt;memset(&rt,0,sizeof rt);memcpy(rt.actors,actors,sizeof rt.actors);memcpy(rt.m68k_regs.d,regs,32);
 rt.m68k_regs.valid=1;rt.g.config_0066=(uint16_t)config;rt.g.progression_0162=(uint16_t)stage;
 if(result)wf_r18_result_sound_20156(&rt,NULL);else wf_r18_tag_bonus_90d6(&rt,NULL);
 memcpy(actors,rt.actors,sizeof rt.actors);memcpy(regs,rt.m68k_regs.d,32);
}
int wf_match_endpoint_oracle(unsigned result0,unsigned result2,unsigned stage,unsigned credited,unsigned mode,unsigned ready,unsigned ack){
 wf_runtime_t rt;memset(&rt,0,sizeof rt);rt.frame_stage=(uint8_t)stage;rt.frame_ready=(uint8_t)ready;rt.vblank_ack=(uint8_t)ack;
 rt.g.credited_route=(uint16_t)credited;rt.g.mode_0161=(uint8_t)mode;
 wf_set_be16(&rt.actors[0],WF_A_RESULT,(uint16_t)result0);wf_set_be16(&rt.actors[2],WF_A_RESULT,(uint16_t)result2);
 return wf_r18_tag_match_complete(&rt);
}
