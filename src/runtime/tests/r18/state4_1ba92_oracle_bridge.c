#include <string.h>
#include "wf_r17_dispatch_1c03e.h"

static wf_status_t video(wf_runtime_t *rt,const wf_r17_fg0_write_t *writes,size_t count,void *user){
    (void)rt;uint8_t *mem=user;
    for(size_t i=0;i<count;i++)for(unsigned j=0;j<writes[i].width;j++)
        mem[writes[i].addr+j]=(uint8_t)(writes[i].value>>(8u*(writes[i].width-1u-j)));
    return WF_OK;
}

/* Host adapter only: enter the production bounded dispatcher and expose all
 * RAM/register effects required by the unchanged $1BA92 arcade owner. */
int wf_state4_1ba92_oracle(uint8_t *mem,uint32_t *regs,unsigned slot){
    wf_runtime_t rt={0};
    wf_r17_actions_r11_bridge_t actions={0};
    wf_r17_dispatch_1c03e_ctx_t dispatch={0};
    uint8_t *g=mem+0x1c0000;
    dispatch.actions=&actions;
    actions.video_batch=video;actions.video_user=mem;
    memcpy(rt.actors,g+0x5b0,sizeof rt.actors);
    memcpy(rt.referee.b,g+0x11f4,sizeof rt.referee.b);
    memcpy(rt.m68k_regs.d,regs,sizeof rt.m68k_regs.d);
    rt.m68k_regs.valid=1;rt.g.global_1802=(uint16_t)((g[0x1802]<<8)|g[0x1803]);rt.g.global_007f=g[0x7f];
    wf_status_t status=wf_r17_dispatch_1c03e_bounded(&rt,&rt.actors[slot],&dispatch);
    memcpy(g+0x5b0,rt.actors,sizeof rt.actors);
    memcpy(g+0x11f4,rt.referee.b,sizeof rt.referee.b);
    memcpy(regs,rt.m68k_regs.d,sizeof rt.m68k_regs.d);g[0x1802]=(uint8_t)(rt.g.global_1802>>8);g[0x1803]=(uint8_t)rt.g.global_1802;
    return status?-(int)dispatch.blocked_dependency_rom:0;
}
