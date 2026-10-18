#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "wf_r17_post7a00.h"
#include "wf_gameplay_bindings.h"

typedef struct sink {
    unsigned n285;
    uint8_t ids[16];
    unsigned n26;
    uint8_t sides[4];
    uint16_t dirs[4];
    int fail_285;
    int fail_26;
} sink_t;

static wf_status_t cb_285(wf_runtime_t *rt,uint8_t rec[4],uint8_t bank,void *user){
    sink_t *s=(sink_t*)user;
    (void)bank;
    if(s->fail_285)return WF_BLOCKED;
    assert(s->n285<16u);
    s->ids[s->n285++]=(uint8_t)(rec[0]&0x3fu);
    /* $28896 preserves D0/D1 but dynamic handler D2-D7 clobbers are visible. */
    rt->m68k_regs.d[0]=0xdead0000u+s->n285;
    rt->m68k_regs.d[1]=0xbeef0000u+s->n285;
    for(unsigned i=2u;i<8u;i++)rt->m68k_regs.d[i]=0x10000000u+(i<<16)+s->n285;
    return WF_OK;
}

static wf_status_t cb_26(wf_runtime_t *rt,uint8_t side,uint16_t direction,void *user){
    sink_t *s=(sink_t*)user;
    if(s->fail_26)return WF_BLOCKED;
    assert(s->n26<4u);
    s->sides[s->n26]=side;s->dirs[s->n26]=direction;s->n26++;
    /* Combined $26B68 must restore D0-D2 but may leave D3-D7 live. */
    rt->m68k_regs.d[0]=0xa0000000u+s->n26;
    rt->m68k_regs.d[1]=0xa1000000u+s->n26;
    rt->m68k_regs.d[2]=0xa2000000u+s->n26;
    for(unsigned i=3u;i<8u;i++)rt->m68k_regs.d[i]=0xb0000000u+(i<<16)+s->n26;
    return WF_OK;
}

static void init(wf_runtime_t *r,wf_r17_post7_ctx_t *c,sink_t *s){
    memset(r,0,sizeof(*r));memset(c,0,sizeof(*c));memset(s,0,sizeof(*s));
    r->m68k_regs.valid=1u;
    for(unsigned i=0;i<8u;i++)r->m68k_regs.d[i]=0x01000000u+(i<<16)+0x55u;
    c->branchb_288f0=cb_285;c->branchb_26b68=cb_26;c->user=s;
    r->branchb_prev_185a[0]=0xffu;
}

static void test_285da_selection_and_regs(void){
    wf_runtime_t r;wf_r17_post7_ctx_t c;sink_t s;uint32_t d0,d1;
    init(&r,&c,&s);
    r.g.global_007e=4u;r.g.global_007f=2u;r.g.global_17da=20u;r.g.global_17dc=12u;
    d0=r.m68k_regs.d[0];d1=r.m68k_regs.d[1];
    assert(wf_r17_owner_285da(&r,&c)==WF_OK);
    assert(r.branchb_list_182a[0]==3u&&r.branchb_list_182a[1]==7u&&r.branchb_list_182a[2]==11u&&r.branchb_list_182a[3]==0xffu);
    assert(s.n285==3u&&s.ids[0]==3u&&s.ids[1]==7u&&s.ids[2]==11u);
    assert(r.m68k_regs.d[0]==d0&&r.m68k_regs.d[1]==d1);
    for(unsigned i=2u;i<8u;i++)assert(r.m68k_regs.d[i]==0x10000000u+(i<<16)+3u);
    puts("PASS $285DA selection/order + D0/D1 restore + D2-D7 visibility");
}

static void test_285da_rollback(void){
    wf_runtime_t r,before;wf_r17_post7_ctx_t c;sink_t s;
    init(&r,&c,&s);r.g.global_007e=4u;r.g.global_17da=20u;r.g.global_17dc=12u;s.fail_285=1;before=r;
    assert(wf_r17_owner_285da(&r,&c)==WF_BLOCKED);assert(c.blocked_dependency_rom==0x000288f0u);
    assert(memcmp(r.branchb_list_182a,before.branchb_list_182a,sizeof(r.branchb_list_182a))==0);
    assert(memcmp(r.branchb_prev_185a,before.branchb_prev_185a,sizeof(r.branchb_prev_185a))==0);
    assert(memcmp(r.branchb_objs_188a,before.branchb_objs_188a,sizeof(r.branchb_objs_188a))==0);
    assert(memcmp(&r.m68k_regs,&before.m68k_regs,sizeof(r.m68k_regs))==0);
    puts("PASS $285DA fail-closed rollback");
}

static void test_26a42_dirs_and_regs(void){
    wf_runtime_t r;wf_r17_post7_ctx_t c;sink_t s;uint32_t d[8];
    init(&r,&c,&s);
    /* side0: (64>>5)=2 > target X=0 => dir1 => X++.
       side1: (0>>5)=0 < target X=2 => dir3 => X--. */
    r.g.global_17e6=64u;r.g.global_17ee=0u;r.g.global_17da=0u;r.g.global_17dc=4u;
    r.g.global_17ea=0u;r.g.global_17f2_word=0u;r.g.global_17de=2u;r.g.global_17e0=9u;
    for(unsigned i=0;i<8u;i++)d[i]=r.m68k_regs.d[i];
    assert(wf_r17_owner_26a42(&r,&c)==WF_OK);
    assert(s.n26==2u&&s.sides[0]==0u&&s.dirs[0]==1u&&s.sides[1]==1u&&s.dirs[1]==3u);
    assert(r.g.global_17da==1u); /* dir1 side0: X++ */
    assert(r.g.global_17de==1u); /* dir3 side1: X-- */
    assert(r.g.global_16d2==0xc000u);
    for(unsigned i=0;i<3u;i++)assert(r.m68k_regs.d[i]==d[i]);
    for(unsigned i=3u;i<8u;i++)assert(r.m68k_regs.d[i]==0xb0000000u+(i<<16)+2u);
    puts("PASS $26A42 directions + D0-D2 restore + D3-D7 visibility");
}

static void test_26a42_rollback(void){
    wf_runtime_t r,before;wf_r17_post7_ctx_t c;sink_t s;
    init(&r,&c,&s);r.g.global_17e6=64u;r.g.global_17da=0u;r.g.global_17dc=1u;s.fail_26=1;before=r;
    assert(wf_r17_owner_26a42(&r,&c)==WF_BLOCKED);assert(c.blocked_dependency_rom==0x00026b68u);
    assert(memcmp(&r.g,&before.g,sizeof(r.g))==0);assert(memcmp(&r.m68k_regs,&before.m68k_regs,sizeof(r.m68k_regs))==0);
    puts("PASS $26A42 fail-closed rollback");
}

static void test_bindings_present(void){
    wf_gameplay_binding_context_t c;wf_phase_binding_t b[40];size_t n=wf_gameplay_bindings_build(&c,b,40u);int a=0,d=0;
    assert(n>=31u);for(size_t i=0;i<n;i++){if(b[i].rom_addr==0x0285dau)a=1;if(b[i].rom_addr==0x026a42u)d=1;}assert(a&&d);
    puts("PASS branch-B preowner bindings present");
}

int main(void){test_285da_selection_and_regs();test_285da_rollback();test_26a42_dirs_and_regs();test_26a42_rollback();test_bindings_present();return 0;}
