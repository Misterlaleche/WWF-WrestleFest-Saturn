#include "wf_r18_match_result.h"
int wf_r18_tag_match_complete(const wf_runtime_t *rt){
 if(!rt||rt->frame_stage!=2u||!rt->frame_ready||!rt->vblank_ack||
    !rt->g.credited_route||(rt->g.mode_0161&1u))return 0;
 return ((wf_be16(&rt->actors[0],WF_A_RESULT)|
          wf_be16(&rt->actors[2],WF_A_RESULT))&0xc000u)!=0u;
}
/* $90D6: post-pin bonus only for the four credited player records whose
 * +$FF.bit0 is clear. ADD.W wraps before the unsigned maximum comparison. */
wf_status_t wf_r18_tag_bonus_90d6(wf_runtime_t *rt,void *user){
 (void)user;if(!rt)return WF_BAD_STATE;
 static const uint16_t bonus[4]={24u,32u,12u,0u};
 uint16_t amount=bonus[(rt->g.config_0066>>13)&3u];
 for(unsigned i=0;i<4u;i++){
  wf_actor_raw_t *a=&rt->actors[i];if(wf_u8(a,WF_A_RESULT_ROLE_LO)&1u)continue;
  uint16_t health=(uint16_t)(wf_be16(a,WF_A_STAMINA)+amount);
  wf_set_be16(a,WF_A_VISIBLE_DELTA,(uint16_t)(wf_be16(a,WF_A_VISIBLE_DELTA)+amount));
  wf_set_be16(a,WF_A_STAMINA,health<wf_be16(a,WF_A_STAMINA_MAX)?health:wf_be16(a,WF_A_STAMINA_MAX));
 }
 return WF_OK;
}
/* $20156 chooses a sound command; $2052 preserves D0 and only writes the
 * sound device. The requested silent build keeps the selector in D0. */
wf_status_t wf_r18_result_sound_20156(wf_runtime_t *rt,void *user){
 (void)user;if(!rt)return WF_BAD_STATE;
 wf_r17_m68k_move_w(&rt->m68k_regs,0u,(rt->g.progression_0162==4u||rt->g.progression_0162==9u)?0x3109u:0x3105u);
 return WF_OK;
}
