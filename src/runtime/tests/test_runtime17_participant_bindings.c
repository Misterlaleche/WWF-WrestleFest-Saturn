#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "wf_gameplay_bindings.h"
static uint16_t hardware[4];
static unsigned reads;
static size_t writes;
static wf_status_t read_word(wf_runtime_t *rt,uint32_t address,uint16_t *value,void *user){
 (void)rt;(void)user;
 assert(address>=0x140020u&&address<=0x140026u&&!(address&1u));
 reads++;*value=hardware[(address-0x140020u)/2u];return WF_OK;
}
static wf_status_t capture(wf_runtime_t *rt,const wf_r17_fg0_write_t *fg,size_t nf,
 const wf_r17_fg0_write_t *video,size_t nv,void *user){
 (void)rt;(void)video;(void)user;
 if(nf&&writes==0u)assert(fg[0].addr==0xc1731u);
 writes+=nf+nv;return WF_OK;
}
static void setup(wf_runtime_t *rt,wf_gameplay_binding_context_t *ctx,wf_phase_binding_t *bindings){
 size_t n=wf_gameplay_bindings_build(ctx,bindings,40);
 assert(n);wf_runtime_init(rt,bindings,n);
 rt->g.credited_route=1;rt->g.config_0066=4;rt->g.credit_count_004f=1;
 ctx->participant_input_word=read_word;ctx->participant_commit_batch=capture;
 for(unsigned i=0;i<4;i++)hardware[i]=0xffffu;
 reads=0;writes=0;
 assert(!ctx->post7_tag_6e34&&!ctx->post7_royal_9132);
}
int main(void){
 wf_runtime_t rt;wf_gameplay_binding_context_t ctx;wf_phase_binding_t bindings[40];
 setup(&rt,&ctx,bindings);rt.actors[0].b[0]=0x80;hardware[0]=0xff7fu;
 assert(wf_dispatch_phase(&rt,0x18c4u)==WF_OK);
 assert(reads==1u&&rt.g.credit_count_004f==0u);
 assert(wf_be16(&rt.actors[0],0x66)==110u&&wf_be16(&rt.actors[0],0x6e)==110u);
 assert(rt.actors[0].b[0xa1]==1u&&rt.m68k_regs.d[0]==2u);
 setup(&rt,&ctx,bindings);rt.g.mode_0161=1;rt.g.royal_slots_1586[3]=0x8000u;
 assert(wf_dispatch_phase(&rt,0x18c4u)==WF_OK);
 assert(writes>0u&&rt.g.global_167b==2u);
 assert(rt.m68k_regs.d[0]==3u&&rt.m68k_regs.d[1]==4u);
 setup(&rt,&ctx,bindings);ctx.participant_input_word=NULL;rt.actors[0].b[0]=0x80;
 assert(wf_dispatch_phase(&rt,0x18c4u)==WF_BLOCKED);
 assert(ctx.last_blocked_dependency==0x140020u&&rt.g.credit_count_004f==1u);
 puts("PASS native TAG/Royal defaults through $18C4 bindings; no gameplay override");
 return 0;
}
