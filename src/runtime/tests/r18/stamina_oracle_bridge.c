/* Test-only raw projection for unmodified 68000 stamina owners. */
#include "wf_core_phases.h"
#include "wf_r17_participant_helpers.h"
#include <string.h>

typedef struct sink {
    uint8_t *memory;
    uint32_t *trace;
    size_t count;
} sink_t;
static uint16_t be16(const uint8_t *p){return (uint16_t)((p[0]<<8)|p[1]);}
static wf_status_t emit(wf_runtime_t *rt,const wf_r17_fg0_write_t *w,size_t n,void *user){
    sink_t *s=user;(void)rt;
    for(size_t i=0;i<n;i++){
        if(s->count>=4096u)return WF_BAD_STATE;
        s->trace[s->count*3]=w[i].addr;s->trace[s->count*3+1]=w[i].value;s->trace[s->count*3+2]=w[i].width;s->count++;
        for(unsigned j=0;j<w[i].width;j++)s->memory[w[i].addr+j]=(uint8_t)(w[i].value>>(8u*(w[i].width-j-1u)));
    }
    return WF_OK;
}
int wf_stamina_oracle(uint8_t *memory,uint32_t *d,uint32_t *a,unsigned owner,uint32_t *trace,unsigned *count){
    wf_runtime_t rt;wf_r17_post7_ctx_t ctx;wf_status_t status;uint8_t *g=memory+0x1c0000;
    sink_t sink={memory,trace,0u};
    memset(&rt,0,sizeof(rt));memset(&ctx,0,sizeof(ctx));
    memcpy(rt.actors,g+0x5b0,sizeof(rt.actors));memcpy(rt.m68k_regs.d,d,sizeof(rt.m68k_regs.d));rt.m68k_regs.valid=1u;
    rt.g.credited_route=be16(g+0x7c);rt.g.global_16c4=be16(g+0x16c4);rt.g.mode_0161=g[0x161];
    ctx.fg0_batch=emit;ctx.user=&sink;
    if(owner==0u)status=wf_phase_damage(&rt,0x24e58u,NULL);
    else if(owner==1u)status=wf_r17_owner_7548(&rt,&ctx);
    else if(owner==2u)status=wf_r17_owner_8ecc(&rt);
    else{
        wf_participant_machine_t machine;wf_participant_init(&machine,&rt,NULL,NULL);memcpy(machine.a,a,sizeof(machine.a));
        if(owner==3u)wf_participant_7506(&machine);
        else if(owner==4u)wf_participant_75ee(&machine);
        else if(owner==5u)wf_participant_782c(&machine);
        else return WF_BAD_STATE;
        status=machine.status;
        if(status==WF_OK)status=emit(&rt,machine.writes,machine.write_count,&sink);
        memcpy(rt.m68k_regs.d,machine.d,sizeof(machine.d));memcpy(a,machine.a,sizeof(machine.a));
    }
    memcpy(g+0x5b0,rt.actors,sizeof(rt.actors));memcpy(d,rt.m68k_regs.d,sizeof(rt.m68k_regs.d));*count=(unsigned)sink.count;
    return status;
}
