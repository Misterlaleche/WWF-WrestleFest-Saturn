#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "wf_gameplay_bindings.h"
#include "wf_r17_resolver_280dc.h"
#include "wf_physics.h"

static void active(wf_actor_raw_t *a){memset(a,0,sizeof(*a));wf_set_u8(a,WF_A_ACTIVE,0x80u);}
static void base_probe(wf_runtime_t *rt,wf_actor_raw_t *a,uint8_t mode,uint8_t side,uint16_t x,uint16_t y,uint16_t z){
 memset(rt,0,sizeof(*rt));active(a);rt->g.global_007e=mode;wf_set_be16(a,WF_A_FLAGS32,side?4u:0u);wf_set_be16(a,WF_A_X,x);wf_set_be16(a,WF_A_Y,y);wf_set_be16(a,WF_A_Z,z);
}
static void test_280dc_early_and_dispatch(void){
 wf_runtime_t rt;wf_actor_raw_t *a=&rt.actors[0];
 base_probe(&rt,a,0u,0u,0x0280u,0x0150u,0x0100u);wf_set_be16(a,WF_A_CORR_X,1u);wf_set_be16(a,WF_A_CORR_Y,2u);wf_set_be16(a,WF_A_CORR_Z,3u);wf_set_u8(a,WF_A_FLAGS32,0x20u);
 assert(wf_r17_resolver_280dc(&rt,a)==WF_OK);assert(wf_be16(a,WF_A_CORR_X)==0u&&wf_be16(a,WF_A_CORR_Y)==0u&&wf_be16(a,WF_A_CORR_Z)==0u);
 base_probe(&rt,a,3u,0u,0x0280u,0x0150u,0x0140u);wf_set_u8(a,WF_A_FACING,0x80u);wf_set_be16(a,WF_A_FIELD3E,0x0010u);assert(wf_r17_resolver_280dc(&rt,a)==WF_OK);assert(wf_be16(a,WF_A_FIELD3E)==0xfff0u);
 base_probe(&rt,a,7u,0u,0x0280u,0x0150u,0x0140u);assert(wf_r17_resolver_280dc(&rt,a)==WF_BLOCKED);
}
static void test_280dc_routes(void){
 wf_runtime_t rt;wf_actor_raw_t *a=&rt.actors[0];
 /* $2818E floor: bit4, correction 0x40, no fabricated contact code. */
 base_probe(&rt,a,0u,0u,0x0280u,0x0150u,0x0100u);assert(wf_r17_resolver_280dc(&rt,a)==WF_OK);assert((wf_u8(a,WF_A_CONTACT+1u)&0x10u)!=0u);assert(wf_be16(a,WF_A_CORR_Z)==0x0040u);assert(wf_u8(a,WF_A_CONTACT)==0u);
 /* $28288: y=0x120 gives DIVS edge 0x0142; x=0x0100 => +0x42, contact 3. */
 base_probe(&rt,a,0u,1u,0x0100u,0x0120u,0x0120u);assert(wf_r17_resolver_280dc(&rt,a)==WF_OK);assert(wf_be16(a,WF_A_CORR_X)==0x0042u);assert((wf_u8(a,WF_A_CONTACT+1u)&0x02u)!=0u);assert(wf_u8(a,WF_A_CONTACT)==3u);
 /* $2831C lower/high actual-Z bands use distinct constants and codes 5/6. */
 base_probe(&rt,a,1u,0u,0x0100u,0x0120u,0x0100u);assert(wf_r17_resolver_280dc(&rt,a)==WF_OK);assert(wf_be16(a,WF_A_CORR_X)==0x00c8u);assert(wf_u8(a,WF_A_CONTACT)==5u);
 base_probe(&rt,a,1u,0u,0x0100u,0x0120u,0x0190u);assert(wf_r17_resolver_280dc(&rt,a)==WF_OK);assert(wf_be16(a,WF_A_CORR_X)==0x009bu);assert(wf_u8(a,WF_A_CONTACT)==6u);
 /* $28480 constant left bound and mandatory low-byte bit2. */
 base_probe(&rt,a,2u,0u,0x0260u,0x0160u,0x0140u);assert(wf_r17_resolver_280dc(&rt,a)==WF_OK);assert(wf_be16(a,WF_A_CORR_X)==0x0010u);assert((wf_u8(a,WF_A_CONTACT+1u)&0x06u)==0x06u);assert(wf_u8(a,WF_A_CONTACT)==1u);
 /* $2851E central band records contact code 4 without inventing correction. */
 base_probe(&rt,a,2u,1u,0x0300u,0x0120u,0x0100u);assert(wf_r17_resolver_280dc(&rt,a)==WF_OK);assert(wf_u8(a,WF_A_CONTACT)==4u);assert(wf_be16(a,WF_A_CORR_X)==0u);
}
static void test_2208_mode2_exact_dead_clamp(void){
 wf_actor_raw_t a;memset(&a,0,sizeof(a));wf_set_u8(&a,WF_A_MOTION_MODE,2u);wf_set_be16(&a,WF_A_VZ,0x2100u);wf_set_be16(&a,WF_A_VZ_DEC,0x0010u);wf_physics_integrate_common(&a);
 /* ROM $22AA..$22BC contains a syntactically present MOVE #$2000 whose BPL/BCC gates make it unreachable for every WORD. */
 assert(wf_be16(&a,WF_A_VZ)==0x20f0u);assert(wf_be32(&a,WF_A_Z)==0x00210000u);
 memset(&a,0,sizeof(a));wf_set_u8(&a,WF_A_MOTION_MODE,2u);wf_set_be16(&a,WF_A_VZ,0xff00u);wf_set_be16(&a,WF_A_VZ_DEC,0x0010u);wf_physics_integrate_common(&a);assert(wf_be16(&a,WF_A_VZ)==0xfef0u);
}
static void prep_phase(wf_runtime_t *rt,wf_gameplay_binding_context_t *c){memset(rt,0,sizeof(*rt));memset(c,0,sizeof(*c));c->camera_valid=1u;c->camera.scroll0_x=0x0140;c->camera.scroll0_y=0x0200;}
static void test_f4c2_bounded_dispatch_and_queue(void){
 wf_runtime_t rt;wf_gameplay_binding_context_t c;wf_actor_raw_t *a;
 prep_phase(&rt,&c);a=&rt.actors[0];active(a);wf_set_be16(a,WF_A_STATE,5u);wf_set_be16(a,WF_A_SELECTOR,0x0072u);wf_set_be16(a,WF_A_X,0x0140u);wf_set_be16(a,WF_A_Y,0x0200u);wf_set_be16(a,WF_A_RENDER_LIST,0u);
 assert(wf_phase_wrestlers_f4c2(&rt,0x00f4c2u,&c)==WF_OK);assert(wf_u8(a,WF_A_LATCHED+1u)==5u);assert(c.render_queue.count[0]==1u&&c.render_queue.list[0][0]==a);
 prep_phase(&rt,&c);a=&rt.actors[0];active(a);wf_set_be16(a,WF_A_STATE,5u);wf_set_be16(a,WF_A_SELECTOR,0x0001u);assert(wf_phase_wrestlers_f4c2(&rt,0x00f4c2u,&c)==WF_BLOCKED);assert(c.last_blocked_phase==0x00f4c2u&&c.last_blocked_dependency==0x00012614u);
 /* State-4 subtypes $1F/$28 have callback $12612 RTS and are therefore executable without invented state logic. */
 prep_phase(&rt,&c);a=&rt.actors[0];active(a);wf_set_be16(a,WF_A_STATE,4u);wf_set_be16(a,WF_A_STATE4,0x001fu);wf_set_be16(a,WF_A_X,0x0140u);wf_set_be16(a,WF_A_Y,0x0200u);assert(wf_phase_wrestlers_f4c2(&rt,0x00f4c2u,&c)==WF_OK);
}
static void test_scheduler_advances_to_collision(void){
 wf_runtime_t rt;wf_gameplay_binding_context_t c;wf_phase_binding_t b[40];size_t n=wf_gameplay_bindings_build(&c,b,40u);assert(n>=29u);wf_runtime_init(&rt,b,n);rt.g.rng_state_valid=1u;rt.g.rng_state=0x13579bdfu;c.cabinet_words_valid=1u;c.cabinet_word_140020=0xffffu;c.cabinet_word_140022=0xffffu;c.camera_valid=1u;c.camera.scene_index=0u;c.camera.scroll0_x=0x0140;c.camera.scroll1_x=0x0140;c.camera.scroll0_y=0x0200;c.camera.scroll1_y=0x0200;rt.g.mode_0161=1u;wf_r17_referee_init_10718(&rt);wf_r17_fixed_four_init_1004c(&rt);
 assert(wf_runtime_frame(&rt)==WF_BLOCKED);assert(rt.trace_count==23u);assert(rt.trace[22]==0x008cf8u);assert(rt.trace[9]==0x01f914u&&rt.trace[10]==0x00f4c2u&&rt.trace[11]==0x024062u&&rt.trace[12]==0x024e58u&&rt.trace[13]==0x00fdeeu&&rt.trace[14]==0x010120u&&rt.trace[15]==0x010e6au);
}
static void test_f4c2_shadow(void){
 wf_runtime_t rt;wf_gameplay_binding_context_t c;wf_actor_raw_t *a;
 prep_phase(&rt,&c);rt.m68k_regs.d[0]=0x12345678u;rt.m68k_regs.d[1]=0x98765432u;
 assert(wf_phase_wrestlers_f4c2(&rt,0xf4c2u,&c)==WF_OK);
 assert(rt.m68k_regs.d[0]==0x12340009u&&rt.m68k_regs.d[1]==0x98765432u);
 prep_phase(&rt,&c);a=&rt.actors[0];active(a);rt.m68k_regs.d[0]=0x12345678u;rt.m68k_regs.d[1]=0x98765432u;
 wf_set_u8(a,WF_A_MOTION_MODE,1u);wf_set_be16(a,WF_A_STATE,4u);wf_set_be16(a,WF_A_STATE4,0x1fu);
 wf_set_be16(a,WF_A_X,0x280u);wf_set_be16(a,WF_A_Y,0x150u);wf_set_be16(a,WF_A_Z,0x100u);
 assert(wf_phase_wrestlers_f4c2(&rt,0xf4c2u,&c)==WF_OK);
 assert(rt.m68k_regs.d[0]==0x12340009u&&rt.m68k_regs.d[1]==0x98760040u);
 puts("PASS $F4C2 iterator and correction D-register composition");
}
int main(void){test_f4c2_shadow();test_280dc_early_and_dispatch();test_280dc_routes();test_2208_mode2_exact_dead_clamp();test_f4c2_bounded_dispatch_and_queue();test_scheduler_advances_to_collision();puts("PASS test_runtime17_f4c2");return 0;}
