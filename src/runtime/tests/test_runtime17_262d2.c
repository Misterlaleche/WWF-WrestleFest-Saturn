#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "wf_r17_post7a00.h"

typedef struct {
    wf_r17_fg0_write_t fg[32]; size_t nfg;
    wf_r17_fg0_write_t video[8]; size_t nvideo;
    uint16_t audio[8]; size_t naudio;
    char order[16]; size_t norder;
    int block_fg,block_video,block_audio;
} sink_t;

static wf_status_t fg_sink(wf_runtime_t *rt,const wf_r17_fg0_write_t *w,size_t n,void *u){
    sink_t *s=(sink_t*)u;(void)rt;
    if(s->block_fg)return WF_BLOCKED;
    assert(n<=32u);memcpy(s->fg,w,n*sizeof(*w));s->nfg=n;s->order[s->norder++]='F';return WF_OK;
}
static wf_status_t video_sink(wf_runtime_t *rt,const wf_r17_fg0_write_t *w,size_t n,void *u){
    sink_t *s=(sink_t*)u;(void)rt;
    if(s->block_video)return WF_BLOCKED;
    assert(n<=8u);memcpy(s->video,w,n*sizeof(*w));s->nvideo=n;s->order[s->norder++]='V';return WF_OK;
}
static wf_status_t audio_sink(wf_runtime_t *rt,uint16_t raw,void *u){
    sink_t *s=(sink_t*)u;(void)rt;
    if(s->block_audio)return WF_BLOCKED;
    assert(s->naudio<8u);s->audio[s->naudio++]=raw;s->order[s->norder++]='A';return WF_OK;
}
static void init(wf_runtime_t *r,wf_r17_post7_ctx_t *c,sink_t *s){
    memset(r,0,sizeof(*r));memset(c,0,sizeof(*c));memset(s,0,sizeof(*s));
    wf_r17_m68k_software_reset(&r->m68k_regs);
    r->m68k_regs.d[0]=0xAAAA1111u;r->m68k_regs.d[1]=0xBBBB2222u;
    r->m68k_regs.d[2]=0xCCCC3333u;r->m68k_regs.d[3]=0xDDDD4444u;
    c->fg0_batch=fg_sink;c->video_batch=video_sink;c->audio_2052=audio_sink;c->user=s;
}
static void enable(wf_runtime_t *r){r->g.credited_route=1u;r->g.global_169e=0x8000u;}

