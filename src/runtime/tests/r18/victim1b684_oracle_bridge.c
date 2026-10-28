#include <string.h>
#include "wf_r17_dispatch_1c03e.h"
static uint16_t r16(const uint8_t*p){return (uint16_t)(((unsigned)p[0]<<8)|p[1]);}
static void w16(uint8_t*p,uint16_t v){p[0]=(uint8_t)(v>>8);p[1]=(uint8_t)v;}
int victim1b684_oracle(uint8_t *memory,uint32_t *d,unsigned slot){
    wf_runtime_t rt={0};wf_r17_actions_r11_bridge_t actions={0};
    wf_r17_dispatch_1c03e_ctx_t dispatch={0};uint8_t *g=memory+0x1c0000;
    if(slot>=WF_ACTOR_SLOTS)return -1;
    memcpy(rt.actors,g+0x5b0,sizeof rt.actors);memcpy(rt.m68k_regs.d,d,32);rt.m68k_regs.valid=1;
    rt.g.mode_0161=g[0x161];rt.g.credited_route=r16(g+0x7c);
    rt.g.global_1150=r16(g+0x1150);rt.g.global_1180=r16(g+0x1180);
    rt.g.global_11b0=r16(g+0x11b0);rt.g.global_11e0=r16(g+0x11e0);
    dispatch.actions=&actions;
    wf_status_t status=wf_r17_dispatch_1c03e_bounded(&rt,&rt.actors[slot],&dispatch);
    memcpy(g+0x5b0,rt.actors,sizeof rt.actors);memcpy(d,rt.m68k_regs.d,32);
    w16(g+0x1150,rt.g.global_1150);w16(g+0x1180,rt.g.global_1180);
    w16(g+0x11b0,rt.g.global_11b0);w16(g+0x11e0,rt.g.global_11e0);
    return status?-(int)dispatch.blocked_dependency_rom:0;
}
