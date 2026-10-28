#include <string.h>
#include "wf_r17_dispatch_1c03e.h"
static uint16_t b16(const uint8_t*p){return (uint16_t)(((unsigned)p[0]<<8)|p[1]);}
static uint32_t b32(const uint8_t*p){return ((uint32_t)b16(p)<<16)|b16(p+2);}
static void w(uint8_t*p,uint32_t v,unsigned n){for(unsigned i=0;i<n;i++)p[i]=(uint8_t)(v>>(8u*(n-1u-i)));}
/* Test-only memory adapter; the bounded production dispatcher owns every effect. */
int wf_action2e_oracle_with_descriptor(uint8_t*mem,uint32_t*regs,unsigned slot,uint32_t*descriptor){
    wf_runtime_t rt={0};wf_r17_actions_r11_bridge_t actions={0};
    wf_r17_dispatch_1c03e_ctx_t dispatch={0};dispatch.actions=&actions;uint8_t*g=mem+0x1c0000;
    memcpy(rt.actors,g+0x5b0,sizeof rt.actors);memcpy(rt.m68k_regs.d,regs,32);rt.m68k_regs.valid=1;
    rt.g.global_15d2=g[0x15d2];rt.g.global_15d3=g[0x15d3];rt.g.global_1800=b16(g+0x1800);
    rt.g.credited_route=b16(g+0x7c);rt.g.mode_0161=g[0x161];
    rt.g.rng_state=b32(g+0x5c);rt.g.rng_state_valid=1;rt.g.frame_counter=b32(g+0x80);
    wf_status_t status=wf_r17_dispatch_1c03e_bounded(&rt,&rt.actors[slot],&dispatch);
    memcpy(g+0x5b0,rt.actors,sizeof rt.actors);memcpy(regs,rt.m68k_regs.d,32);
    g[0x15d2]=rt.g.global_15d2;g[0x15d3]=rt.g.global_15d3;w(g+0x1800,rt.g.global_1800,2);w(g+0x5c,rt.g.rng_state,4);
    if(descriptor)*descriptor=actions.trace[slot].tick_descriptor_rom;
    return status?-(int)dispatch.blocked_dependency_rom:0;
}
int wf_action2e_oracle(uint8_t*mem,uint32_t*regs,unsigned slot){
    return wf_action2e_oracle_with_descriptor(mem,regs,slot,0);
}
