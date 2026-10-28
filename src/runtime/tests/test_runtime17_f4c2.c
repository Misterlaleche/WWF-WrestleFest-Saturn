#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "wf_gameplay_bindings.h"
#include "wf_r17_resolver_280dc.h"
#include "wf_physics.h"
#include "wf_r17_dispatch_1c03e.h"
#include "wf_r17_actions_r11.h"
#include "wf_cpu_runtime.h"
#include "wf_r18_prematch.h"

static void active(wf_actor_raw_t *a){memset(a,0,sizeof(*a));wf_set_u8(a,WF_A_ACTIVE,0x80u);}
static wf_status_t audio_ok(wf_runtime_t *rt,uint16_t command,void *user){(void)rt;(void)command;(void)user;return WF_OK;}
static wf_status_t raw_audio_ok(uint16_t command,void *user){(void)command;(void)user;return WF_OK;}
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
 /* $28480 sets bit2 only when Y needs correction; Y==0160 preserves it. */
 base_probe(&rt,a,2u,0u,0x0260u,0x0160u,0x0140u);assert(wf_r17_resolver_280dc(&rt,a)==WF_OK);assert(wf_be16(a,WF_A_CORR_X)==0x0010u);assert((wf_u8(a,WF_A_CONTACT+1u)&0x06u)==0x02u);assert(wf_u8(a,WF_A_CONTACT)==1u);
 base_probe(&rt,a,2u,0u,0x0260u,0x0160u,0x0140u);wf_set_u8(a,WF_A_CONTACT+1u,4u);assert(wf_r17_resolver_280dc(&rt,a)==WF_OK);assert((wf_u8(a,WF_A_CONTACT+1u)&0x06u)==0x06u);
 base_probe(&rt,a,2u,0u,0x0260u,0x015fu,0x0140u);assert(wf_r17_resolver_280dc(&rt,a)==WF_OK);assert(wf_be16(a,WF_A_CORR_Y)==1u);assert((wf_u8(a,WF_A_CONTACT+1u)&0x06u)==0x06u);
 /* $2851E tests central contact only in the Y > $140 correction branch. */
 base_probe(&rt,a,2u,1u,0x0300u,0x0120u,0x0100u);assert(wf_r17_resolver_280dc(&rt,a)==WF_OK);assert(wf_u8(a,WF_A_CONTACT)==0u);assert(wf_be16(a,WF_A_CORR_X)==0u);
 base_probe(&rt,a,2u,1u,0x0300u,0x0141u,0x0100u);assert(wf_r17_resolver_280dc(&rt,a)==WF_OK);assert(wf_u8(a,WF_A_CONTACT)==4u);assert(wf_be16(a,WF_A_CORR_Y)==0xffffu);
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
 /* $35 is now implemented. An out-of-table selector still fails closed. */
 prep_phase(&rt,&c);a=&rt.actors[0];active(a);wf_set_be16(a,WF_A_STATE,5u);wf_set_be16(a,WF_A_SELECTOR,0x008fu);assert(wf_phase_wrestlers_f4c2(&rt,0x00f4c2u,&c)==WF_BLOCKED);assert(c.last_blocked_phase==0x00f4c2u&&c.last_blocked_dependency==0x00012614u);
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
static void test_11b5a_contact_transition(void){
 wf_runtime_t rt;wf_actor_raw_t *a=&rt.actors[0];wf_r17_dispatch_1c03e_ctx_t ctx={0};
 memset(&rt,0,sizeof(rt));active(a);wf_set_be16(a,WF_A_STATE,1u);wf_set_be16(a,WF_A_AE,4u);wf_set_u8(a,WF_A_LATCHED,0x80u);wf_set_u8(a,WF_A_LATCHED+1u,1u);
 wf_set_u8(a,WF_A_FLAGS33,0x80u);wf_set_u8(a,WF_A_CONTACT+1u,1u);
 assert(wf_r17_dispatch_1c03e_bounded(&rt,a,&ctx)==WF_OK);
 assert(wf_be16(a,WF_A_STATE)==5u&&wf_be16(a,WF_A_SELECTOR)==0x4fu);
}
static void test_18b0e_action_4f(void){
 wf_runtime_t rt;wf_actor_raw_t *a=&rt.actors[0];wf_r17_actions_r11_bridge_t bridge={0};
 memset(&rt,0,sizeof(rt));active(a);wf_set_be16(a,WF_A_STATE,5u);wf_set_be16(a,WF_A_SELECTOR,0x4fu);wf_set_u8(a,WF_A_LATCHED+1u,5u);wf_set_u8(a,WF_A_FLAGS33,0x80u);
 assert(wf_r17_dispatch_1c03e_r11(&rt,0u,&bridge)==WF_OK);
 assert(bridge.trace[0].callback_rom==0x18b0eu&&bridge.trace[0].descriptor_rom==0x18afeu);
 assert(wf_u8(a,WF_A_MOTION_MODE)==0u&&wf_be16(a,WF_A_RENDER_LIST)==2u);
 assert(wf_be16(a,WF_A_FACING)==0x8000u&&wf_be16(a,WF_A_ANIM_COUNTER)==16u);
 wf_set_be16(a,WF_A_ANIM_INDEX,0x00feu);wf_set_u8(a,WF_A_FLAGS33,0x80u);wf_set_be16(a,WF_A_X,0x0200u);
 assert(wf_r17_dispatch_1c03e_r11(&rt,0u,&bridge)==WF_OK);
 assert(wf_be16(a,WF_A_STATE)==1u&&wf_be16(a,WF_A_AE)==1u);
 assert(wf_be16(a,WF_A_X)==0x0238u&&wf_be16(a,WF_A_RENDER_LIST)==4u);
}
static void test_1cad6_latched_target_idle(void){
 wf_runtime_t rt;wf_cpu_trace_t trace;wf_actor_raw_t *a=&rt.actors[0],*t=&rt.actors[1];
 memset(&rt,0,sizeof(rt));active(a);active(t);
 wf_set_be16(a,WF_A_STATE,1u);wf_set_be32(a,WF_A_TARGET,wf_actor_rom_base(1u));
 wf_set_be16(a,WF_A_X,0x0200u);wf_set_be16(t,WF_A_X,0x0240u);
 wf_set_u8(a,WF_A_CPU_B9,2u);
 assert(wf_cpu_actor_step(&rt,0u,&trace)==WF_OK);
 assert(wf_u8(a,WF_A_ANGLE)==0xc0u&&wf_be16(a,WF_A_STATE)==1u);
}
static void test_1d398_uninitialized_state_returns(void){
 wf_runtime_t rt;wf_cpu_trace_t trace;memset(&rt,0,sizeof(rt));
 wf_actor_raw_t *a=&rt.actors[0];active(a);wf_set_be16(a,WF_A_STATE,1u);wf_set_u8(a,WF_A_CPU_SUBSTATE,1u);
 assert(wf_cpu_actor_step(&rt,0u,&trace)==WF_OK);
 assert(wf_be16(a,WF_A_STATE)==1u&&trace.handler_rom==0x1c8f4u);
}
static void test_1d3a2_partner_pursuit_far_target(void){
 wf_runtime_t rt;wf_cpu_trace_t trace;memset(&rt,0,sizeof(rt));
 wf_actor_raw_t *a=&rt.actors[5],*partner=&rt.actors[4],*target=&rt.actors[0];
 active(a);active(partner);active(target);rt.g.credited_route=1u;
 wf_set_be16(a,WF_A_STATE,0x8001u);wf_set_u8(a,WF_A_CPU_SUBSTATE,1u);
 wf_set_be32(a,WF_A_TEAM_LINK,wf_actor_rom_base(4u));
 wf_set_be32(partner,WF_A_TARGET,wf_actor_rom_base(0u));
 wf_set_be16(a,WF_A_X,826u);wf_set_be16(a,WF_A_Y,352u);
 wf_set_be16(partner,WF_A_X,658u);wf_set_be16(partner,WF_A_Y,341u);
 wf_set_be16(target,WF_A_X,400u);wf_set_be16(target,WF_A_Y,341u);
 wf_set_u8(a,WF_A_FLAGS33,0x80u);wf_set_u8(a,WF_A_CPU_B7,2u);
 assert(wf_cpu_actor_step(&rt,5u,&trace)==WF_OK);
 assert(wf_u8(a,WF_A_ANGLE)==0x78u);
 assert((wf_u8(a,WF_A_CPU_B7)&2u)==0u&&wf_be16(a,WF_A_STATE)==0x8001u);
 wf_set_be16(partner,WF_A_Y,349u);wf_set_u8(a,WF_A_CPU_B7,2u);
 assert(wf_cpu_actor_step(&rt,5u,&trace)==WF_OK);
 assert(wf_u8(a,WF_A_ANGLE)==0u);
 assert((wf_u8(a,WF_A_CPU_B7)&2u)==0u);
}
static void test_11796_state_one_recovery(void){
 wf_runtime_t rt;wf_actor_raw_t *a=&rt.actors[0];wf_r17_dispatch_1c03e_ctx_t ctx={0};
 memset(&rt,0,sizeof(rt));active(a);wf_set_be16(a,WF_A_STATE,1u);wf_set_be16(a,WF_A_AE,1u);
 wf_set_u8(a,WF_A_LATCHED+1u,1u);wf_set_u8(a,WF_A_FLAGS33,0x80u);
 wf_set_be16(a,WF_A_Y,0x0150u);wf_set_be16(a,WF_A_CHARACTER,3u);
 assert(wf_r17_dispatch_1c03e_bounded(&rt,a,&ctx)==WF_OK);
 assert(wf_be16(a,WF_A_X)==0x033eu&&wf_be16(a,WF_A_RENDER_LIST)==4u);
 assert(wf_be16(a,WF_A_MAGNITUDE)==16u&&wf_be16(a,0x44u)==148u);
 assert(wf_be16(a,WF_A_ANIM_COUNTER)==10u&&wf_be16(a,WF_A_MIRROR)==1u);
 assert((wf_u8(a,WF_A_LATCHED)&0x80u)!=0u);
 wf_set_be16(a,0x44u,1u);wf_set_be16(a,0x66u,30u);wf_set_be16(a,0x72u,40u);
 assert(wf_r17_dispatch_1c03e_bounded(&rt,a,&ctx)==WF_OK);
 assert(wf_be16(a,0x44u)==148u&&wf_be16(a,0x66u)==31u);
 assert(wf_be16(a,0x6au)==1u&&wf_be16(a,WF_A_ANIM_COUNTER)==9u);
 assert(wf_be16(a,WF_A_STATE)==1u);
}
static void test_1c150_skips_human_actor(void){
 wf_runtime_t rt;wf_gameplay_binding_context_t c;memset(&rt,0,sizeof(rt));memset(&c,0,sizeof(c));
 active(&rt.actors[0]);wf_set_be16(&rt.actors[0],WF_A_STATE,1u);
 wf_set_be16(&rt.actors[0],WF_A_ROLE,0u);
 assert(wf_phase_cpu_real(&rt,0x1c150u,&c)==WF_OK);
 assert(c.cpu_trace[0].handler_rom==0u);
}
static void test_1cffa_defender_return_to_idle(void){
 wf_runtime_t rt;wf_cpu_trace_t trace;wf_actor_raw_t *a,*target,*partner;
 memset(&rt,0,sizeof(rt));memset(&trace,0,sizeof(trace));
 a=&rt.actors[1];target=&rt.actors[4];partner=&rt.actors[0];
 active(a);active(target);active(partner);
 wf_set_be16(a,WF_A_STATE,0x8001u);
 wf_set_u8(a,WF_A_FLAGS33,2u);wf_set_u8(a,WF_A_FLAGS34,4u);
 wf_set_u8(a,WF_A_ROLE,0x40u);
 wf_set_u8(a,WF_A_PINFLAGS,0x20u);
 wf_set_be32(a,WF_A_TARGET,wf_actor_rom_base(4u));
 wf_set_be32(a,WF_A_TEAM_LINK,wf_actor_rom_base(0u));
 wf_set_u8(target,WF_A_FLAGS33,1u);
 assert(wf_cpu_actor_step(&rt,1u,&trace)==WF_OK);
 assert(trace.blocked_dependency_rom==0u);
 assert(wf_be16(a,WF_A_STATE)==0u);
 assert(wf_u8(a,WF_A_FLAGS34)==0u);
 assert(wf_u8(a,WF_A_ROLE)==0u);
 assert(wf_u8(a,WF_A_PINFLAGS)==0x28u);
 wf_set_be16(a,WF_A_STATE,0x8001u);
 wf_set_u8(a,WF_A_FLAGS34,4u);wf_set_u8(a,WF_A_ROLE,0x40u);
 wf_set_u8(a,WF_A_PINFLAGS,0x20u);
 wf_set_u8(partner,WF_A_FLAGS33,2u);
 memset(&trace,0,sizeof(trace));
 assert(wf_cpu_actor_step(&rt,1u,&trace)==WF_OK);
 assert(wf_u8(a,WF_A_PINFLAGS)==0x20u);
}
static void test_1f5da_royal_defends_selector_4a(void){
 wf_runtime_t rt;wf_cpu_trace_t trace;wf_actor_raw_t *a,*target;
 memset(&rt,0,sizeof(rt));memset(&trace,0,sizeof(trace));
 a=&rt.actors[0];target=&rt.actors[4];active(a);active(target);
 rt.g.mode_0161=1u;
 wf_set_be16(a,WF_A_STATE,0x8001u);
 wf_set_u8(a,WF_A_CPU_B5,0x80u);
 wf_set_be32(a,WF_A_TARGET,wf_actor_rom_base(4u));
 wf_set_be16(a,WF_A_X,0x0200u);wf_set_be16(a,WF_A_Y,0x0150u);
 wf_set_be16(target,WF_A_STATE,0x8005u);
 wf_set_be16(target,WF_A_SELECTOR,0x004au);
 wf_set_be16(target,WF_A_X,0x0280u);wf_set_be16(target,WF_A_Y,0x0150u);
 assert(wf_cpu_actor_step(&rt,0u,&trace)==WF_OK);
 assert(trace.blocked_dependency_rom==0u);
 assert(wf_be16(a,WF_A_SELECTOR)==0x0049u);
 assert(wf_be16(a,WF_A_STATE)==1u);
 assert(wf_be16(a,WF_A_AE)==10u);
}
static void test_royal_action49_pin_intervention_initial(void){
 wf_runtime_t rt;wf_r17_actions_r11_bridge_t bridge={0};
 wf_actor_raw_t *a,*held,*attacker;
 memset(&rt,0,sizeof(rt));a=&rt.actors[4];held=&rt.actors[0];attacker=&rt.actors[7];
 active(a);active(held);active(attacker);rt.m68k_regs.valid=1u;
 wf_set_be16(a,WF_A_STATE,0x8005u);wf_set_be16(a,WF_A_SELECTOR,0x4049u);
 wf_set_be16(a,WF_A_LATCHED,5u);wf_set_be32(a,WF_A_LINK,wf_actor_rom_base(0u));
 wf_set_be16(a,WF_A_X,0x0260u);wf_set_be16(a,WF_A_Y,0x0150u);
 wf_set_be16(held,WF_A_STATE,0x8005u);wf_set_be16(held,WF_A_SELECTOR,0x804au);
 wf_set_be32(held,WF_A_LINK,wf_actor_rom_base(7u));
 wf_set_be16(held,WF_A_X,0x0280u);wf_set_be16(held,WF_A_Y,0x0160u);
 wf_set_be16(attacker,WF_A_STATE,0x8005u);wf_set_be16(attacker,WF_A_SELECTOR,0xc048u);
 wf_set_be16(attacker,WF_A_X,0x0280u);wf_set_be16(attacker,WF_A_Y,0x0160u);
 assert(wf_r17_dispatch_1c03e_r11(&rt,4u,&bridge)==WF_OK);
 assert(bridge.trace[4].blocked_dependency_rom==0u);
 assert(bridge.trace[4].callback_rom==0x13444u);
 assert(wf_u8(a,WF_A_MOTION_MODE)==2u);
 assert(wf_be16(a,WF_A_Y)==0x015fu);
}
static void test_royal_action42_pin_victim_initial(void){
 wf_runtime_t rt;wf_r17_actions_r11_bridge_t bridge={0};
 memset(&rt,0,sizeof(rt));wf_actor_raw_t *a=&rt.actors[8],*held=&rt.actors[5];
 active(a);active(held);rt.m68k_regs.valid=1u;
 wf_set_be16(a,WF_A_STATE,0x8005u);wf_set_be16(a,WF_A_SELECTOR,0x42u);
 wf_set_be16(a,WF_A_LATCHED,5u);wf_set_be32(a,WF_A_LINK,wf_actor_rom_base(5u));
 wf_set_be16(held,WF_A_STATE,0x8005u);wf_set_be16(held,WF_A_SELECTOR,0x7bu);
 assert(wf_r17_dispatch_1c03e_r11(&rt,8u,&bridge)==WF_OK);
 assert(bridge.trace[8].callback_rom==0x17194u);
 assert(wf_be16(a,WF_A_SELECTOR)==0x8042u);
 assert(wf_be16(a,WF_A_TIMER44)==3u);
 assert(wf_be16(held,WF_A_STATE)==0xffu);
}
static void test_royal_action42_pin_victim_release(void){
 wf_runtime_t rt;wf_r17_actions_r11_bridge_t bridge={0};
 memset(&rt,0,sizeof(rt));wf_actor_raw_t *a=&rt.actors[8],*held=&rt.actors[5];
 active(a);active(held);rt.m68k_regs.valid=1u;rt.g.mode_0161=1u;
 wf_set_be16(a,WF_A_STATE,0x8005u);wf_set_be16(a,WF_A_SELECTOR,0x8042u);
 wf_set_be16(a,WF_A_LATCHED,0x8005u);wf_set_be16(a,WF_A_ANIM_INDEX,0xfeu);
 wf_set_be32(a,WF_A_LINK,wf_actor_rom_base(5u));
 wf_set_be16(a,WF_A_X,0x0260u);wf_set_be16(a,WF_A_Y,0x0160u);
 wf_set_be16(a,WF_A_Z,0x0140u);
 wf_set_be16(held,WF_A_STATE,0xffu);
 assert(wf_r17_dispatch_1c03e_r11(&rt,8u,&bridge)==WF_OK);
 assert(bridge.trace[8].callback_rom==0x17194u);
 assert(wf_be16(a,WF_A_STATE)==0u);
 assert(wf_be16(held,WF_A_STATE)==4u);
 assert(wf_be16(held,0x64u)==2u);
 assert(wf_be16(held,WF_A_X)==0x0250u);
 assert(wf_be16(held,WF_A_Y)==0x015fu);
}
static void test_royal_action23_pin_partner_route(void){
 wf_runtime_t rt;wf_gameplay_binding_context_t c;prep_phase(&rt,&c);
 wf_actor_raw_t *a=&rt.actors[8],*t=&rt.actors[5];
 active(a);rt.g.mode_0161=1u;rt.g.credited_route=1u;c.audio_command=audio_ok;
 wf_set_be16(a,WF_A_STATE,5u);wf_set_be16(a,WF_A_SELECTOR,0x4023u);
 wf_set_be16(a,WF_A_LATCHED,0x8005u);wf_set_be16(a,0x44u,8u);
 wf_set_be16(a,WF_A_ANIM_INDEX,3u);wf_set_be16(a,WF_A_ANIM_COUNTER,0xff00u);
 wf_set_be32(a,WF_A_LINK,wf_actor_rom_base(5u));
 wf_set_be16(a,WF_A_X,0x025fu);wf_set_be16(a,WF_A_Y,0x0169u);
 wf_set_u8(a,WF_A_FLAGS33,1u);wf_set_u8(a,0x37u,0x10u);
 wf_set_be16(t,WF_A_STATE,4u);wf_set_be16(t,WF_A_STATE4,9u);
 wf_set_be16(t,WF_A_X,0x028fu);wf_set_be16(t,WF_A_Y,0x016au);
 wf_set_be16(t,WF_A_FACING,0x8000u);wf_set_u8(t,WF_A_FLAGS33,1u);
 wf_set_be32(t,WF_A_LINK,wf_actor_rom_base(8u));
 assert(wf_phase_wrestlers_f4c2(&rt,0xf4c2u,&c)==WF_OK);
 assert(c.last_blocked_dependency==0u);
 assert(c.f4c2_raw23_ctx.special_215b6!=NULL);
 wf_set_be16(a,WF_A_STATE,5u);wf_set_be16(a,WF_A_SELECTOR,0x4023u);
 wf_set_be16(a,WF_A_LATCHED,0x8005u);wf_set_be16(a,0x44u,8u);
 wf_set_be16(a,WF_A_ANIM_INDEX,3u);wf_set_be16(a,WF_A_ANIM_COUNTER,0xff00u);
 wf_set_u8(a,0x37u,0x10u);wf_set_be16(t,WF_A_STATE,4u);
 c.f4c2_raw23_ctx.audio_emit=raw_audio_ok;c.f4c2_raw23_ctx.user=NULL;
 wf_r17_raw23_trace_t trace={0};
 assert(wf_r17_dispatch_1c03e_raw23(&rt,8u,&c.f4c2_raw23_ctx,&trace)==WF_OK);
 assert(wf_be16(t,WF_A_STATE)==5u);
 assert(wf_be16(t,WF_A_SELECTOR)==0x64u);
}
static void test_124fa_linked_hold_initial(void){
 wf_runtime_t rt;wf_actor_raw_t *a=&rt.actors[0],*linked=&rt.actors[4];
 wf_r17_dispatch_1c03e_ctx_t ctx={0};memset(&rt,0,sizeof(rt));
 active(a);active(linked);wf_set_be16(a,WF_A_STATE,0x800cu);
 wf_set_u8(a,WF_A_LATCHED+1u,12u);wf_set_be32(a,WF_A_LINK,wf_actor_rom_base(4u));
 wf_set_be16(a,WF_A_X,0x026du);wf_set_be16(a,WF_A_Y,0x0152u);
 wf_set_be16(a,WF_A_Z,0x0140u);wf_set_be16(a,WF_A_RENDER_LIST,0u);
 wf_set_u8(a,WF_A_FLAGS33,0x03u);wf_set_u8(linked,WF_A_FLAGS33,0x81u);
 assert(wf_r17_dispatch_1c03e_bounded(&rt,a,&ctx)==WF_OK);
 assert((wf_u8(a,WF_A_FLAGS33)&0x40u)!=0u);
 assert((wf_u8(linked,WF_A_FLAGS33)&0x40u)!=0u);
 assert(wf_be16(a,WF_A_TIMER46)==0xdfu);
 assert(wf_be16(a,WF_A_ANIM_COUNTER)==16u);
 assert(wf_be16(a,WF_A_MIRROR)==0x39u);
 assert((wf_u8(a,WF_A_LATCHED)&0x80u)!=0u);
}
static void test_14cf4_hold_action_initial(void){
 wf_runtime_t rt;wf_r17_actions_r11_bridge_t bridge={0};
 wf_actor_raw_t *a=&rt.actors[0],*linked=&rt.actors[4];memset(&rt,0,sizeof(rt));
 active(a);active(linked);wf_set_be16(a,WF_A_STATE,5u);
 wf_set_be16(a,WF_A_SELECTOR,0x15u);wf_set_u8(a,WF_A_LATCHED+1u,5u);
 wf_set_be32(a,WF_A_LINK,wf_actor_rom_base(4u));wf_set_be16(a,WF_A_TIMER44,1u);
 wf_set_be16(linked,WF_A_STATE,0xffu);
 assert(wf_r17_dispatch_1c03e_r11(&rt,0u,&bridge)==WF_OK);
 assert(bridge.trace[0].callback_rom==0x14cf4u&&bridge.trace[0].descriptor_rom==0x14ce0u);
 assert(wf_be16(linked,WF_A_STATE)==5u&&wf_be16(linked,WF_A_SELECTOR)==0x7bu);
 assert(wf_be16(a,WF_A_ANIM_INDEX)==1u&&wf_be16(a,WF_A_ANIM_COUNTER)==14u);
 assert(wf_be16(a,WF_A_MIRROR)==0xf9u);
}
static void test_1a538_held_opponent(void){
 wf_runtime_t rt;wf_r17_actions_r11_bridge_t bridge={0};
 wf_actor_raw_t *a=&rt.actors[4],*linked=&rt.actors[0];memset(&rt,0,sizeof(rt));
 active(a);active(linked);wf_set_be16(a,WF_A_STATE,5u);
 wf_set_be16(a,WF_A_SELECTOR,0x7bu);wf_set_u8(a,WF_A_LATCHED+1u,5u);
 wf_set_be32(a,WF_A_LINK,wf_actor_rom_base(0u));wf_set_be16(a,WF_A_PRIOR,0xffu);
 assert(wf_r17_dispatch_1c03e_r11(&rt,4u,&bridge)==WF_OK);
 assert(bridge.trace[4].callback_rom==0x1a538u&&bridge.trace[4].descriptor_rom==0x1a532u);
 assert(wf_be16(a,WF_A_MIRROR)==0xffffu&&wf_be16(a,WF_A_TIMER44)==128u);
 assert((wf_u8(a,WF_A_LATCHED)&0x80u)!=0u);
 assert(wf_r17_dispatch_1c03e_r11(&rt,4u,&bridge)==WF_OK);
 assert(wf_be16(a,WF_A_TIMER44)==127u);
}
static void test_68k_24bit_actor_pointer(void){
 wf_runtime_t rt;unsigned slot=99u;memset(&rt,0,sizeof(rt));
 assert(wf_actor_from_rom_ptr(&rt,0x801c09e0u,&slot)==&rt.actors[4]);
 assert(slot==4u);
}
static void test_14e40_linked_throw(void){
 wf_runtime_t rt;wf_r17_actions_r11_bridge_t bridge={0};
 wf_actor_raw_t *a=&rt.actors[4],*victim=&rt.actors[0];memset(&rt,0,sizeof(rt));
 active(a);active(victim);wf_set_be16(a,WF_A_STATE,5u);
 wf_set_be16(a,WF_A_SELECTOR,0x17u);wf_set_u8(a,WF_A_LATCHED+1u,5u);
 wf_set_be32(a,WF_A_LINK,wf_actor_rom_base(0u));
 wf_set_be32(victim,WF_A_LINK,wf_actor_rom_base(4u));
 wf_set_be16(a,WF_A_X,620u);wf_set_be16(a,WF_A_Y,338u);wf_set_be16(a,WF_A_Z,0x140u);
 assert(wf_r17_dispatch_1c03e_r11(&rt,4u,&bridge)==WF_OK);
 assert(bridge.trace[4].callback_rom==0x14e40u&&bridge.trace[4].descriptor_rom==0x14e2cu);
 assert(wf_be16(victim,WF_A_STATE)==0xffu&&wf_be16(victim,WF_A_X)==620u);
 assert(wf_be16(a,WF_A_ANIM_COUNTER)==8u&&wf_be16(a,WF_A_MIRROR)==0xfeu);
 wf_set_be16(a,WF_A_ANIM_INDEX,1u);wf_set_be16(a,WF_A_ANIM_COUNTER,0u);
 assert(wf_r17_dispatch_1c03e_r11(&rt,4u,&bridge)==WF_OK);
 assert(wf_be16(victim,WF_A_STATE)==2u&&wf_be16(victim,WF_A_TIMER44)==2u);
 assert((wf_u8(victim,WF_A_FLAGS34)&0x40u)!=0u);
 assert(wf_be32(victim,WF_A_LINK)==(wf_actor_rom_base(4u)|0x80000000u));
 assert(wf_be16(victim,WF_A_X)==700u&&wf_be16(victim,WF_A_Y)==336u);
}
static void test_11c82_thrown_initial(void){
 wf_runtime_t rt;wf_r17_dispatch_1c03e_ctx_t ctx={0};
 wf_actor_raw_t *a=&rt.actors[0];memset(&rt,0,sizeof(rt));active(a);
 wf_set_be16(a,WF_A_STATE,0xc002u);wf_set_be16(a,WF_A_LATCHED,0x4002u);
 wf_set_be16(a,WF_A_CHARACTER,0u);wf_set_be16(a,WF_A_X,700u);
 assert(wf_r17_dispatch_1c03e_bounded(&rt,a,&ctx)==WF_OK);
 assert(wf_u8(a,WF_A_MOTION_MODE)==1u&&wf_u8(a,WF_A_FLAGS33)==0x10u);
 assert(wf_be16(a,WF_A_MAGNITUDE)==0x2fu&&wf_be16(a,WF_A_PROFILE)==3u);
 assert(wf_be16(a,WF_A_ANIM_COUNTER)==6u&&wf_be16(a,WF_A_MIRROR)==9u);
 assert((wf_u8(a,WF_A_LATCHED)&0x80u)!=0u);
}
static void test_12566_hold_expiry_handoff(void){
 wf_runtime_t rt;wf_r17_dispatch_1c03e_ctx_t ctx={0};
 wf_actor_raw_t *a=&rt.actors[0],*linked=&rt.actors[4];memset(&rt,0,sizeof(rt));
 active(a);active(linked);wf_set_be16(a,WF_A_STATE,0x800cu);
 wf_set_be16(a,WF_A_LATCHED,0x800cu);wf_set_be32(a,WF_A_LINK,wf_actor_rom_base(4u));
 wf_set_be16(a,WF_A_TIMER46,1u);wf_set_be16(a,WF_A_ANIM_COUNTER,2u);
 wf_set_be16(linked,WF_A_STATE,0xffu);wf_set_u8(linked,WF_A_FACING,0x80u);
 assert(wf_r17_dispatch_1c03e_bounded(&rt,a,&ctx)==WF_OK);
 assert(wf_be16(a,WF_A_STATE)==0xffu&&wf_be16(a,WF_A_MIRROR)==0xffffu);
 assert(wf_be16(linked,WF_A_STATE)==12u&&wf_be16(linked,WF_A_MIRROR)==0x8039u);
 assert(ctx.extra_render_actor==linked);
}
static void test_1f06a_wait_and_1c2cc_post_throw(void){
 wf_runtime_t rt;wf_cpu_trace_t trace;memset(&rt,0,sizeof(rt));
 wf_actor_raw_t *a=&rt.actors[4],*target=&rt.actors[0];active(a);active(target);
 wf_set_be16(a,WF_A_STATE,12u);wf_set_u8(a,WF_A_CPU_BD,0u);
 assert(wf_cpu_actor_step(&rt,4u,&trace)==WF_OK);
 assert(trace.handler_rom==0x1f06au&&wf_u8(a,WF_A_CPU_BD)==1u);
 wf_set_be16(a,WF_A_STATE,0x8000u);
 wf_set_be32(a,WF_A_TARGET,wf_actor_rom_base(0u));
 wf_set_u8(a,WF_A_CPU_B5,8u);wf_set_u8(a,WF_A_FLAGS33,0x81u);
 wf_set_u8(a,WF_A_FLAGS34,0x80u);
 wf_set_u8(target,WF_A_FLAGS32,1u);
 assert(wf_cpu_actor_step(&rt,4u,&trace)==WF_OK);
 assert(trace.handler_rom==0x1c2ccu&&trace.blocked_dependency_rom==0u);
}
static void test_attract_low_health_target_1c4f4(void){
 wf_runtime_t rt;wf_cpu_trace_t trace;memset(&rt,0,sizeof rt);
 wf_actor_raw_t*a=&rt.actors[0],*target=&rt.actors[2];
 rt.m68k_regs.valid=1;rt.g.rng_state_valid=1;
 wf_set_u8(a,WF_A_ACTIVE,0x80u);wf_set_be16(a,WF_A_STATE,0x8000u);
 wf_set_be32(a,WF_A_TARGET,wf_actor_rom_base(2u));
 wf_set_be16(a,WF_A_STAMINA,0x7000u);wf_set_be16(target,WF_A_STAMINA,100u);
 wf_set_u8(a,WF_A_CPU_B5,8u);wf_set_u8(a,WF_A_INTERNAL_ID,0u);
 wf_set_u8(a,0x103u,1u);wf_set_u8(target,WF_A_FLAGS33,16u);
 (void)wf_cpu_actor_step(&rt,0u,&trace);
 assert(trace.blocked_dependency_rom!=0x1c4f4u);
 assert(wf_u8(a,WF_A_CPU_B7)&8u);
 assert(wf_u8(a,0x103u)==2u);
}
static void test_royal_action57_initial(void){
 wf_runtime_t rt;wf_r17_actions_r11_bridge_t bridge;
 memset(&rt,0,sizeof rt);memset(&bridge,0,sizeof bridge);
 wf_actor_raw_t*a=&rt.actors[6],*linked=&rt.actors[8];
 wf_set_u8(a,WF_A_ACTIVE,0x80u);wf_set_be16(a,WF_A_STATE,0x8005u);
 wf_set_u8(a,WF_A_LATCHED+1u,5u);
 wf_set_be16(a,WF_A_SELECTOR,0x57u);
 wf_set_be32(a,WF_A_LINK,wf_actor_rom_base(8u));
 wf_set_u8(linked,WF_A_ACTIVE,0x80u);
 (void)wf_r17_dispatch_1c03e_r11(&rt,6u,&bridge);
 assert(bridge.trace[6].callback_rom==0x1914eu);
 assert(wf_be16(linked,WF_A_STATE)==5u);
 assert(wf_be16(linked,WF_A_SELECTOR)==0x76u);
}
static void test_royal_action76_reaction(void){
 wf_runtime_t rt;wf_r17_actions_r11_bridge_t bridge;
 memset(&rt,0,sizeof rt);memset(&bridge,0,sizeof bridge);
 wf_actor_raw_t*a=&rt.actors[8];
 wf_set_u8(a,WF_A_ACTIVE,0x80u);wf_set_be16(a,WF_A_STATE,5u);
 wf_set_be16(a,WF_A_LATCHED,5u);wf_set_be16(a,WF_A_SELECTOR,0x76u);
 wf_set_u8(a,1u,3u);
 assert(wf_r17_dispatch_1c03e_r11(&rt,8u,&bridge)==WF_OK);
 assert(bridge.trace[8].callback_rom==0x1a10eu);
 assert(wf_u8(a,1u)==0u);
 wf_set_u8(a,WF_A_LATCHED,0x80u);wf_set_u8(a,WF_A_LATCHED+1u,5u);
 wf_set_be16(a,WF_A_ANIM_INDEX,0xfeu);
 assert(wf_r17_dispatch_1c03e_r11(&rt,8u,&bridge)==WF_OK);
 assert(wf_be16(a,WF_A_STATE)==4u);
 assert(wf_be16(a,WF_A_STATE4)==2u);
}
static void test_silent_presenter_pacing(void){
 uint8_t ram[0x4000]={0};wf_r17_m68k_shadow_t regs={0};
 wf_r18_prematch_t c={.ram=ram,.regs=&regs};uint8_t*p=ram+0x14ce;
 regs.valid=1u;p[0x1c]=0x80u;p[0x1d]=2u;
 p[0x1f]=54u;p[0x23]=2u;
 wf_r18_prematch_speech_divisor_set(1u);
 assert(wf_r18_prematch_tag_step_a68a(&c)==WF_OK);
 assert(p[0x1d]==2u);
 wf_r18_prematch_speech_divisor_set(3u);
 assert(wf_r18_prematch_tag_step_a68a(&c)==WF_OK);
 assert(p[0x1d]==3u);
 wf_r18_prematch_speech_divisor_set(1u);
}
static void test_state4_1bbc6_launch_and_landing(void){
 wf_runtime_t rt;wf_r17_actions_r11_bridge_t bridge;wf_r17_dispatch_1c03e_ctx_t dispatch;
 memset(&rt,0,sizeof rt);memset(&bridge,0,sizeof bridge);memset(&dispatch,0,sizeof dispatch);
 dispatch.actions=&bridge;
 for(unsigned i=0;i<11u;i++)rt.transient_1258[i][0]=0x80u;
 wf_actor_raw_t*a=&rt.actors[2];wf_set_be16(a,WF_A_ACTIVE,0x8000u);
 wf_set_be16(a,WF_A_STATE,0x8004u);wf_set_be16(a,WF_A_LATCHED,4u);
 wf_set_be16(a,WF_A_STATE4,0x1au);wf_set_be16(a,WF_A_FACING,0x8000u);
 rt.m68k_regs.valid=1u;rt.m68k_regs.d[0]=0x11223300u;rt.m68k_regs.d[1]=0x11223301u;
 assert(wf_r17_dispatch_1c03e_bounded(&rt,a,&dispatch)==WF_OK);
 assert(wf_u8(a,WF_A_MOTION_MODE)==2u&&wf_be16(a,WF_A_VX)==0u&&
  wf_be16(a,WF_A_VY)==0u&&wf_be16(a,WF_A_VZ)==0x200u&&
  wf_be16(a,WF_A_VZ_DEC)==0x40u);
 assert(wf_be16(a,WF_A_ANIM_COUNTER)==16u&&wf_be16(a,WF_A_MIRROR)==0x8016u);
 assert(rt.m68k_regs.d[0]==0x11223300u&&rt.m68k_regs.d[1]==0x11223301u);
 wf_set_be16(a,WF_A_ANIM_COUNTER,1u);rt.m68k_regs.d[1]=0x11223301u;
 assert(wf_r17_dispatch_1c03e_bounded(&rt,a,&dispatch)==WF_OK);
 assert(wf_be16(a,WF_A_ANIM_COUNTER)==0u&&rt.m68k_regs.d[1]==0x11223300u);
 wf_set_be16(a,0x9au,0x1234u);wf_set_u8(a,WF_A_CONTACT+1u,16u);
 assert(wf_r17_dispatch_1c03e_bounded(&rt,a,&dispatch)==WF_OK);
 assert(wf_be16(a,WF_A_STATE)==4u&&wf_be16(a,WF_A_STATE4)==9u&&
  wf_be16(a,0x9au)==0u&&rt.m68k_regs.d[0]==0x11223300u);
}
int main(void){test_f4c2_shadow();test_280dc_early_and_dispatch();test_280dc_routes();test_2208_mode2_exact_dead_clamp();test_f4c2_bounded_dispatch_and_queue();test_scheduler_advances_to_collision();test_11b5a_contact_transition();test_18b0e_action_4f();test_1cad6_latched_target_idle();test_1d398_uninitialized_state_returns();test_1d3a2_partner_pursuit_far_target();test_1c150_skips_human_actor();test_1cffa_defender_return_to_idle();test_1f5da_royal_defends_selector_4a();test_royal_action49_pin_intervention_initial();test_royal_action42_pin_victim_initial();test_royal_action42_pin_victim_release();test_royal_action23_pin_partner_route();test_11796_state_one_recovery();test_124fa_linked_hold_initial();test_14cf4_hold_action_initial();test_1a538_held_opponent();test_68k_24bit_actor_pointer();test_14e40_linked_throw();test_11c82_thrown_initial();test_12566_hold_expiry_handoff();test_1f06a_wait_and_1c2cc_post_throw();test_attract_low_health_target_1c4f4();test_royal_action57_initial();test_royal_action76_reaction();test_silent_presenter_pacing();test_state4_1bbc6_launch_and_landing();puts("PASS test_runtime17_f4c2");return 0;}
