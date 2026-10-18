#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "wf_gameplay_bindings.h"
#include "wf_pair_relation.h"
#include "wf_actor_prepass.h"
#include "wf_r17_follow_10e86.h"
#include "wf_r17_integration.h"
#include "wf_r17_dependencies.h"
#include "wf_r17_contract.h"
#include "wf_r17_referee.h"

static unsigned platform_calls,audio_calls;static uint16_t last_audio;
static wf_status_t platform_ok(wf_runtime_t *rt,uint8_t royal,void *u){(void)rt;(void)royal;(void)u;platform_calls++;return WF_OK;}
static wf_status_t audio_ok(wf_runtime_t *rt,uint16_t cmd,void *u){(void)rt;(void)u;audio_calls++;last_audio=cmd;return WF_OK;}
static wf_r17_tri_t visible_yes(wf_actor_raw_t *a,void *u){(void)a;(void)u;return WF_R17_YES;}
static void active(wf_actor_raw_t*a){memset(a,0,sizeof(*a));wf_set_u8(a,WF_A_ACTIVE,0x80u);}

static void test_shadow_and_f8c(void){
 wf_runtime_t rt;wf_r17_f8c_trace_t tr;memset(&rt,0,sizeof(rt));wf_runtime_init(&rt,0,0);
 assert(rt.m68k_regs.valid==1u);for(unsigned i=0;i<8u;i++)assert(rt.m68k_regs.d[i]==0u);
 rt.g.rng_state_valid=1u;rt.g.rng_state=0x12345678u;rt.g.frame_counter=99u;rt.g.pipeline_mode=1u;
 wf_r17_m68k_move_l(&rt.m68k_regs,1u,0x11112222u);wf_r17_m68k_move_l(&rt.m68k_regs,7u,0x77778888u);
 assert(wf_r17_runtime_enter_f8c(&rt,&tr)==WF_OK);assert(tr.rng_calls==1u);assert(rt.g.frame_counter==0u);assert((rt.g.gameplay_vblank_enable&0x80u)!=0u);assert(rt.g.frame_handshake[0]==0u&&rt.g.frame_handshake[1]==0u);assert(rt.m68k_regs.d[1]==0x11112222u&&rt.m68k_regs.d[7]==0x77778888u);assert(rt.m68k_regs.d[0]==rt.g.rng_state);
 puts("PASS R17 persistent shadow + F8C one-call handoff");
}

static void test_direct_attract_handoff_preserves_closed_bootstrap_state(void){
 wf_runtime_t rt;wf_r17_f8c_trace_t tr;memset(&rt,0,sizeof(rt));wf_runtime_init(&rt,0,0);
 rt.g.rng_state_valid=1u;rt.g.rng_state=0x89abcdefu;
 rt.g.mode_0160=1u;rt.g.mode_0161=1u;rt.g.progression_0162=9u;rt.g.global_0166=0x000du;
 active(&rt.actors[0]);active(&rt.actors[1]);
 wf_set_u8(&rt.actors[0],WF_A_INTERNAL_ID,8u);wf_set_u8(&rt.actors[0],WF_A_ROLE,0x80u);
 wf_set_u8(&rt.actors[1],WF_A_INTERNAL_ID,2u);wf_set_u8(&rt.actors[1],WF_A_ROLE,0xc0u);
 wf_set_be32(&rt.actors[0],WF_A_TEAM_LINK,wf_actor_rom_base(1));wf_set_be32(&rt.actors[1],WF_A_TEAM_LINK,wf_actor_rom_base(0));
 wf_set_be16(&rt.actors[0],WF_A_STAMINA,135u);wf_set_be16(&rt.actors[0],WF_A_STAMINA_MAX,135u);
 wf_set_be16(&rt.referee,WF_A_X,0x0280u);wf_set_be16(&rt.referee,WF_A_Y,0x0198u);
 wf_r17_m68k_move_l(&rt.m68k_regs,2u,0xdeadbeefu);wf_r17_m68k_move_l(&rt.m68k_regs,6u,0x12345678u);
 assert(wf_r17_runtime_enter_f8c(&rt,&tr)==WF_OK);
 assert(rt.g.mode_0160==1u&&rt.g.mode_0161==1u&&rt.g.progression_0162==9u&&rt.g.global_0166==0x000du);
 assert(wf_u8(&rt.actors[0],WF_A_INTERNAL_ID)==8u&&wf_u8(&rt.actors[0],WF_A_ROLE)==0x80u);
 assert(wf_u8(&rt.actors[1],WF_A_INTERNAL_ID)==2u&&wf_u8(&rt.actors[1],WF_A_ROLE)==0xc0u);
 assert(wf_be32(&rt.actors[0],WF_A_TEAM_LINK)==wf_actor_rom_base(1)&&wf_be32(&rt.actors[1],WF_A_TEAM_LINK)==wf_actor_rom_base(0));
 assert(wf_be16(&rt.actors[0],WF_A_STAMINA)==135u&&wf_be16(&rt.actors[0],WF_A_STAMINA_MAX)==135u);
 assert(wf_be16(&rt.referee,WF_A_X)==0x0280u&&wf_be16(&rt.referee,WF_A_Y)==0x0198u);
 assert(rt.m68k_regs.d[2]==0xdeadbeefu&&rt.m68k_regs.d[6]==0x12345678u);
 assert(wf_r17_attract_timeout_after_increment(99u,99u)==0);
 assert(wf_r17_attract_timeout_after_increment(100u,99u)==1);
 puts("PASS R17 Direct/ATTRACT CLOSED handoff preserves pre-F8C state");
}

