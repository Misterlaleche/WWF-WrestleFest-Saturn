#include <string.h>
#include "../../src/wf_cpu_runtime.c"

int cpu_flag2_branch_native(uint8_t *actor,uint8_t *target,uint32_t *dregs){
 wf_runtime_t runtime={0};wf_cpu_trace_t trace={0};
 if(!actor||!target||!dregs)return -1;
 memcpy(&runtime.actors[0],actor,WF_ACTOR_BYTES);
 memcpy(&runtime.actors[1],target,WF_ACTOR_BYTES);
 memcpy(runtime.m68k_regs.d,dregs,8u*sizeof(uint32_t));
 runtime.m68k_regs.valid=1;
 wf_status_t status=state1_target_flag2_1ccf0(&runtime,&runtime.actors[0],&runtime.actors[1],&trace);
 memcpy(actor,&runtime.actors[0],WF_ACTOR_BYTES);
 memcpy(dregs,runtime.m68k_regs.d,8u*sizeof(uint32_t));
 return status==WF_OK?0:-(int)trace.blocked_dependency_rom;
}
