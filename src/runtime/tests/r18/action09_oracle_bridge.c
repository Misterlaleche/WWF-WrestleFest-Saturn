#include <string.h>
#include "wf_r17_dispatch_1c03e.h"

static uint16_t b16(const uint8_t*p){return (uint16_t)(((unsigned)p[0]<<8)|p[1]);}
static void word(uint8_t*p,uint16_t v){p[0]=(uint8_t)(v>>8);p[1]=(uint8_t)v;}
static wf_status_t video(wf_runtime_t*rt,const wf_r17_fg0_write_t*writes,size_t count,void*user){
    (void)rt;uint8_t*mem=user;
    for(size_t i=0;i<count;i++)for(unsigned k=0;k<writes[i].width;k++)
        mem[writes[i].addr+k]=(uint8_t)(writes[i].value>>(8u*(writes[i].width-1u-k)));
    return WF_OK;
}

/* Test-only adapter; the production bounded dispatcher owns every action. */
int wf_action09_oracle(uint8_t*mem,uint32_t*regs,unsigned slot){
    wf_runtime_t rt={0};wf_r17_actions_r11_bridge_t actions={0};
    wf_r17_dispatch_1c03e_ctx_t dispatch={0};dispatch.actions=&actions;
    uint8_t*g=mem+0x1c0000;
    memcpy(rt.actors,g+0x5b0,sizeof rt.actors);memcpy(rt.referee.b,g+0x11f4,sizeof rt.referee.b);
    memcpy(rt.m68k_regs.d,regs,32);rt.m68k_regs.valid=1;
    rt.g.mode_0161=g[0x161];rt.g.credited_route=b16(g+0x7c);rt.g.config_0066=b16(g+0x66);
    rt.g.global_15d2=g[0x15d2];rt.g.global_15d3=g[0x15d3];
    rt.g.global_1682=b16(g+0x1682);rt.g.global_169e=b16(g+0x169e);
    actions.video_batch=video;actions.video_user=mem;
    wf_status_t status=wf_r17_dispatch_1c03e_bounded(&rt,&rt.actors[slot],&dispatch);
    memcpy(g+0x5b0,rt.actors,sizeof rt.actors);memcpy(g+0x11f4,rt.referee.b,sizeof rt.referee.b);
    memcpy(regs,rt.m68k_regs.d,32);
    g[0x15d2]=rt.g.global_15d2;g[0x15d3]=rt.g.global_15d3;
    word(g+0x1682,rt.g.global_1682);word(g+0x169e,rt.g.global_169e);
    return status?-(int)dispatch.blocked_dependency_rom:0;
}
