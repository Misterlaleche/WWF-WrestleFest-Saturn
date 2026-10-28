#include "wf_r18_runtime_handoff.h"
#include <string.h>
int runtime_handoff_oracle(uint8_t*ram,uint32_t d[8],uint32_t*out){
 static wf_runtime_t r;static wf_phase_binding_t binding;
 memset(&r,0xa5,sizeof r);r.bindings=&binding;r.binding_count=1;r.trace_count=3;
 wf_r17_m68k_shadow_t regs={.valid=1};memcpy(regs.d,d,32);
 wf_status_t st=wf_r18_runtime_handoff_f8c(&r,ram,0x4000,&regs);if(st!=WF_OK)return st;
 memcpy(d,regs.d,32);unsigned n=0;
 out[n++]=r.bindings==&binding&&r.binding_count==1&&r.trace_count==3;
 out[n++]=r.g.mode_0160;out[n++]=r.g.mode_0161;out[n++]=r.g.progression_0162;out[n++]=r.g.config_0066;out[n++]=r.g.credited_route;
 out[n++]=r.g.rng_state;out[n++]=r.g.frame_counter;out[n++]=((uint16_t)r.g.global_0082<<8)|r.g.pipeline_mode;out[n++]=r.frame_stage;
 out[n++]=r.g.global_1804;out[n++]=r.g.global_1806;out[n++]=r.g.global_16a0;out[n++]=r.g.global_16a2;out[n++]=r.g.global_169e;
 out[n++]=r.g.credit_count_004f;out[n++]=r.g.credit_remainder_004e;
 for(unsigned i=0;i<9;i++){out[n++]=wf_be16(&r.actors[i],0x56);out[n++]=wf_be16(&r.actors[i],0x66);out[n++]=wf_be32(&r.actors[i],0x8a);}
 return 0;
}
