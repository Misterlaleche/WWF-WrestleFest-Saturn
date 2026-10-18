#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "wf_target_relation.h"
#include "wf_gameplay_bindings.h"

static void activate(wf_actor_raw_t *a){memset(a,0,sizeof(*a));wf_set_u8(a,WF_A_ACTIVE,0x80u);}

static void test_royal_primary_metric(void){
 wf_runtime_t rt;memset(&rt,0,sizeof(rt));rt.g.mode_0161=1u;
 activate(&rt.actors[0]); activate(&rt.actors[1]);
 wf_set_u8(&rt.actors[1],WF_A_ROLE,0x40u);
 wf_set_be16(&rt.actors[0],WF_A_X,0x0200);wf_set_be16(&rt.actors[0],WF_A_Y,0x0100);
 wf_set_be16(&rt.actors[1],WF_A_X,0x0240);wf_set_be16(&rt.actors[1],WF_A_Y,0x0110);
 assert(wf_phase_target_relation_20958(&rt,0x020958u,0)==WF_OK);
 assert(rt.g.global_168c==wf_actor_rom_base(1));
 assert(rt.g.global_1692==0x20u); /* (0x40>>2)+0x10 */
}

static void test_tag_pair_outputs(void){
 wf_runtime_t rt;memset(&rt,0,sizeof(rt));rt.g.mode_0161=0u;
 for(unsigned i=0;i<4;i++)activate(&rt.actors[i]);
 wf_set_be32(&rt.actors[0],WF_A_TEAM_LINK,wf_actor_rom_base(1));
 wf_set_be32(&rt.actors[1],WF_A_TEAM_LINK,wf_actor_rom_base(0));
 wf_set_be32(&rt.actors[2],WF_A_TEAM_LINK,wf_actor_rom_base(3));
 wf_set_be32(&rt.actors[3],WF_A_TEAM_LINK,wf_actor_rom_base(2));
 wf_set_u8(&rt.actors[0],WF_A_FLAGS33,1u);
 wf_set_u8(&rt.actors[2],WF_A_FLAGS33,1u|0x40u);
 wf_set_u8(&rt.actors[3],WF_A_FLAGS33,0u);
 assert(wf_phase_target_relation_20958(&rt,0x020958u,0)==WF_OK);
 assert(wf_be32(&rt.actors[0],WF_A_TARGET)!=0u);
 assert(wf_be32(&rt.actors[0],WF_A_TARGET2)!=0u);
 assert(wf_be32(&rt.actors[0],WF_A_TARGET)!=wf_actor_rom_base(0));
 assert(wf_be32(&rt.actors[0],WF_A_TARGET)!=wf_actor_rom_base(1));
}

static void test_wp28_real_chain_to_f560(void){
 wf_runtime_t rt;memset(&rt,0,sizeof(rt));wf_phase_binding_t b[40];wf_gameplay_binding_context_t ctx;
 size_t n=wf_gameplay_bindings_build(&ctx,b,40);assert(n>=8u);
 wf_runtime_init(&rt,b,n);rt.g.rng_state_valid=1u;rt.g.rng_state=0x13579bdfu;
 ctx.cabinet_words_valid=1u;ctx.cabinet_word_140020=0xffffu;ctx.cabinet_word_140022=0xffffu;
 ctx.camera_valid=1u;ctx.camera.scene_index=0u;ctx.camera.scroll0_x=0x0140;ctx.camera.scroll1_x=0x0140;ctx.camera.scroll0_y=0x0200;ctx.camera.scroll1_y=0x0200;
 assert(wf_runtime_frame(&rt)==WF_BLOCKED);
 assert(rt.trace_count>=7u);
 assert(rt.trace[0]==0x00514eu);assert(rt.trace[1]==0x008b3au);assert(rt.trace[2]==0x02088eu);
 assert(rt.trace[3]==0x020958u);assert(rt.trace[4]==0x026936u);assert(rt.trace[5]==0x02983cu);assert(rt.trace[6]==0x00f560u);
}
int main(void){test_royal_primary_metric();test_tag_pair_outputs();test_wp28_real_chain_to_f560();puts("PASS test_runtime15");return 0;}
