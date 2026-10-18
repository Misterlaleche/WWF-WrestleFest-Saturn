#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "wf_r17_post7a00.h"

static void init_rt(wf_runtime_t *rt){
    memset(rt,0,sizeof(*rt));
    wf_r17_m68k_software_reset(&rt->m68k_regs);
    rt->m68k_regs.d[0]=0xA5A50011u;
    rt->m68k_regs.d[1]=0x5A5A0022u;
}
static void make_active(wf_actor_raw_t *a,uint8_t state1d,uint16_t live,uint16_t max,uint16_t c){
    wf_set_u8(a,WF_A_ACTIVE,0x80u);
    wf_set_u8(a,WF_A_LATCHED+1u,state1d);
    wf_set_be16(a,WF_A_STAMINA,live);
    wf_set_be16(a,WF_A_STAMINA_MAX,max);
    wf_set_be16(a,WF_A_RECOVERY_COUNTER,c);
}
static void test_nonroyal_transparent(void){
    wf_runtime_t rt; init_rt(&rt); make_active(&rt.actors[0],0u,10u,20u,127u);
    assert(wf_r17_owner_8ecc(&rt)==WF_OK);
    assert(wf_be16(&rt.actors[0],WF_A_STAMINA)==10u);
    assert(wf_be16(&rt.actors[0],WF_A_RECOVERY_COUNTER)==127u);
    assert(rt.m68k_regs.d[0]==0xA5A50011u&&rt.m68k_regs.d[1]==0x5A5A0022u);
}
static void test_threshold_and_tier_stale(void){
    wf_runtime_t rt; init_rt(&rt); rt.g.mode_0161=1u;
    make_active(&rt.actors[0],0u,23u,100u,127u);
    wf_set_be16(&rt.actors[0],WF_A_VISIBLE_DELTA,7u);
    wf_set_be16(&rt.actors[0],WF_A_TIER,2u);
    assert(wf_r17_owner_8ecc(&rt)==WF_OK);
    assert(wf_be16(&rt.actors[0],WF_A_RECOVERY_COUNTER)==0u);
    assert(wf_be16(&rt.actors[0],WF_A_STAMINA)==24u);
    assert(wf_be16(&rt.actors[0],WF_A_VISIBLE_DELTA)==8u);
    assert(wf_be16(&rt.actors[0],WF_A_TIER)==2u);
    assert(rt.m68k_regs.d[0]==0xA5A5FFFFu);
    assert(rt.m68k_regs.d[1]==0x5A5A0017u);
}
static void test_pause_gates(void){
    wf_runtime_t rt; init_rt(&rt); rt.g.mode_0161=1u;
    make_active(&rt.actors[0],0x0bu,10u,20u,127u);
    make_active(&rt.actors[1],0x0cu,10u,20u,126u);
    make_active(&rt.actors[2],0x05u,10u,20u,125u); wf_set_u8(&rt.actors[2],WF_A_SELECTOR_LO,0x4au);
    make_active(&rt.actors[3],0u,10u,10u,124u);
    assert(wf_r17_owner_8ecc(&rt)==WF_OK);
    assert(wf_be16(&rt.actors[0],WF_A_RECOVERY_COUNTER)==127u);
    assert(wf_be16(&rt.actors[1],WF_A_RECOVERY_COUNTER)==126u);
    assert(wf_be16(&rt.actors[2],WF_A_RECOVERY_COUNTER)==125u);
    assert(wf_be16(&rt.actors[3],WF_A_RECOVERY_COUNTER)==124u);
    /* Only slot3 reached MOVE.W +$66,D1 despite exact-full exclusion. */
    assert(rt.m68k_regs.d[1]==0x5A5A000Au);
}
static void test_ff_link_gate_and_exact_four(void){
    wf_runtime_t rt; init_rt(&rt); rt.g.mode_0161=1u;
    make_active(&rt.actors[0],0xffu,10u,20u,127u);
    make_active(&rt.actors[1],0x0cu,10u,20u,0u);
    wf_set_be32(&rt.actors[0],WF_A_LINK,wf_actor_rom_base(1u));
    make_active(&rt.actors[4],0u,10u,20u,127u);
    assert(wf_r17_owner_8ecc(&rt)==WF_OK);
    assert(wf_be16(&rt.actors[0],WF_A_RECOVERY_COUNTER)==127u);
    assert(wf_be16(&rt.actors[4],WF_A_STAMINA)==10u);
    assert(wf_be16(&rt.actors[4],WF_A_RECOVERY_COUNTER)==127u);
}
static void test_inequality_not_less_than(void){
    wf_runtime_t rt; init_rt(&rt); rt.g.mode_0161=1u;
    make_active(&rt.actors[0],0u,101u,100u,127u);
    assert(wf_r17_owner_8ecc(&rt)==WF_OK);
    assert(wf_be16(&rt.actors[0],WF_A_STAMINA)==102u);
    assert(wf_be16(&rt.actors[0],WF_A_RECOVERY_COUNTER)==0u);
}
static void test_missing_ff_link_blocks(void){
    wf_runtime_t rt,before; init_rt(&rt); rt.g.mode_0161=1u;
    make_active(&rt.actors[0],0u,10u,20u,127u);
    make_active(&rt.actors[1],0xffu,10u,20u,127u);
    before=rt;
    assert(wf_r17_owner_8ecc(&rt)==WF_BLOCKED);
    /* Full-owner transaction: an unresolved later linked actor must roll back
       both earlier recovery mutations and caller-visible register writes. */
    assert(memcmp(&rt.actors[0],&before.actors[0],sizeof(rt.actors[0])*4u)==0);
    assert(memcmp(&rt.m68k_regs,&before.m68k_regs,sizeof(rt.m68k_regs))==0);
}
int main(void){
    test_nonroyal_transparent();
    test_threshold_and_tier_stale();
    test_pause_gates();
    test_ff_link_gate_and_exact_four();
    test_inequality_not_less_than();
    test_missing_ff_link_blocks();
    puts("PASS $8ECC Royal recovery ROM contract");
    return 0;
}
