#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "wf_gameplay_bindings.h"
#include "wf_r17_effects_7a00.h"

static wf_r17_render_ctx_t rc(wf_r17_render_queue_t*q,uint16_t sx,uint16_t sy){wf_r17_render_ctx_t r;r.scroll_a_x=sx;r.scroll_a_y=sy;r.queue=q;r.visibility_2188=wf_r17_visibility_2188_exact;r.visibility_user=0;return r;}
static void rng_ready(wf_runtime_t*rt,uint32_t seed){rt->g.rng_state_valid=1u;rt->g.rng_state=seed;rt->m68k_regs.valid=1u;for(unsigned i=0;i<8u;i++)rt->m68k_regs.d[i]=0x11110000u+(uint32_t)i;}

static void test_init_delay_and_state0(void){
 wf_runtime_t rt;wf_r17_render_queue_t q;wf_r17_render_ctx_t r;wf_r17_effects_7a00_trace_t t;memset(&rt,0,sizeof(rt));wf_r17_m68k_software_reset(&rt.m68k_regs);rng_ready(&rt,0x12345678u);wf_r17_render_queue_clear(&q);r=rc(&q,0x0140u,0x0200u);
 uint32_t state=rt.g.rng_state;uint32_t d[8];memcpy(d,rt.m68k_regs.d,sizeof(d));d[1]=2u;uint32_t rv=wf_r17_rng21b4_step(&state,0u,d);uint16_t delay=(uint16_t)(rv&0x7fu);uint16_t after=(uint16_t)(delay-1u);
 assert(wf_r17_effects_7a00(&rt,&r,&t)==WF_OK);assert(t.processed_objects==3u&&t.rng_calls==3u);assert((wf_u8(&rt.effect_1450[0],WF_A_ACTIVE)&0x80u)!=0u);assert(wf_be16(&rt.effect_1450[0],WF_A_ANIM_INDEX)==after);assert(wf_be16(&rt.effect_1450[0],WF_A_LATCHED)==(after&0x8000u?1u:0u));puts("PASS $7A00 inactive init + RNG delay + state0 decrement");
}

static void test_nonroyal_state1(void){
 wf_runtime_t rt;wf_r17_render_queue_t q;wf_r17_render_ctx_t r;wf_r17_effects_7a00_trace_t t;memset(&rt,0,sizeof(rt));wf_r17_m68k_software_reset(&rt.m68k_regs);rng_ready(&rt,0x89abcdefu);wf_r17_render_queue_clear(&q);r=rc(&q,0x0140u,0x0200u);rt.g.mode_0161=0u;
 wf_actor_raw_t*a=&rt.effect_1450[0];wf_set_u8(a,WF_A_ACTIVE,0x80u);wf_set_be16(a,WF_A_LATCHED,1u);
 for(unsigned i=1;i<3u;i++){wf_set_u8(&rt.effect_1450[i],WF_A_ACTIVE,0x80u);wf_set_be16(&rt.effect_1450[i],WF_A_LATCHED,0u);wf_set_be16(&rt.effect_1450[i],WF_A_ANIM_INDEX,5u);}
 assert(wf_r17_effects_7a00(&rt,&r,&t)==WF_OK);assert((wf_u8(a,WF_A_LATCHED)&0x80u)!=0u);assert(wf_be16(a,WF_A_CHARACTER)==0x001du);assert(wf_be16(a,WF_A_Z)==0x0140u);assert(wf_be16(a,WF_A_X)==0x0198u);assert(wf_be16(a,WF_A_Y)==0x020bu);assert(wf_be16(a,WF_A_ANIM_INDEX)==1u);assert(wf_be16(a,WF_A_MIRROR)==1u);assert(t.rendered_objects==1u);puts("PASS $7A5C non-Royal first-entry + two RNG positions + phase1 render");
}

