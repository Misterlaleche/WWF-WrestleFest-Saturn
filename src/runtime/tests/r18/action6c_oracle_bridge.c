#include <string.h>
#include "wf_r17_dispatch_1c03e.h"

static unsigned b16(const uint8_t *p){return ((unsigned)p[0]<<8)|p[1];}
static uint32_t b32(const uint8_t *p){return ((uint32_t)b16(p)<<16)|b16(p+2);}
static void w16(uint8_t *p,uint16_t v){p[0]=(uint8_t)(v>>8);p[1]=(uint8_t)v;}
static void w32(uint8_t *p,uint32_t v){w16(p,(uint16_t)(v>>16));w16(p+2,(uint16_t)v);}
static wf_status_t video(wf_runtime_t *rt,const wf_r17_fg0_write_t *writes,size_t count,void *user){
    (void)rt;uint8_t *mem=user;
    for(size_t i=0;i<count;i++)for(unsigned j=0;j<writes[i].width;j++)
        mem[writes[i].addr+j]=(uint8_t)(writes[i].value>>(8u*(writes[i].width-1u-j)));
    return WF_OK;
}

/* Host adapter only: enter the production bounded dispatcher and expose all
 * RAM/register effects required by the unchanged $19C48 arcade owner. */
int wf_action6c_oracle(uint8_t *mem,uint32_t *regs,unsigned slot){
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
    rt.g.mode_0161=g[0x161];
    rt.g.global_167a=g[0x167a];
    rt.g.global_1682=(uint16_t)b16(g+0x1682);
    rt.g.global_168c=b32(g+0x168c);
    rt.g.global_1692=(uint16_t)b16(g+0x1692);
    wf_status_t status=wf_r17_dispatch_1c03e_bounded(&rt,&rt.actors[slot],&dispatch);
    memcpy(g+0x5b0,rt.actors,sizeof rt.actors);
    memcpy(g+0x11f4,rt.referee.b,sizeof rt.referee.b);
    memcpy(regs,rt.m68k_regs.d,sizeof rt.m68k_regs.d);
    g[0x167a]=rt.g.global_167a;
    w16(g+0x1682,rt.g.global_1682);
    w32(g+0x168c,rt.g.global_168c);
    w16(g+0x1692,rt.g.global_1692);
    return status?-(int)dispatch.blocked_dependency_rom:0;
}
