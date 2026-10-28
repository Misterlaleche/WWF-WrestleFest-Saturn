#include <string.h>
#include "wf_r17_dispatch_1c03e.h"
int state4_1bb08_oracle(uint8_t*memory,uint32_t*d,unsigned slot){
 wf_runtime_t r={0};wf_r17_dispatch_1c03e_ctx_t dispatch={0};
 if(slot>=WF_ACTOR_SLOTS)return -1;
 memcpy(r.actors,memory+0x1c05b0,sizeof r.actors);memcpy(r.m68k_regs.d,d,32);r.m68k_regs.valid=1;
 wf_status_t st=wf_r17_dispatch_1c03e_bounded(&r,&r.actors[slot],&dispatch);
 memcpy(memory+0x1c05b0,r.actors,sizeof r.actors);memcpy(d,r.m68k_regs.d,32);
 return st?-(int)dispatch.blocked_dependency_rom:0;
}
