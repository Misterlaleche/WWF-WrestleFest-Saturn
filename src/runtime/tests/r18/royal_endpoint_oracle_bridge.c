#include <string.h>
#include "wf_r18_match_result.h"
#include "wf_r17_postack_2017a.h"
int royal_endpoint_observe(unsigned flag,unsigned stage,unsigned ready,unsigned ack,unsigned credit,unsigned mode){
 wf_runtime_t r,before;memset(&r,0xa5,sizeof r);r.g.global_16c4=(uint16_t)flag;
 r.frame_stage=(uint8_t)stage;r.frame_ready=(uint8_t)ready;r.vblank_ack=(uint8_t)ack;
 r.g.credited_route=(uint16_t)credit;r.g.mode_0161=(uint8_t)mode;before=r;
 int result=wf_r18_royal_game_over(&r);return memcmp(&r,&before,sizeof r)?-1:result;
}
int royal_endpoint_null(void){return wf_r18_royal_game_over(NULL);}
static wf_status_t postack(wf_runtime_t*r,uint32_t pc,void*user){(void)pc;(void)user;return wf_r17_postack_2017a(r,NULL,NULL);}
int royal_endpoint_ack_sequence(unsigned flag,unsigned expected_after,unsigned expected_endpoint){
 wf_runtime_t r;wf_phase_binding_t b={0x2017au,postack,NULL};wf_runtime_init(&r,&b,1);
 r.frame_stage=1;r.frame_ready=1;r.g.credited_route=0x8000;r.g.mode_0161=1;
 r.g.global_16c4=(uint16_t)flag;r.g.frame_counter=23;
 if(wf_r18_royal_game_over(&r)||wf_runtime_frame(&r)!=WF_BLOCKED||r.g.frame_counter!=23)return 1;
 wf_runtime_vblank_ack(&r);wf_status_t status=wf_runtime_frame(&r);
 if(r.g.global_16c4!=(uint16_t)expected_after||r.g.frame_counter!=24)return 2;
 if(wf_r18_royal_game_over(&r)!=(int)expected_endpoint)return 3;
 if(expected_endpoint&&(status!=WF_BLOCKED||r.frame_stage!=2||!r.frame_ready||!r.vblank_ack))return 4;
 if((expected_after>>8)&&status!=WF_BLOCKED)return 5;
 return 0;
}