static void test_gates_transparent(void){
    wf_runtime_t r;wf_r17_post7_ctx_t c;sink_t s;wf_r17_m68k_shadow_t before;
    init(&r,&c,&s);before=r.m68k_regs;assert(wf_r17_owner_262d2(&r,&c)==WF_OK);assert(memcmp(&before,&r.m68k_regs,sizeof(before))==0);assert(s.nfg==0&&s.nvideo==0&&s.naudio==0);
    init(&r,&c,&s);r.g.credited_route=1;r.g.global_169e=0x1000u|0x8000u;before=r.m68k_regs;assert(wf_r17_owner_262d2(&r,&c)==WF_OK);assert(memcmp(&before,&r.m68k_regs,sizeof(before))==0);
    init(&r,&c,&s);r.g.credited_route=1;r.g.global_169e=0;before=r.m68k_regs;assert(wf_r17_owner_262d2(&r,&c)==WF_OK);assert(memcmp(&before,&r.m68k_regs,sizeof(before))==0);
}
static void test_first_call_visible_3000(void){
    wf_runtime_t r;wf_r17_post7_ctx_t c;sink_t s;init(&r,&c,&s);enable(&r);
    assert(wf_r17_owner_262d2(&r,&c)==WF_OK);
    assert((r.g.global_169e&0x4000u)!=0u);assert(r.g.global_16a0==0x0030u);assert(r.g.global_16a2==0x0000u);assert(r.g.global_16a4==4u);
    assert(s.nvideo==2u);assert(s.video[0].addr==0x0018071cu&&s.video[0].width==2u&&s.video[0].value==0x00ffu);assert(s.video[1].addr==0x0018071eu&&s.video[1].value==0x000fu);
    assert(s.nfg==22u);assert(s.fg[0].addr==0x000c1d04u&&s.fg[0].value==0x009c00e4u);assert(s.fg[1].addr==0x000c1e04u&&s.fg[1].value==0x00a200e4u);
    assert(s.fg[12].addr==0x000c1d24u&&s.fg[12].value==0x00b200e4u);assert(s.fg[13].addr==0x000c1e24u&&s.fg[13].value==0x00b200e4u);
    assert(s.fg[14].addr==0x000c1d1cu&&s.fg[14].value==0x00ab00e4u);assert(s.fg[16].addr==0x000c1d20u&&s.fg[16].value==0x00a800e4u);
    assert(s.fg[18].addr==0x000c1d28u&&s.fg[18].value==0x00a800e4u);assert(s.fg[20].addr==0x000c1d2cu&&s.fg[20].value==0x00a800e4u);
    assert(s.norder==2u&&s.order[0]=='V'&&s.order[1]=='F');
    assert(r.m68k_regs.d[0]==0x00e40000u);assert(r.m68k_regs.d[1]==0x00000010u);assert(r.m68k_regs.d[2]==0x00b300e4u);assert(r.m68k_regs.d[3]==0xDDDD4444u);
}
static void test_divisor_and_2959(void){
    wf_runtime_t r;wf_r17_post7_ctx_t c;sink_t s;unsigned i;init(&r,&c,&s);enable(&r);assert(wf_r17_owner_262d2(&r,&c)==WF_OK);
    for(i=0;i<3u;i++){memset(&s,0,sizeof(s));c.user=&s;assert(wf_r17_owner_262d2(&r,&c)==WF_OK);assert(s.nfg==0&&s.nvideo==0&&s.naudio==0);}
    assert(r.g.global_16a4==1u&&r.g.global_16a0==0x30u&&r.g.global_16a2==0u);
    memset(&s,0,sizeof(s));c.user=&s;assert(wf_r17_owner_262d2(&r,&c)==WF_OK);assert(r.g.global_16a4==4u&&r.g.global_16a0==0x29u&&r.g.global_16a2==0x59u);assert(s.nfg==8u&&s.nvideo==0u&&s.naudio==0u);
    assert(s.fg[0].value==0x00aa00e4u);assert(s.fg[2].value==0x00b100e4u);assert(s.fg[4].value==0x00ad00e4u);assert(s.fg[6].value==0x00b100e4u);
}
static void test_minute05_audio_before_render(void){
    wf_runtime_t r;wf_r17_post7_ctx_t c;sink_t s;init(&r,&c,&s);r.g.credited_route=1u;r.g.global_169e=0xC000u;r.g.global_16a4=1u;r.g.global_16a0=0x0006u;r.g.global_16a2=0u;
    assert(wf_r17_owner_262d2(&r,&c)==WF_OK);assert(r.g.global_16a0==0x05u&&r.g.global_16a2==0x59u);assert((r.g.global_169e&0x2000u)!=0u);assert(s.naudio==1u&&s.audio[0]==0x000bu);assert(s.nfg==8u&&s.nvideo==0u);assert(s.norder==2u&&s.order[0]=='A'&&s.order[1]=='F');
    assert(r.m68k_regs.d[0]==0xAAAA0009u);assert(r.m68k_regs.d[1]==0x10u);assert(r.m68k_regs.d[2]==0x00bc00e4u);
}
static void test_timeup_failclosed(void){
    wf_runtime_t r,b;wf_r17_post7_ctx_t c;sink_t s;init(&r,&c,&s);r.g.credited_route=1u;r.g.global_169e=0xC000u;r.g.global_16a4=1u;r.g.global_16a0=0u;r.g.global_16a2=0u;b=r;
    assert(wf_r17_owner_262d2(&r,&c)==WF_BLOCKED);assert(memcmp(&r.g,&b.g,sizeof(r.g))==0);assert(memcmp(&r.m68k_regs,&b.m68k_regs,sizeof(r.m68k_regs))==0);assert(c.blocked_dependency_rom==0x0002503cu);assert(s.nfg==0&&s.naudio==0&&s.nvideo==0);
}
static void test_missing_video_rolls_back_init(void){
    wf_runtime_t r,b;wf_r17_post7_ctx_t c;sink_t s;init(&r,&c,&s);enable(&r);c.video_batch=NULL;b=r;
    assert(wf_r17_owner_262d2(&r,&c)==WF_BLOCKED);assert(memcmp(&r.g,&b.g,sizeof(r.g))==0);assert(memcmp(&r.m68k_regs,&b.m68k_regs,sizeof(r.m68k_regs))==0);assert(c.blocked_dependency_rom==0x0018071cu);assert(s.nfg==0&&s.naudio==0);
}
static void test_missing_fg_rolls_back(void){
    wf_runtime_t r,b;wf_r17_post7_ctx_t c;sink_t s;init(&r,&c,&s);enable(&r);c.fg0_batch=NULL;b=r;
    assert(wf_r17_owner_262d2(&r,&c)==WF_BLOCKED);assert(memcmp(&r.g,&b.g,sizeof(r.g))==0);assert(memcmp(&r.m68k_regs,&b.m68k_regs,sizeof(r.m68k_regs))==0);assert(c.blocked_dependency_rom==0x000c1d04u);assert(s.nvideo==0&&s.naudio==0);
}
int main(void){test_gates_transparent();test_first_call_visible_3000();test_divisor_and_2959();test_minute05_audio_before_render();test_timeup_failclosed();test_missing_video_rolls_back_init();test_missing_fg_rolls_back();puts("PASS $262D2 timer/FG0 ROM contract");return 0;}
