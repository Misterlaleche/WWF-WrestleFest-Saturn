#include <string.h>
#include "wf_r17_dispatch_1c03e.h"
static uint16_t r16(const uint8_t*p){return (uint16_t)(((unsigned)p[0]<<8)|p[1]);}
static uint32_t r32(const uint8_t*p){return ((uint32_t)r16(p)<<16)|r16(p+2);}
static void w16(uint8_t*p,uint16_t v){p[0]=(uint8_t)(v>>8);p[1]=(uint8_t)v;}
static void w32(uint8_t*p,uint32_t v){w16(p,(uint16_t)(v>>16));w16(p+2,(uint16_t)v);}
int action31_oracle(uint8_t *memory,uint32_t *d,unsigned slot,uint32_t *descriptor){
    wf_runtime_t rt={0};wf_r17_actions_r11_bridge_t actions={0};
    wf_r17_dispatch_1c03e_ctx_t dispatch={0};uint8_t *g=memory+0x1c0000;
    if(slot>=WF_ACTOR_SLOTS)return -1;
    memcpy(rt.actors,g+0x5b0,sizeof rt.actors);memcpy(rt.m68k_regs.d,d,32);rt.m68k_regs.valid=1;
    rt.g.mode_0161=g[0x161];rt.g.credited_route=r16(g+0x7c);
    rt.g.rng_state=r32(g+0x5c);rt.g.rng_state_valid=1;rt.g.frame_counter=r32(g+0x80);
    rt.g.global_1800=r16(g+0x1800);dispatch.actions=&actions;
    wf_status_t status=wf_r17_dispatch_1c03e_bounded(&rt,&rt.actors[slot],&dispatch);
    memcpy(g+0x5b0,rt.actors,sizeof rt.actors);memcpy(d,rt.m68k_regs.d,32);
    w32(g+0x5c,rt.g.rng_state);w16(g+0x1800,rt.g.global_1800);
    if(descriptor)*descriptor=actions.trace[slot].tick_descriptor_rom;
    return status?-(int)dispatch.blocked_dependency_rom:0;
}
