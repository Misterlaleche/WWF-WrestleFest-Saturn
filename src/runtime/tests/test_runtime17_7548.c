#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "wf_r17_post7a00.h"

typedef struct sink { wf_r17_fg0_write_t w[256]; size_t n; int fail; } sink_t;
static wf_status_t fg(wf_runtime_t *rt,const wf_r17_fg0_write_t *w,size_t n,void *u){
    sink_t*s=(sink_t*)u;if(s->fail)return WF_BLOCKED;assert(n<=256u);memcpy(s->w,w,n*sizeof(*w));s->n=n;
    for(unsigned i=0;i<8u;i++)rt->m68k_regs.d[i]=0xf0000000u+i; /* external join must not leak */
    return WF_OK;
}
static void init(wf_runtime_t*r,wf_r17_post7_ctx_t*c,sink_t*s){
    memset(r,0,sizeof(*r));memset(c,0,sizeof(*c));memset(s,0,sizeof(*s));r->m68k_regs.valid=1u;
    for(unsigned i=0;i<8u;i++)r->m68k_regs.d[i]=0x12000000u+(i<<16)+0x55u;
    r->g.credited_route=1u;c->fg0_batch=fg;c->user=s;
}
static void active0(wf_runtime_t*r){
    wf_set_u8(&r->actors[0],WF_A_ACTIVE,0x80u);wf_set_u8(&r->actors[0],WF_A_ROLE,0u);
    wf_set_be32(&r->actors[0],WF_A_INPUT_SOURCE,0x00140020u);wf_set_be16(&r->actors[0],WF_A_STAMINA_MAX,100u);
}
static void test_positive_and_fg0(void){
    wf_runtime_t r;wf_r17_post7_ctx_t c;sink_t s;uint32_t d4,d5,d6,d7;
    init(&r,&c,&s);active0(&r);wf_set_be16(&r.actors[0],WF_A_VISIBLE_DELTA,2u);wf_set_be16(&r.actors[0],0x6cu,0u);wf_set_be16(&r.actors[0],0x6eu,10u);
    d4=r.m68k_regs.d[4];d5=r.m68k_regs.d[5];d6=r.m68k_regs.d[6];d7=r.m68k_regs.d[7];
    assert(wf_r17_owner_7548(&r,&c)==WF_OK);assert(wf_be16(&r.actors[0],WF_A_VISIBLE_DELTA)==0u);assert(wf_be16(&r.actors[0],0x6cu)==0u);assert(wf_be16(&r.actors[0],0x6eu)==12u);
    assert(s.n==34u);
    assert(s.w[0].addr==0x000c0105u&&s.w[0].value==0x0fu&&s.w[0].width==1u);
    assert(s.w[1].addr==0x000c0107u&&s.w[1].value==0x1bu);
    assert(s.w[6].addr==0x000c0311u&&s.w[6].value==0x33u);
    assert(s.w[7].addr==0x000c0313u&&s.w[7].value==0x1bu);
    /* $7624/$7628 reload the bar origin after clearing its six cells. */
    assert(s.w[30].addr==0x000c0311u&&s.w[30].value==0xdfu);
    assert(s.w[31].addr==0x000c0313u&&s.w[31].value==0x1au);
    assert(s.w[32].addr==0x000c0411u&&s.w[32].value==0xeau);
    assert(s.w[33].addr==0x000c0413u&&s.w[33].value==0x1au);
    assert(r.m68k_regs.d[0]==4u);assert(r.m68k_regs.d[1]==6u);assert(r.m68k_regs.d[2]==6u);assert(r.m68k_regs.d[3]==1u);assert(r.m68k_regs.d[4]==d4);assert(r.m68k_regs.d[5]==d5&&r.m68k_regs.d[6]==d6&&r.m68k_regs.d[7]==d7);
    puts("PASS $7548 positive smoothing + exact FG0 bytes + register shadow");
}
static void test_negative_one(void){
    wf_runtime_t r;wf_r17_post7_ctx_t c;sink_t s;init(&r,&c,&s);active0(&r);wf_set_be16(&r.actors[0],WF_A_VISIBLE_DELTA,0xffffu);wf_set_be16(&r.actors[0],0x6cu,0u);wf_set_be16(&r.actors[0],0x6eu,10u);
    assert(wf_r17_owner_7548(&r,&c)==WF_OK);assert(wf_be16(&r.actors[0],0x6cu)==0u);assert(wf_be16(&r.actors[0],0x6eu)==9u);assert(r.m68k_regs.d[1]==4u&&r.m68k_regs.d[2]==4u&&r.m68k_regs.d[3]==1u);puts("PASS $7548 -1 path");
}
static void test_clamp_max(void){
    wf_runtime_t r;wf_r17_post7_ctx_t c;sink_t s;init(&r,&c,&s);active0(&r);wf_set_be16(&r.actors[0],WF_A_VISIBLE_DELTA,1u);wf_set_be16(&r.actors[0],0x6eu,100u);
    assert(wf_r17_owner_7548(&r,&c)==WF_OK);assert(wf_be16(&r.actors[0],0x6eu)==100u);assert(wf_be16(&r.actors[0],0x6cu)==0u);puts("PASS $7548 equality/max clamp clears accumulator");
}
static void test_low_byte_gate(void){
    wf_runtime_t r;wf_r17_post7_ctx_t c;sink_t s;init(&r,&c,&s);active0(&r);r.g.global_16c4=0x0100u; /* $16C5 byte is still zero */
    assert(wf_r17_owner_7548(&r,&c)==WF_OK);assert(s.n==6u);r.g.global_16c4=0x0001u;s.n=0u;assert(wf_r17_owner_7548(&r,&c)==WF_OK);assert(s.n==0u);puts("PASS $7548 TST.B $16C5 low-byte gate");
}
static void test_rollback_bad_input_and_batch(void){
    wf_runtime_t r,b;wf_r17_post7_ctx_t c;sink_t s;init(&r,&c,&s);active0(&r);wf_set_be32(&r.actors[0],WF_A_INPUT_SOURCE,0x00140021u);b=r;assert(wf_r17_owner_7548(&r,&c)==WF_BLOCKED);assert(c.blocked_dependency_rom==0x000077d0u);assert(memcmp(&r.actors,&b.actors,sizeof(r.actors))==0&&memcmp(&r.m68k_regs,&b.m68k_regs,sizeof(r.m68k_regs))==0);
    init(&r,&c,&s);active0(&r);wf_set_be16(&r.actors[0],WF_A_VISIBLE_DELTA,2u);wf_set_be16(&r.actors[0],0x6eu,10u);s.fail=1;b=r;assert(wf_r17_owner_7548(&r,&c)==WF_BLOCKED);assert(memcmp(&r.actors,&b.actors,sizeof(r.actors))==0&&memcmp(&r.m68k_regs,&b.m68k_regs,sizeof(r.m68k_regs))==0);puts("PASS $7548 fail-closed rollback");
}
int main(void){test_positive_and_fg0();test_negative_one();test_clamp_max();test_low_byte_gate();test_rollback_bad_input_and_batch();return 0;}
