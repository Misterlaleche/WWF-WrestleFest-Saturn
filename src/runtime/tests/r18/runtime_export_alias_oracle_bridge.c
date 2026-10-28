#include <string.h>
#include "wf_gameplay_bindings.h"
#include "wf_r18_runtime_handoff.h"
static int audio_fail;
static uint16_t rd(const uint8_t*p){return(uint16_t)((p[0]<<8)|p[1]);}
static wf_status_t audio(wf_runtime_t*r,uint16_t command,void*u){(void)r;(void)command;(void)u;return audio_fail?WF_BLOCKED:WF_OK;}
int runtime_export_alias_oracle(const uint8_t*input,uint8_t*out,unsigned slot,unsigned value,unsigned fault,uint32_t*metrics){
 wf_runtime_t r={0};wf_r17_m68k_shadow_t regs={.valid=1};wf_gameplay_binding_context_t c={0};
 if(slot>=4u)return -1;for(unsigned i=0;i<8;i++)regs.d[i]=0x12345670u+i;
 if(wf_r18_runtime_import(&r,input,0x4000,&regs)!=WF_OK)return -2;
 uint16_t*g[4]={&r.g.global_1150,&r.g.global_1180,&r.g.global_11b0,&r.g.global_11e0};
 wf_actor_raw_t*o[4]={&r.fixed_four_1134,&r.referee_aux,&r.fixed_four_1194,&r.fixed_four_11c4};
 *g[slot]=(uint16_t)value;wf_runtime_t before=r;
 c.camera_valid=1;c.camera.scroll0_x=0x140;c.camera.scroll0_y=0x200;c.audio_command=fault==1u?NULL:audio;
 audio_fail=fault==2u;if(fault==3u)c.render_queue.count[1]=WF_R17_RENDER_LIST_CAP;
 wf_status_t st=wf_phase_fixed_four_10120(&r,0x10120,&c);
 memcpy(out,input,0x4000);if(wf_r18_runtime_export(&r,out,0x4000)!=WF_OK)return -3;
 metrics[0]=st;metrics[1]=*g[slot];metrics[2]=wf_be16(o[slot],WF_A_LATCHED);
 metrics[3]=rd(out+0x1150+slot*0x30);metrics[4]=wf_be16(o[slot],WF_A_MIRROR);
 metrics[5]=wf_be16(o[slot],WF_A_ANIM_COUNTER);metrics[6]=wf_be16(o[slot],WF_A_ANIM_INDEX);
 metrics[7]=c.last_blocked_dependency;metrics[8]=memcmp(&r,&before,sizeof r)==0;
 return 0;
}