static void test_10e86(void){
 wf_runtime_t rt;memset(&rt,0,sizeof(rt));wf_actor_raw_t*a=&rt.actors[0],*p=&rt.actors[1];
 wf_set_u8(a,WF_A_FLAGS33,1u);wf_set_be32(a,WF_A_TEAM_LINK,wf_actor_rom_base(1));wf_set_be16(a,WF_A_X,0x0230u);wf_set_be16(a,WF_A_Y,0x0180u);
 wf_set_u8(p,WF_A_FLAGS32,1u);wf_set_be16(p,WF_A_STATE,1u);wf_set_u8(p,WF_A_CPU_SUBSTATE,2u);
 assert(wf_r17_follow_10e86(&rt,a,0u)==WF_OK);assert(wf_be16(p,WF_A_STATE)==1u&&wf_u8(p,WF_A_CPU_SUBSTATE)==1u);
 puts("PASS R17 10E86 path E");
}
static void test_f98c_a0e0(void){
 wf_runtime_t rt;wf_gameplay_binding_context_t c;memset(&rt,0,sizeof(rt));memset(&c,0,sizeof(c));c.transition_reset=platform_ok;c.transition_barrier=platform_ok;c.audio_command=audio_ok;
 platform_calls=audio_calls=0;rt.g.global_1678=0x80u;rt.g.progression_0162=0u;assert(wf_r17_f98c(&rt,&c)==WF_OK);assert(platform_calls==2u);assert(audio_calls==1u&&last_audio==0x0080u);assert(rt.g.global_007f==2u);assert((rt.g.mode_0161&2u)!=0u);assert(wf_be16(&rt.referee,WF_A_X)==0x0340u);assert(wf_be16(&rt.referee,WF_A_Y)==0x0160u);
 memset(&rt,0,sizeof(rt));platform_calls=audio_calls=0;rt.g.global_1678=0xc0u;rt.g.progression_0162=1u;assert(wf_r17_f98c(&rt,&c)==WF_OK);assert(platform_calls==2u);assert(audio_calls==1u&&last_audio==0x0020u);assert(rt.g.global_007f==5u);assert((rt.g.mode_0161&2u)==0u);assert(wf_be16(&rt.referee,WF_A_X)==0x0280u);assert(wf_be16(&rt.referee,WF_A_Y)==0x0198u);
 memset(&rt,0,sizeof(rt));audio_calls=0;active(&rt.actors[0]);wf_set_u8(&rt.actors[0],WF_A_INTERNAL_ID,0u);wf_set_u8(&rt.actors[0],WF_A_FLAGS33,1u);rt.g.global_15d2=0u;rt.g.global_15d3=1u;assert(wf_r17_a0e0(&rt,&c)==WF_OK);assert(audio_calls==1u&&last_audio==0x3134u);assert(rt.g.global_15d4==0x0fu&&rt.g.global_15d5==1u&&rt.g.global_15d8==0x30u);
 puts("PASS R17 F98C both bodies + A0E0 mailbox");
}
static void test_f560_rng(void){
 wf_runtime_t rt;wf_gameplay_binding_context_t c;memset(&rt,0,sizeof(rt));memset(&c,0,sizeof(c));wf_runtime_init(&rt,0,0);rt.g.rng_state_valid=1u;rt.g.rng_state=0x10203040u;rt.g.credited_route=1u;rt.g.progression_0162=0u;
 active(&rt.actors[0]);active(&rt.actors[1]);wf_set_be16(&rt.actors[0],WF_A_STATE,0x8000u);wf_set_be16(&rt.actors[1],WF_A_STATE,0x8000u);wf_set_u8(&rt.actors[1],WF_A_ROLE,0x80u);wf_set_u8(&rt.actors[0],WF_A_FACING,0u);wf_set_u8(&rt.actors[1],WF_A_FACING,0x80u);wf_set_be32(&rt.actors[0],WF_A_TARGET,wf_actor_rom_base(1));wf_set_be32(&rt.actors[1],WF_A_TARGET,wf_actor_rom_base(0));wf_set_be16(&rt.actors[0],WF_A_X,0x200u);wf_set_be16(&rt.actors[1],WF_A_X,0x220u);wf_set_be16(&rt.actors[0],WF_A_Y,0x100u);wf_set_be16(&rt.actors[1],WF_A_Y,0x108u);
 assert(wf_phase_pair_relation_f560(&rt,0xf560u,&c)==WF_OK);assert(rt.g.rng_state!=0x10203040u);assert(rt.m68k_regs.d[0]==9u); /* $F5E4 restores D0; $250E finishes at slot 9. */
 assert(rt.g.rng_state==0xc1020305u); /* direct unmodified ROM oracle */
 assert(rt.m68k_regs.d[1]==0u&&rt.m68k_regs.d[2]==0x113u&&rt.m68k_regs.d[3]==8u&&rt.m68k_regs.d[4]==0xffffu);
 puts("PASS R17 F6F2 shared RNG shadow");
}
static void test_signed_y(void){
 wf_r17_render_queue_t q;wf_actor_raw_t a,b,*out[4];memset(&q,0,sizeof(q));memset(&a,0,sizeof(a));memset(&b,0,sizeof(b));wf_set_be16(&a,WF_A_Y,0xff00u);wf_set_be16(&b,WF_A_Y,0x0100u);q.list[0][0]=&a;q.list[0][1]=&b;q.count[0]=2u;assert(wf_r17_emit_actor_order_2836(&q,out,4u)==2u);assert(out[0]==&b&&out[1]==&a);(void)visible_yes;puts("PASS R17 signed-Y 2836 regression");
}
static void test_frame_regression(void){
 wf_runtime_t rt;wf_phase_binding_t b[40];wf_gameplay_binding_context_t c;size_t n=wf_gameplay_bindings_build(&c,b,40u);assert(n>=29u);wf_runtime_init(&rt,b,n);rt.g.rng_state_valid=1u;rt.g.rng_state=0x13579bdfu;c.cabinet_words_valid=1u;c.cabinet_word_140020=0xffffu;c.cabinet_word_140022=0xffffu;c.camera_valid=1u;c.camera.scene_index=0u;c.camera.scroll0_x=0x0140;c.camera.scroll1_x=0x0140;c.camera.scroll0_y=0x0200;c.camera.scroll1_y=0x0200;rt.g.mode_0161=1u;wf_r17_referee_init_10718(&rt);wf_r17_fixed_four_init_1004c(&rt);assert(wf_runtime_frame(&rt)==WF_BLOCKED);assert(rt.trace_count==23u&&rt.trace[9]==0x01f914u&&rt.trace[10]==0x00f4c2u&&rt.trace[11]==0x024062u&&rt.trace[12]==0x024e58u&&rt.trace[13]==0x00fdeeu&&rt.trace[14]==0x010120u&&rt.trace[15]==0x010e6au&&rt.trace[16]==0x007a00u);puts("PASS R17 ringside owner advances exact scheduler boundary through 10120 through 10E6A to 88A2");
}
int main(void){test_shadow_and_f8c();test_direct_attract_handoff_preserves_closed_bootstrap_state();test_10e86();test_f98c_a0e0();test_f560_rng();test_signed_y();test_frame_regression();puts("PASS test_runtime17");return 0;}
