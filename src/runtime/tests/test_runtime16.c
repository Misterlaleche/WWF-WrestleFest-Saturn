#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "wf_pair_relation.h"
#include "wf_actor_prepass.h"
#include "wf_gameplay_bindings.h"

static void active(wf_actor_raw_t*a){memset(a,0,sizeof(*a));wf_set_u8(a,WF_A_ACTIVE,0x80u);}
static void eligible(wf_actor_raw_t*a){active(a);wf_set_u8(a,WF_A_STATE,0x80u);}

static void test_f560_pair_stamina_route(void){
 wf_runtime_t rt;wf_gameplay_binding_context_t ctx;memset(&rt,0,sizeof(rt));memset(&ctx,0,sizeof(ctx));
 eligible(&rt.actors[0]);eligible(&rt.actors[1]);
 wf_set_be32(&rt.actors[0],WF_A_TARGET,wf_actor_rom_base(1));
 wf_set_be32(&rt.actors[1],WF_A_TARGET,wf_actor_rom_base(0));
 wf_set_u8(&rt.actors[0],WF_A_FACING,0x00u);wf_set_u8(&rt.actors[1],WF_A_FACING,0x80u);
 wf_set_be16(&rt.actors[0],WF_A_X,0x0200u);wf_set_be16(&rt.actors[1],WF_A_X,0x0220u);
 wf_set_be16(&rt.actors[0],WF_A_Y,0x0100u);wf_set_be16(&rt.actors[1],WF_A_Y,0x0108u);
 wf_set_be16(&rt.actors[0],WF_A_STAMINA,0x0060u);wf_set_be16(&rt.actors[1],WF_A_STAMINA,0x0040u);
 assert(wf_phase_pair_relation_f560(&rt,0x00f560u,&ctx)==WF_OK);
 assert(wf_be16(&rt.actors[0],WF_A_STATE)==0x000cu);
 assert(wf_be16(&rt.actors[1],WF_A_STATE)==0x00ffu);
 assert(wf_be16(&rt.actors[0],WF_A_X)==0x0210u&&wf_be16(&rt.actors[1],WF_A_X)==0x0210u);
 assert(wf_be16(&rt.actors[0],WF_A_Y)==0x0104u&&wf_be16(&rt.actors[1],WF_A_Y)==0x0104u);
 assert(wf_be32(&rt.actors[0],WF_A_LINK)==wf_actor_rom_base(1));
 assert(wf_be32(&rt.actors[1],WF_A_LINK)==wf_actor_rom_base(0));
 assert((wf_u8(&rt.actors[0],WF_A_FLAGS33)&0x40u)!=0u&&(wf_u8(&rt.actors[1],WF_A_FLAGS33)&0x40u)!=0u);
}

static void test_f560_fd00_and_boundaries(void){
 wf_runtime_t rt;wf_gameplay_binding_context_t ctx;memset(&rt,0,sizeof(rt));memset(&ctx,0,sizeof(ctx));
 wf_set_u8(&rt.actors[0],WF_A_INTERNAL_ID,0u);wf_set_u8(&rt.actors[0],WF_A_FLAGS32,1u);wf_set_be16(&rt.actors[0],WF_A_C8,0x0931u);
 assert(wf_phase_pair_relation_f560(&rt,0x00f560u,&ctx)==WF_OK);
 assert(wf_be16(&rt.actors[0],WF_A_C8)==0x03a9u);assert((wf_u8(&rt.actors[0],WF_A_C6)&0x40u)!=0u);
 rt.g.global_1678=0x80u;assert(wf_phase_pair_relation_f560(&rt,0x00f560u,&ctx)==WF_BLOCKED);assert(ctx.last_blocked_dependency==0x00f98cu);
 rt.g.global_1678=0u;rt.g.global_15d3=1u;assert(wf_phase_pair_relation_f560(&rt,0x00f560u,&ctx)==WF_BLOCKED);assert(ctx.last_blocked_dependency==0x0000a0e0u);
}

static void test_f18a_exact_idle_and_states(void){
 wf_runtime_t rt;wf_gameplay_binding_context_t ctx;memset(&rt,0,sizeof(rt));memset(&ctx,0,sizeof(ctx));
 active(&rt.actors[0]);wf_set_be16(&rt.actors[0],WF_A_STATE,3u);wf_set_be16(&rt.actors[0],WF_A_CPU_B4,0xffffu);wf_set_be16(&rt.actors[0],WF_A_CPU_B6,0xffffu);
 assert(wf_phase_actor_prepass_f18a(&rt,0x00f18au,&ctx)==WF_OK);assert(wf_be16(&rt.actors[0],WF_A_CPU_B4)==0u);assert(wf_be16(&rt.actors[0],WF_A_CPU_B6)==0u);
 memset(&rt,0,sizeof(rt));memset(&ctx,0,sizeof(ctx));active(&rt.actors[0]);wf_set_be16(&rt.actors[0],WF_A_STATE,9u);wf_set_u8(&rt.actors[0],WF_A_INPUT_DERIVED+1u,8u);
 assert(wf_phase_actor_prepass_f18a(&rt,0x00f18au,&ctx)==WF_OK);assert(wf_be16(&rt.actors[0],WF_A_STATE)==10u);
 memset(&rt,0,sizeof(rt));memset(&ctx,0,sizeof(ctx));active(&rt.actors[0]);wf_set_u8(&rt.actors[0],WF_A_FLAGS33,2u);
 assert(wf_phase_actor_prepass_f18a(&rt,0x00f18au,&ctx)==WF_BLOCKED);assert(ctx.last_blocked_dependency==0x0000dcdeu);
}

static void test_wp28_chain_moves_to_1f914(void){
 wf_runtime_t rt;wf_phase_binding_t b[16];wf_gameplay_binding_context_t ctx;memset(&rt,0,sizeof(rt));
 size_t n=wf_gameplay_bindings_build(&ctx,b,16);assert(n==10u);wf_runtime_init(&rt,b,n);
 ctx.cabinet_words_valid=1u;ctx.cabinet_word_140020=0xffffu;ctx.cabinet_word_140022=0xffffu;
 ctx.camera_valid=1u;ctx.camera.scene_index=0u;ctx.camera.scroll0_x=0x0140;ctx.camera.scroll1_x=0x0140;ctx.camera.scroll0_y=0x0200;ctx.camera.scroll1_y=0x0200;
 assert(wf_runtime_frame(&rt)==WF_BLOCKED);assert(rt.trace_count==10u);
 const uint32_t exp[10]={0x00514e,0x008b3a,0x02088e,0x020958,0x026936,0x02983c,0x00f560,0x00f18a,0x01c150,0x01f914};
 for(unsigned i=0;i<10;i++)assert(rt.trace[i]==exp[i]);
}
int main(void){test_f560_pair_stamina_route();test_f560_fd00_and_boundaries();test_f18a_exact_idle_and_states();test_wp28_chain_moves_to_1f914();puts("PASS test_runtime16");return 0;}
