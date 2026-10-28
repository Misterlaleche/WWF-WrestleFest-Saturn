#include <string.h>
#include "wf_r17_dispatch_1c03e.h"

static uint32_t read32(const uint8_t *p){
    return ((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|((uint32_t)p[2]<<8)|p[3];
}

int action6b_oracle(uint8_t *memory,uint32_t *d,unsigned slot,uint32_t *descriptor){
    wf_runtime_t rt={0};
    wf_r17_actions_r11_bridge_t actions={0};
    wf_r17_dispatch_1c03e_ctx_t dispatch={0};
    uint8_t *ram=memory+0x1c0000;
    if(slot>=WF_ACTOR_SLOTS)return -1;
    memcpy(rt.actors,ram+0x5b0,sizeof rt.actors);
    memcpy(rt.m68k_regs.d,d,sizeof rt.m68k_regs.d);
    rt.m68k_regs.valid=1;
    rt.g.rng_state=read32(ram+0x5c);
    rt.g.rng_state_valid=1;
    rt.g.frame_counter=read32(ram+0x80);
    memcpy(rt.g.direction_scratch_15e4,ram+0x15e4,8);
    dispatch.actions=&actions;
    wf_status_t status=wf_r17_dispatch_1c03e_bounded(&rt,&rt.actors[slot],&dispatch);
    memcpy(ram+0x5b0,rt.actors,sizeof rt.actors);
    memcpy(ram+0x15e4,rt.g.direction_scratch_15e4,8);
    memcpy(d,rt.m68k_regs.d,sizeof rt.m68k_regs.d);
    if(descriptor)*descriptor=actions.trace[slot].tick_descriptor_rom;
    return status?-(int)dispatch.blocked_dependency_rom:0;
}
