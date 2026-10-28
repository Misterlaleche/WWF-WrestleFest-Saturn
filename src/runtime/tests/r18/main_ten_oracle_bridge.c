#include <string.h>
#include "../../src/wf_r17_dispatch_1c03e.c"

int main_ten_native(uint8_t *actor,uint8_t mode,uint32_t *global,
                    uint32_t *dregs,uint32_t *descriptor){
    wf_runtime_t runtime={0};
    wf_r17_dispatch_1c03e_ctx_t ctx={0};
    unsigned variant=0u;
    if(!actor||!global||!dregs||!descriptor)return -1;
    memcpy(&runtime.actors[0],actor,WF_ACTOR_BYTES);
    runtime.g.mode_0161=mode;
    runtime.g.global_1670=*global;
    memcpy(runtime.m68k_regs.d,dregs,8u*sizeof(uint32_t));
    runtime.m68k_regs.valid=1;
    wf_status_t status=main_ten_122e6(&runtime,&runtime.actors[0],&ctx,&variant);
    memcpy(actor,&runtime.actors[0],WF_ACTOR_BYTES);
    *global=runtime.g.global_1670;
    memcpy(dregs,runtime.m68k_regs.d,8u*sizeof(uint32_t));
    *descriptor=variant==1u?0x122a6u:variant==2u?0x122c2u:0x12282u;
    return status==WF_OK?0:-(int)ctx.blocked_dependency_rom;
}