static void test_royal_and_terminal(void){
 wf_runtime_t rt;wf_r17_render_queue_t q;wf_r17_render_ctx_t r;wf_r17_effects_7a00_trace_t t;memset(&rt,0,sizeof(rt));wf_r17_m68k_software_reset(&rt.m68k_regs);rng_ready(&rt,0x13579bdfu);wf_r17_render_queue_clear(&q);r=rc(&q,0x0200u,0x0200u);rt.g.mode_0161=2u;
 wf_actor_raw_t*a=&rt.effect_1450[0];wf_set_u8(a,WF_A_ACTIVE,0x80u);wf_set_be16(a,WF_A_LATCHED,1u);for(unsigned i=1;i<3u;i++){wf_set_u8(&rt.effect_1450[i],WF_A_ACTIVE,0x80u);wf_set_be16(&rt.effect_1450[i],WF_A_LATCHED,0u);wf_set_be16(&rt.effect_1450[i],WF_A_ANIM_INDEX,5u);}
 assert(wf_r17_effects_7a00(&rt,&r,&t)==WF_OK);assert(wf_be16(a,WF_A_Z)==0x0100u);assert(wf_be16(a,WF_A_X)==0x027eu);assert(wf_be16(a,WF_A_Y)==0x01d0u);assert(t.rng_calls==2u);
 wf_set_be16(a,WF_A_LATCHED,0x8001u);wf_set_be16(a,WF_A_ANIM_COUNTER,0u);wf_set_be16(a,WF_A_ANIM_INDEX,5u);wf_r17_render_queue_clear(&q);assert(wf_r17_effects_7a00(&rt,&r,&t)==WF_OK);assert(wf_be16(a,WF_A_LATCHED)==0u);assert((wf_u8(a,WF_A_ACTIVE)&0x80u)==0u);puts("PASS $7A5C Royal positioning branch + phase6 deactivate");
}

static void test_fail_closed_rollback(void){
 wf_runtime_t rt,before;wf_r17_render_queue_t q;wf_r17_render_ctx_t r;wf_r17_effects_7a00_trace_t t;memset(&rt,0,sizeof(rt));wf_r17_m68k_software_reset(&rt.m68k_regs);wf_r17_render_queue_clear(&q);r=rc(&q,0x0140u,0x0200u);before=rt;
 assert(wf_r17_effects_7a00(&rt,&r,&t)==WF_BLOCKED);assert(t.blocked_dependency_rom==0x000021b4u);assert(memcmp(&rt.effect_1450,&before.effect_1450,sizeof(rt.effect_1450))==0);assert(rt.g.rng_state==before.g.rng_state);puts("PASS $7A00 missing RNG context fail-closed + rollback");
}

static void test_scheduler_cross(void){
 wf_runtime_t rt;wf_gameplay_binding_context_t c;wf_phase_binding_t b[40];size_t n=wf_gameplay_bindings_build(&c,b,40u);assert(n>=29u);wf_runtime_init(&rt,b,n);c.cabinet_words_valid=1u;c.cabinet_word_140020=c.cabinet_word_140022=0xffffu;c.camera_valid=1u;c.camera.scroll0_x=0x0140;c.camera.scroll1_x=0x0140;c.camera.scroll0_y=0x0200;c.camera.scroll1_y=0x0200;rt.g.mode_0161=1u;rt.g.rng_state_valid=1u;rt.g.rng_state=0x2468ace0u;wf_r17_referee_init_10718(&rt);wf_r17_fixed_four_init_1004c(&rt);
 assert(wf_runtime_frame(&rt)==WF_BLOCKED);assert(rt.trace_count==23u);assert(rt.trace[22]==0x008cf8u);assert(rt.trace[15]==0x010e6au&&rt.trace[16]==0x007a00u&&rt.trace[17]==0x0088a2u);assert(c.last_blocked_phase==0u||c.last_blocked_phase!=0x007a00u);puts("PASS scheduler crosses $7A00 and blocks at $8CF8");
}
int main(void){test_init_delay_and_state0();test_nonroyal_state1();test_royal_and_terminal();test_fail_closed_rollback();test_scheduler_cross();puts("PASS RUNTIME-17 $7A00 effect owner");return 0;}
