#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "wf_r17_post7a00.h"

typedef struct sink {
    unsigned seq_n; char seq[8];
    wf_r17_fg0_write_t video[4]; size_t nvideo;
    wf_r17_fg0_write_t fg[64]; size_t nfg;
    uint16_t raw; unsigned nraw;
} sink_t;
static wf_status_t vb(wf_runtime_t *rt,const wf_r17_fg0_write_t *w,size_t n,void *u){
    sink_t*s=(sink_t*)u;(void)rt;assert(s->seq_n<8u);s->seq[s->seq_n++]='V';assert(n<=4u);memcpy(s->video,w,n*sizeof(*w));s->nvideo=n;return WF_OK;
}
static wf_status_t fg(wf_runtime_t *rt,const wf_r17_fg0_write_t *w,size_t n,void *u){
    sink_t*s=(sink_t*)u;(void)rt;assert(s->seq_n<8u);s->seq[s->seq_n++]='F';assert(n<=64u);memcpy(s->fg,w,n*sizeof(*w));s->nfg=n;return WF_OK;
}
static wf_status_t raw(wf_runtime_t *rt,uint16_t v,void *u){
    sink_t*s=(sink_t*)u;(void)rt;assert(s->seq_n<8u);s->seq[s->seq_n++]='R';s->raw=v;s->nraw++;return WF_OK;
}
static void init(wf_runtime_t*r,wf_r17_post7_ctx_t*c,sink_t*s){
    memset(r,0,sizeof(*r));memset(c,0,sizeof(*c));memset(s,0,sizeof(*s));r->m68k_regs.valid=1u;
    r->g.mode_0161=1u;c->video_batch=vb;c->fg0_batch=fg;c->raw_2503c=raw;c->user=s;
    r->m68k_regs.d[0]=0xaaaa1234u;r->m68k_regs.d[1]=0xbbbb5678u;
}
static void test_nonroyal_gate(void){
    wf_runtime_t r,b;wf_r17_post7_ctx_t c;sink_t s;init(&r,&c,&s);r.g.mode_0161=0u;b=r;
    assert(wf_r17_owner_bcfe(&r,&c)==WF_OK);assert(memcmp(&r,&b,sizeof(r))==0);assert(s.seq_n==0u);puts("PASS $BCFE non-Royal gate");
}
static void test_no_active(void){
    wf_runtime_t r;wf_r17_post7_ctx_t c;sink_t s;init(&r,&c,&s);
    assert(wf_r17_owner_bcfe(&r,&c)==WF_OK);assert((uint16_t)r.m68k_regs.d[0]==4u);assert((uint16_t)r.m68k_regs.d[1]==6u);assert(s.seq_n==0u);puts("PASS $BCFE four-slot scan exit registers");
}
static void test_video_then_2503c(void){
    wf_runtime_t r;wf_r17_post7_ctx_t c;sink_t s;init(&r,&c,&s);r.g.global_0156[2]=0x0201u;r.g.pipeline_mode=0u;
    assert(wf_r17_owner_bcfe(&r,&c)==WF_OK);assert(r.g.global_0156[2]==0x0101u);assert(s.seq_n==2u&&s.seq[0]=='V'&&s.seq[1]=='R');
    assert(s.nvideo==1u&&s.video[0].addr==0x00180012u&&s.video[0].value==0x000fu&&s.video[0].width==2u);assert(s.nraw==1u&&s.raw==0x004eu);
    assert((uint16_t)r.m68k_regs.d[0]==2u);assert((uint16_t)r.m68k_regs.d[1]==4u);puts("PASS $BCFE $BD88 -> $2503C exact order");
}
static void test_video_then_clear(void){
    wf_runtime_t r;wf_r17_post7_ctx_t c;sink_t s;init(&r,&c,&s);r.g.global_0156[1]=0x0201u;r.g.pipeline_mode=0x0au;
    assert(wf_r17_owner_bcfe(&r,&c)==WF_OK);assert(r.g.global_0156[1]==0x0101u);assert(s.seq_n==2u&&s.seq[0]=='V'&&s.seq[1]=='F');
    assert(s.video[0].value==0x0fffu);assert(s.nfg==48u);assert(s.fg[0].addr==0x000c0520u&&s.fg[47].addr==0x000c067cu);
    assert(r.m68k_regs.d[0]==0x0000ffffu);assert((uint16_t)r.m68k_regs.d[1]==0xffffu);puts("PASS $BCFE bit3 clear-panel route/order");
}
static void test_expire(void){
    wf_runtime_t r;wf_r17_post7_ctx_t c;sink_t s;init(&r,&c,&s);r.g.global_0156[0]=0x0101u;
    assert(wf_r17_owner_bcfe(&r,&c)==WF_OK);assert(r.g.global_0156[0]==0u);assert(s.seq_n==2u&&s.seq[0]=='V'&&s.seq[1]=='F');assert(s.video[0].value==0x06ffu);assert(s.nfg==48u);puts("PASS $BCFE expiration -> $06FF then FG clear");
}
static void test_missing_dependency_rollback(void){
    wf_runtime_t r,b;wf_r17_post7_ctx_t c;sink_t s;init(&r,&c,&s);r.g.global_0156[0]=0x0201u;c.video_batch=0;b=r;
    assert(wf_r17_owner_bcfe(&r,&c)==WF_BLOCKED);assert(c.blocked_dependency_rom==0x00180012u);assert(memcmp(r.g.global_0156,b.g.global_0156,sizeof(r.g.global_0156))==0);assert(memcmp(&r.m68k_regs,&b.m68k_regs,sizeof(r.m68k_regs))==0);puts("PASS $BCFE fail-closed rollback");
}
int main(void){test_nonroyal_gate();test_no_active();test_video_then_2503c();test_video_then_clear();test_expire();test_missing_dependency_rollback();return 0;}
