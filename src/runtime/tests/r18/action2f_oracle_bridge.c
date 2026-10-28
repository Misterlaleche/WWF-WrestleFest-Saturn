#include <string.h>
#include "wf_r17_dispatch_1c03e.h"

static uint16_t b16(const uint8_t*p){return (uint16_t)(((unsigned)p[0]<<8)|p[1]);}
static uint32_t b32(const uint8_t*p){return ((uint32_t)b16(p)<<16)|b16(p+2);}
static void w32(uint8_t*p,uint32_t v){for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(v>>(24u-i*8u));}
static wf_status_t video(wf_runtime_t *rt,const wf_r17_fg0_write_t *writes,size_t count,void *user){
    (void)rt;uint8_t *mem=user;
    for(size_t i=0;i<count;i++)for(unsigned j=0;j<writes[i].width;j++)
        mem[writes[i].addr+j]=(uint8_t)(writes[i].value>>(8u*(writes[i].width-1u-j)));
    return WF_OK;
}

/* Host adapter only: enter the production bounded dispatcher and expose all
 * RAM/register effects required by the unchanged $16C9E arcade owner. */
int wf_action2f_oracle_with_descriptor(uint8_t *mem,uint32_t *regs,unsigned slot,uint32_t *descriptor){
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
    rt.g.global_15d2=g[0x15d2];rt.g.global_15d3=g[0x15d3];
    rt.g.global_007e=b16(g+0x7e);rt.g.credited_route=b16(g+0x7c);rt.g.mode_0161=g[0x161];rt.g.progression_0162=b16(g+0x162);
    rt.g.rng_state=b32(g+0x5c);rt.g.rng_state_valid=1;rt.g.frame_counter=b32(g+0x80);
    wf_status_t status=wf_r17_dispatch_1c03e_bounded(&rt,&rt.actors[slot],&dispatch);
    memcpy(g+0x5b0,rt.actors,sizeof rt.actors);
    memcpy(g+0x11f4,rt.referee.b,sizeof rt.referee.b);
    memcpy(regs,rt.m68k_regs.d,sizeof rt.m68k_regs.d);
    g[0x15d2]=rt.g.global_15d2;g[0x15d3]=rt.g.global_15d3;
    w32(g+0x5c,rt.g.rng_state);
    if(descriptor)*descriptor=actions.trace[slot].tick_descriptor_rom;
    return status?-(int)dispatch.blocked_dependency_rom:0;
}
int wf_action2f_oracle(uint8_t *mem,uint32_t *regs,unsigned slot){
    return wf_action2f_oracle_with_descriptor(mem,regs,slot,0);
}
