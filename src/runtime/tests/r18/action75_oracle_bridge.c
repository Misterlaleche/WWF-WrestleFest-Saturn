#include <string.h>
#include "wf_r17_dispatch_1c03e.h"

static uint16_t b16(const uint8_t*p){return (uint16_t)(((unsigned)p[0]<<8)|p[1]);}
static void word(uint8_t*p,uint16_t v){p[0]=(uint8_t)(v>>8);p[1]=(uint8_t)v;}

/* Test-only adapter; rope-record globals are copied by their exact widths. */
int wf_action75_oracle(uint8_t*mem,uint32_t*regs,unsigned slot){
    wf_runtime_t rt={0};wf_r17_actions_r11_bridge_t actions={0};
    wf_r17_dispatch_1c03e_ctx_t dispatch={0};dispatch.actions=&actions;
    uint8_t*g=mem+0x1c0000;
    memcpy(rt.actors,g+0x5b0,sizeof rt.actors);memcpy(rt.m68k_regs.d,regs,32);rt.m68k_regs.valid=1;
    rt.g.mode_0161=g[0x161];
    rt.g.global_1150=b16(g+0x1150);rt.g.global_1180=b16(g+0x1180);
    rt.g.global_11b0=b16(g+0x11b0);rt.g.global_11e0=b16(g+0x11e0);
    wf_status_t status=wf_r17_dispatch_1c03e_bounded(&rt,&rt.actors[slot],&dispatch);
    memcpy(g+0x5b0,rt.actors,sizeof rt.actors);memcpy(regs,rt.m68k_regs.d,32);
    word(g+0x1150,rt.g.global_1150);word(g+0x1180,rt.g.global_1180);
    word(g+0x11b0,rt.g.global_11b0);word(g+0x11e0,rt.g.global_11e0);
    return status?-(int)dispatch.blocked_dependency_rom:0;
}
