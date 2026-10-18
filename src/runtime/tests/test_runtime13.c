#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "wf_cpu_runtime.h"
#include "wf_collision_dispatch.h"
#include "wf_payment.h"
#include "wf_gameplay_bindings.h"

static void prep_state(wf_actor_raw_t *a,uint16_t st){memset(a,0,sizeof(*a));wf_set_be16(a,WF_A_STATE,st);wf_set_u8(a,WF_A_ACTIVE,0x80u);}

int main(void){
 wf_runtime_t rt; wf_cpu_trace_t tr; memset(&rt,0,sizeof(rt));
 /* CPU FINAL state6 $1EDC4: missing exact $21B4 register context must fail closed at $24CC. */
 prep_state(&rt.actors[0],6u);rt.g.credited_route=1u;rt.g.progression_0162=0u;wf_set_u8(&rt.actors[0],WF_A_TIMER44+1u,1u);wf_set_u8(&rt.actors[0],WF_A_CPU_B5,0x10u);
 assert(wf_cpu_actor_step(&rt,0,&tr)==WF_BLOCKED);assert(tr.blocked_dependency_rom==0x000024ccu);assert((wf_u8(&rt.actors[0],WF_A_CPU_B5)&0x10u)==0u);
 /* With externally supplied exact RNG/register context, row0/tier0 and seed 1 selects index0. */
 memset(&rt,0,sizeof(rt));prep_state(&rt.actors[0],6u);rt.g.credited_route=1u;rt.g.progression_0162=0u;rt.g.rng_state=1u;rt.g.rng_state_valid=1u;rt.g.rng_register_context_valid=1u;wf_set_u8(&rt.actors[0],WF_A_TIMER44+1u,1u);
 assert(wf_cpu_actor_step(&rt,0,&tr)==WF_OK);assert((wf_u8(&rt.actors[0],WF_A_CPU_B5)&0x10u)!=0u);
 /* Later initialized pass consumes old B5.bit4 and commits exact state5/selector3F. */
 wf_set_u8(&rt.actors[0],WF_A_STATE,0x80u);wf_set_u8(&rt.actors[0],WF_A_STATE+1u,6u);wf_set_be16(&rt.actors[0],WF_A_TIMER44,0x1234u);
 assert(wf_cpu_actor_step(&rt,0,&tr)==WF_OK);assert(wf_be16(&rt.actors[0],WF_A_STATE)==5u);assert(wf_be16(&rt.actors[0],WF_A_SELECTOR)==0x003fu);assert(wf_be16(&rt.actors[0],WF_A_TIMER44)==0u);
 /* Seed 45 on same exact context selects index1: clear BC and B7.bit1, no synthetic choice. */
 memset(&rt,0,sizeof(rt));prep_state(&rt.actors[0],6u);rt.g.credited_route=1u;rt.g.progression_0162=0u;rt.g.rng_state=45u;rt.g.rng_state_valid=1u;rt.g.rng_register_context_valid=1u;wf_set_u8(&rt.actors[0],WF_A_TIMER44+1u,1u);wf_set_be16(&rt.actors[0],WF_A_CPU_BC,0x1234u);wf_set_u8(&rt.actors[0],WF_A_CPU_B7,0x02u);
 assert(wf_cpu_actor_step(&rt,0,&tr)==WF_OK);assert(wf_be16(&rt.actors[0],WF_A_CPU_BC)==0u);assert((wf_u8(&rt.actors[0],WF_A_CPU_B7)&0x02u)==0u);
 /* WP01 $1EB48 special controller16 path: angle $40 then wait until X >= $0308; no invented decision tail. */
 memset(&rt,0,sizeof(rt));prep_state(&rt.actors[0],5u);wf_set_be16(&rt.actors[0],WF_A_SELECTOR,0x0016u);wf_set_u8(&rt.actors[0],WF_A_INTERNAL_ID,7u);wf_set_be16(&rt.actors[0],WF_A_X,0x0307u);
 assert(wf_state5_execute(&rt,0,&tr.state5)==WF_OK);assert(wf_u8(&rt.actors[0],WF_A_ANGLE)==0x40u);assert((wf_u8(&rt.actors[0],WF_A_SELECTOR)&0x40u)!=0u);
 wf_set_be16(&rt.actors[0],WF_A_X,0x0308u);assert(wf_state5_execute(&rt,0,&tr.state5)==WF_BLOCKED);assert(tr.state5.blocked_dependency_rom==0x001e9ccu);
 /* Collision SECOND S5 $24A90 exact non-subtype side effects. */
 wf_actor_raw_t s,d,l;memset(&s,0,sizeof(s));memset(&d,0,sizeof(d));memset(&l,0,sizeof(l));wf_set_be16(&d,WF_A_REL_FLAGS,0x55aau);wf_set_be32(&d,WF_A_REL_LINK,0x12345678u);wf_set_be16(&d,WF_A_TIMER44,0x9999u);wf_set_be16(&l,WF_A_LATCHED,0x7777u);wf_state4_write_word(&d,0x8009u);
 wf_collision_pair_ctx_t c={&s,&d,&l,0,0,0};assert(wf_collision_second_apply(0x05u,&c)==WF_COLL_ACCEPT);assert(wf_be16(&d,WF_A_REL_FLAGS)==0u);assert(wf_be32(&d,WF_A_REL_LINK)==0u);assert(wf_be16(&d,WF_A_TIMER44)==0u);assert(wf_be16(&l,WF_A_LATCHED)==2u);assert(wf_be16(&d,WF_A_STATE4)==0x8009u);

 /* WP19 direct gameplay dependency $055A: mode2 fixed-unit and whole-credit branches. */
 memset(&rt,0,sizeof(rt));rt.g.credit_cycle_units_004b=3u;rt.g.credit_count_004f=1u;assert(wf_payment_consume_055a(&rt,2u)==1);assert(rt.g.credit_count_004f==0u&&rt.g.credit_remainder_004e==2u);
 memset(&rt,0,sizeof(rt));rt.g.config_0066=0x0008u;rt.g.credit_count_004f=1u;assert(wf_payment_consume_055a(&rt,2u)==1);assert(rt.g.credit_count_004f==0u);
 memset(&rt,0,sizeof(rt));assert(wf_payment_consume_055a(&rt,6u)==0);
 /* $8B3A successful regain-power path uses exact payment and WORD refill writes. */
 memset(&rt,0,sizeof(rt));wf_gameplay_binding_context_t gc;memset(&gc,0,sizeof(gc));gc.cabinet_words_valid=1u;gc.cabinet_word_140020=0xffffu;gc.cabinet_word_140022=0xffffu;rt.g.mode_0161=1u;rt.g.credit_cycle_units_004b=1u;rt.g.credit_remainder_004e=1u;wf_set_u8(&rt.actors[0],WF_A_INPUT_FRESH,1u);wf_set_u8(&rt.actors[0],WF_A_RESULT_LO,1u);wf_set_be16(&rt.actors[0],WF_A_STAMINA_MAX,100u);wf_set_be16(&rt.actors[0],WF_A_STAMINA,20u);wf_set_be16(&rt.actors[0],WF_A_TIER,2u);wf_set_be16(&rt.actors[0],WF_A_FIELD62,0x1234u);
 assert(wf_phase_refill_8b3a(&rt,0x008b3au,&gc)==WF_OK);assert(wf_be16(&rt.actors[0],WF_A_STAMINA)==100u);assert(wf_be16(&rt.actors[0],WF_A_VISIBLE_DELTA)==100u);assert(wf_be16(&rt.actors[0],WF_A_TIER)==0u);assert(wf_be16(&rt.actors[0],WF_A_MASH)==1u);assert(wf_be16(&rt.actors[0],WF_A_FIELD62)==0u);assert(rt.g.global_169e==0x8000u);assert(rt.g.credit_remainder_004e==0u);
 puts("PASS test_runtime13");return 0;
}
