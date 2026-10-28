#include <string.h>
#include "wf_r17_dispatch_1c03e.h"

static uint16_t b16(const uint8_t*p){return (uint16_t)(((unsigned)p[0]<<8)|p[1]);}
static uint32_t b32(const uint8_t*p){return ((uint32_t)b16(p)<<16)|b16(p+2);}
static void w(uint8_t*p,uint32_t v,unsigned n){for(unsigned i=0;i<n;i++)p[i]=(uint8_t)(v>>(8u*(n-1u-i)));}
static wf_status_t video(wf_runtime_t*rt,const wf_r17_fg0_write_t*writes,size_t count,void*user){
    (void)rt;uint8_t*mem=user;
    for(size_t i=0;i<count;i++)w(mem+writes[i].addr,writes[i].value,writes[i].width);
    return WF_OK;
}

/* Host adapter: preserves the natural snapshot and enters the production
 * bounded dispatcher; compact render queues append to the real ROM lists. */
int wf_action50_oracle(uint8_t*mem,uint32_t*regs,unsigned slot,uint32_t*descriptor){
    wf_runtime_t rt={0};wf_r17_actions_r11_bridge_t actions={0};
    wf_r17_dispatch_1c03e_ctx_t dispatch={0};dispatch.actions=&actions;
    wf_r17_render_queue_t queue={0};wf_r17_render_ctx_t render={0};uint8_t*g=mem+0x1c0000;
    memcpy(rt.actors,g+0x5b0,sizeof rt.actors);memcpy(rt.referee.b,g+0x11f4,sizeof rt.referee.b);
    memcpy(rt.transient_1258,g+0x1258,sizeof rt.transient_1258);
    memcpy(rt.m68k_regs.d,regs,32);rt.m68k_regs.valid=1;
    rt.g.credited_route=b16(g+0x7c);rt.g.config_0066=b16(g+0x66);rt.g.mode_0161=g[0x161];
    rt.g.global_1682=b16(g+0x1682);rt.g.global_169e=b16(g+0x169e);rt.g.global_1697=g[0x1697];
    rt.g.global_15d2=g[0x15d2];rt.g.global_15d3=g[0x15d3];
    rt.g.rng_state=b32(g+0x5c);rt.g.rng_state_valid=1;rt.g.frame_counter=b32(g+0x80);
    render.queue=&queue;render.visibility_2188=wf_r17_visibility_2188_exact;
    render.scroll_a_x=b16(g+0x17e6);render.scroll_a_y=b16(g+0x17ee);actions.render=&render;
    actions.video_batch=video;actions.video_user=mem;
    wf_status_t status=wf_r17_dispatch_1c03e_bounded(&rt,&rt.actors[slot],&dispatch);
    memcpy(g+0x5b0,rt.actors,sizeof rt.actors);memcpy(g+0x11f4,rt.referee.b,sizeof rt.referee.b);
    memcpy(g+0x1258,rt.transient_1258,sizeof rt.transient_1258);memcpy(regs,rt.m68k_regs.d,32);
    w(g+0x1682,rt.g.global_1682,2);w(g+0x169e,rt.g.global_169e,2);g[0x1697]=rt.g.global_1697;
    g[0x15d2]=rt.g.global_15d2;g[0x15d3]=rt.g.global_15d3;w(g+0x5c,rt.g.rng_state,4);
    for(unsigned k=0;k<6;k++)if(queue.count[k]){
        uint32_t list=b32(mem+0x2806+k*8),counter=b32(mem+0x280a+k*8);uint16_t first=b16(mem+counter);
        for(unsigned j=0;j<queue.count[k];j++)w(mem+list+(first+j)*4,0x1c1258u+(uint32_t)(queue.list[k][j]-actions.transient_views)*0x2au,4);
        w(mem+counter,first+queue.count[k],2);
    }
    if(descriptor)*descriptor=actions.trace[slot].tick_descriptor_rom;
    return status?-(int)dispatch.blocked_dependency_rom:0;
}
