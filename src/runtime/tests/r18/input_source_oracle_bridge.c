#include "wf_gameplay_bindings.h"
#include <string.h>
static wf_runtime_t machine;
static wf_gameplay_binding_context_t gameplay;
static uint16_t physical[4];
static uint32_t addresses[16],denied;
static unsigned reads,transport_flags;
static wf_status_t physical_word(wf_runtime_t*r,uint32_t at,uint16_t*out,void*u){
 (void)u;addresses[reads++]=at;
 if(transport_flags&4u)for(unsigned i=0;i<8;i++)r->m68k_regs.d[i]=0xfaced000u+i;
 if((transport_flags&2u)&&at==denied)return WF_BLOCKED;
 if(at<0x140020u||at>0x140026u||(at&1u))return WF_BLOCKED;
 *out=physical[(at-0x140020u)/2u];return WF_OK;
}
int input_source_oracle(uint8_t*raw,uint32_t d[8],const uint16_t words[4],
 unsigned flags,uint32_t fail_at,uint32_t output[3],uint32_t requested[16]){
 memset(&machine,0,sizeof machine);memset(&gameplay,0,sizeof gameplay);
 memcpy(machine.actors,raw+0x5b0,sizeof machine.actors);
 memcpy(machine.m68k_regs.d,d,32);machine.m68k_regs.valid=1;
 machine.g.pipeline_mode=raw[0x83];reads=0;denied=fail_at;transport_flags=flags;
 memcpy(physical,words,sizeof physical);memset(addresses,0,sizeof addresses);
 gameplay.participant_input_word=(flags&1u)?NULL:physical_word;
 gameplay.input_valid_mask=0x1ffu;memset(gameplay.input_hw_low,0xff,sizeof gameplay.input_hw_low);
 for(unsigned i=0;i<4;i++)gameplay.input_hw_low[i]=(uint8_t)words[i];
 int status=wf_phase_input_real(&machine,0x514eu,&gameplay);
 memcpy(raw+0x5b0,machine.actors,sizeof machine.actors);memcpy(d,machine.m68k_regs.d,32);
 output[0]=gameplay.last_blocked_phase;output[1]=gameplay.last_blocked_dependency;output[2]=reads;
 memcpy(requested,addresses,sizeof addresses);return status;
}
/* The runner appends the actual native input_word() body verbatim here. */
