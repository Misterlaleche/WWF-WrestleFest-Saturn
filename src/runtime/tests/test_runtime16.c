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
 eligible(&rt.actors[0]);eligible(&rt.actors[1]);wf_set_be32(&rt.actors[0],WF_A_TARGET,wf_actor_rom_base(1));wf_set_be32(&rt.actors[1],WF_A_TARGET,wf_actor_rom_base(0));wf_set_u8(&rt.actors[0],WF_A_FACING,0);wf_set_u8(&rt.actors[1],WF_A_FACING,0x80);wf_set_be16(&rt.actors[0],WF_A_X,0x0200);wf_set_be16(&rt.actors[1],WF_A_X,0x0220);wf_set_be16(&rt.actors[0],WF_A_Y,0x0100);wf_set_be16(&rt.actors[1],WF_A_Y,0x0108);wf_set_be16(&rt.actors[0],WF_A_STAMINA,0x60);wf_set_be16(&rt.actors[1],WF_A_STAMINA,0x40);
 assert(wf_phase_pair_relation_f560(&rt,0x00f560u,&ctx)==WF_OK);assert(wf_be16(&rt.actors[0],WF_A_STATE)==0x000c);assert(wf_be16(&rt.actors[1],WF_A_STATE)==0x00ff);assert(wf_be32(&rt.actors[0],WF_A_LINK)==wf_actor_rom_base(1));assert(wf_be32(&rt.actors[1],WF_A_LINK)==wf_actor_rom_base(0));
}
static void test_f560_boundaries(void){wf_runtime_t rt;wf_gameplay_binding_context_t ctx;memset(&rt,0,sizeof(rt));memset(&ctx,0,sizeof(ctx));rt.g.global_1678=0x80;assert(wf_phase_pair_relation_f560(&rt,0x00f560u,&ctx)==WF_BLOCKED);assert(ctx.last_blocked_dependency==0x00f98c);rt.g.global_1678=0;rt.g.global_15d3=1;assert(wf_phase_pair_relation_f560(&rt,0x00f560u,&ctx)==WF_BLOCKED);assert(ctx.last_blocked_dependency==0x0000a0e0);}
static void test_f18a(void){wf_runtime_t rt;wf_gameplay_binding_context_t ctx;memset(&rt,0,sizeof(rt));memset(&ctx,0,sizeof(ctx));active(&rt.actors[0]);wf_set_be16(&rt.actors[0],WF_A_STATE,9);wf_set_u8(&rt.actors[0],WF_A_INPUT_DERIVED+1,8);assert(wf_phase_actor_prepass_f18a(&rt,0x00f18au,&ctx)==WF_OK);assert(wf_be16(&rt.actors[0],WF_A_STATE)==10);}
static void test_chain(void){wf_runtime_t rt;wf_phase_binding_t b[16];wf_gameplay_binding_context_t ctx;memset(&rt,0,sizeof(rt));size_t n=wf_gameplay_bindings_build(&ctx,b,16);assert(n==10);wf_runtime_init(&rt,b,n);ctx.cabinet_words_valid=1;ctx.cabinet_word_140020=0xffff;ctx.cabinet_word_140022=0xffff;ctx.camera_valid=1;ctx.camera.scene_index=0;ctx.camera.scroll0_x=0x140;ctx.camera.scroll1_x=0x140;ctx.camera.scroll0_y=0x200;ctx.camera.scroll1_y=0x200;assert(wf_runtime_frame(&rt)==WF_BLOCKED);assert(rt.trace_count==10);const uint32_t e[10]={0x514e,0x8b3a,0x2088e,0x20958,0x26936,0x2983c,0xf560,0xf18a,0x1c150,0x1f914};for(unsigned i=0;i<10;i++)assert(rt.trace[i]==e[i]);}
int main(void){test_f560_pair_stamina_route();test_f560_boundaries();test_f18a();test_chain();puts("PASS test_runtime16");return 0;}
