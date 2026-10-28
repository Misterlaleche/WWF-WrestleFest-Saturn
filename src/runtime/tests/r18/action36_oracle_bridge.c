#include <string.h>
#include "wf_r17_dispatch_1c03e.h"

static unsigned b16(const uint8_t *p){return ((unsigned)p[0]<<8)|p[1];}
static void w16(uint8_t *p,uint16_t value){p[0]=(uint8_t)(value>>8);p[1]=(uint8_t)value;}
static wf_status_t video(wf_runtime_t *rt,const wf_r17_fg0_write_t *writes,size_t count,void *user){
    (void)rt;uint8_t *mem=user;
    for(size_t i=0;i<count;i++)for(unsigned j=0;j<writes[i].width;j++)
        mem[writes[i].addr+j]=(uint8_t)(writes[i].value>>(8u*(writes[i].width-1u-j)));
    return WF_OK;
}

/* Host-only adapter. The comparison enters the real bounded dispatcher and
 * compares every actor byte and D0-D7 with the unchanged 68000 owner. */
int action36_oracle(uint8_t *mem,uint32_t *regs,unsigned slot,uint32_t *descriptor){
    wf_runtime_t rt={0};
    wf_r17_actions_r11_bridge_t actions={0};
    wf_r17_dispatch_1c03e_ctx_t dispatch={0};
    uint8_t *g=mem+0x1c0000;
    dispatch.actions=&actions;
    actions.video_batch=video;actions.video_user=mem;
    memcpy(rt.actors,g+0x5b0,sizeof rt.actors);
    memcpy(rt.referee.b,g+0x11f4,sizeof rt.referee.b);
    memcpy(rt.m68k_regs.d,regs,sizeof rt.m68k_regs.d);
    rt.m68k_regs.valid=1;
    rt.g.credited_route=(uint16_t)b16(g+0x7c);
    rt.g.config_0066=(uint16_t)b16(g+0x66);
    rt.g.mode_0161=g[0x161];
    rt.g.global_1682=(uint16_t)b16(g+0x1682);
    rt.g.global_169e=(uint16_t)b16(g+0x169e);rt.g.global_1697=g[0x1697];
    rt.g.global_15d2=g[0x15d2];rt.g.global_15d3=g[0x15d3];
    wf_status_t status=wf_r17_dispatch_1c03e_bounded(&rt,&rt.actors[slot],&dispatch);
    memcpy(g+0x5b0,rt.actors,sizeof rt.actors);
    memcpy(g+0x11f4,rt.referee.b,sizeof rt.referee.b);
    memcpy(regs,rt.m68k_regs.d,sizeof rt.m68k_regs.d);
    w16(g+0x1682,rt.g.global_1682);
    w16(g+0x169e,rt.g.global_169e);g[0x1697]=rt.g.global_1697;
    g[0x15d2]=rt.g.global_15d2;g[0x15d3]=rt.g.global_15d3;
    if(descriptor)*descriptor=actions.trace[slot].tick_descriptor_rom;
    return status?-(int)dispatch.blocked_dependency_rom:0;
}
