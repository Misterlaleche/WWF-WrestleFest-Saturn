#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "wf_r17_post7a00.h"

typedef struct call9 { uint16_t d2,d7; uint32_t dest; } call9_t;
typedef struct sink {
    char seq[16]; unsigned seq_n;
    wf_r17_fg0_write_t fg[273]; size_t nfg;
    wf_r17_fg0_write_t video[16]; size_t nvideo;
    call9_t a[4]; unsigned na;
} sink_t;

static wf_status_t fg_cb(wf_runtime_t *rt,const wf_r17_fg0_write_t *w,size_t n,void *u){
    sink_t*s=(sink_t*)u;(void)rt;assert(s->seq_n<16u);s->seq[s->seq_n++]='F';assert(n<=273u);memcpy(s->fg,w,n*sizeof(*w));s->nfg=n;return WF_OK;
}
static wf_status_t video_cb(wf_runtime_t *rt,const wf_r17_fg0_write_t *w,size_t n,void *u){
    sink_t*s=(sink_t*)u;(void)rt;assert(s->seq_n<16u);s->seq[s->seq_n++]='V';assert(n<=16u);memcpy(s->video,w,n*sizeof(*w));s->nvideo=n;return WF_OK;
}
static wf_status_t raw9_cb(wf_runtime_t *rt,uint16_t d2,uint16_t d7,uint32_t dest,void *u){
    sink_t*s=(sink_t*)u;unsigned i;assert(s->seq_n<16u&&s->na<4u);s->seq[s->seq_n++]='A';s->a[s->na++]=(call9_t){d2,d7,dest};
    /* Prove $9ACA's MOVEM contract is enforced by the wrapper. */
    for(i=0u;i<8u;i++)rt->m68k_regs.d[i]=0xdead0000u+i;
    return WF_OK;
}
static void init(wf_runtime_t*r,wf_r17_post7_ctx_t*c,sink_t*s){
    memset(r,0,sizeof(*r));memset(c,0,sizeof(*c));memset(s,0,sizeof(*s));
    r->m68k_regs.valid=1u;r->m68k_regs.d[0]=0xaaaa1234u;r->m68k_regs.d[1]=0xbbbb5678u;
    c->fg0_batch=fg_cb;c->video_batch=video_cb;c->raw_9aca=raw9_cb;c->user=s;
}
static void test_gate_16c5(void){
    wf_runtime_t r,b;wf_r17_post7_ctx_t c;sink_t s;init(&r,&c,&s);r.g.global_16c4=0x0001u;b=r;
    assert(wf_r17_owner_c04a(&r,&c)==WF_OK);assert(memcmp(&r,&b,sizeof(r))==0);assert(s.seq_n==0u);puts("PASS $C04A $16C5 gate");
}
static void test_credit_carry(void){
    wf_runtime_t r;wf_r17_post7_ctx_t c;sink_t s;init(&r,&c,&s);r.g.credit_count_004f=1u;
    assert(wf_r17_owner_c04a(&r,&c)==WF_OK);assert((uint16_t)r.m68k_regs.d[0]==3u);assert(s.seq_n==0u);assert((r.g.global_167b&1u)==0u);puts("PASS $C04A $C404 credit-carry gate");
}
static void test_start_warning_full_sequence(void){
    wf_runtime_t r;wf_r17_post7_ctx_t c;sink_t s;init(&r,&c,&s);r.g.global_16ca=0u;
    wf_set_u8(&r.actors[0],WF_A_ACTIVE,0x80u);wf_set_u8(&r.actors[0],WF_A_ROLE,0u);wf_set_u8(&r.actors[0],WF_A_STAMINA+1u,0x20u);
    assert(wf_r17_owner_c04a(&r,&c)==WF_OK);
    assert((r.g.global_167b&1u)!=0u);assert(r.g.global_16ca==0x0080u);assert(r.g.global_16cc==0xc02fu);
    assert(s.seq_n==4u&&s.seq[0]=='F'&&s.seq[1]=='V'&&s.seq[2]=='A'&&s.seq[3]=='A');
    assert(s.nfg==273u);assert(s.fg[0].addr==0x000c1004u&&s.fg[0].value==0u&&s.fg[0].width==4u);assert(s.fg[272].addr==0x000c1c54u);
    assert(s.nvideo==16u);assert(s.video[0].addr==0x0018031eu&&s.video[0].value==0x0fd0u&&s.video[0].width==2u);assert(s.video[15].addr==0x00180300u&&s.video[15].value==0u);
    assert(s.na==2u);assert(s.a[0].d2==0x0011u&&s.a[0].d7==0x0010u&&s.a[0].dest==0x000c1804u);assert(s.a[1].d2==0x0012u&&s.a[1].d7==0x0060u&&s.a[1].dest==0x000c1224u);
    assert((uint16_t)r.m68k_regs.d[2]==0x0012u);assert((uint16_t)r.m68k_regs.d[7]==0x0060u);
    puts("PASS $C04A start -> clear/video/$9ACA exact order");
}
static void test_existing_warning(void){
    wf_runtime_t r;wf_r17_post7_ctx_t c;sink_t s;init(&r,&c,&s);r.g.global_167b=1u;r.g.global_16cc=0xc001u;
    assert(wf_r17_owner_c04a(&r,&c)==WF_OK);assert(r.g.global_16cc==0xc000u);assert(s.seq_n==1u&&s.seq[0]=='A');assert(s.na==1u);assert(s.a[0].d2==0x0013u&&s.a[0].d7==0x0060u&&s.a[0].dest==0x000c1224u);puts("PASS $C04A existing warning countdown");
}
static void test_expire(void){
    wf_runtime_t r;wf_r17_post7_ctx_t c;sink_t s;init(&r,&c,&s);r.g.global_167b=1u;r.g.global_16cc=0xc000u;
    assert(wf_r17_owner_c04a(&r,&c)==WF_OK);assert((r.g.global_167b&1u)==0u);assert(r.g.global_16cc==0x00ffu);assert(s.seq_n==1u&&s.seq[0]=='F');assert(s.nfg==273u);puts("PASS $C04A expiration -> $C172 clear");
}
static void test_missing_dependency_rollback(void){
    wf_runtime_t r,b;wf_r17_post7_ctx_t c;sink_t s;init(&r,&c,&s);r.g.global_16ca=0u;wf_set_u8(&r.actors[0],WF_A_ACTIVE,0x80u);wf_set_u8(&r.actors[0],WF_A_STAMINA+1u,0x20u);c.raw_9aca=0;b=r;
    assert(wf_r17_owner_c04a(&r,&c)==WF_BLOCKED);assert(c.blocked_dependency_rom==0x00009acau);assert(memcmp(&r.m68k_regs,&b.m68k_regs,sizeof(r.m68k_regs))==0);assert(r.g.global_167b==b.g.global_167b&&r.g.global_16ca==b.g.global_16ca&&r.g.global_16cc==b.g.global_16cc);assert(s.seq_n==0u);puts("PASS $C04A fail-closed preflight rollback");
}
int main(void){test_gate_16c5();test_credit_carry();test_start_warning_full_sequence();test_existing_warning();test_expire();test_missing_dependency_rollback();return 0;}
