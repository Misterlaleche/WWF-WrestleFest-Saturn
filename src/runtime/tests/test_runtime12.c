#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "wf_state5_exec.h"
#include "arcade_cpu_final_bridge.h"
#include "wf_cpu_runtime.h"
#include "wf_gameplay_bindings.h"

static void bind_target(wf_runtime_t *rt,unsigned a,unsigned t){wf_set_be32(&rt->actors[a],WF_A_TARGET,wf_actor_rom_base(t));}
static void prep5(wf_actor_raw_t *a,uint8_t sel){memset(a,0,sizeof(*a));wf_set_be16(a,WF_A_STATE,5u);wf_set_be16(a,WF_A_SELECTOR,sel);}

int main(void){
 wf_runtime_t rt; memset(&rt,0,sizeof(rt)); wf_state5_trace_t tr;
 /* Executor always clears +B9.bit1; non-forced suppression must skip callback. */
 prep5(&rt.actors[0],0x50u);wf_set_u8(&rt.actors[0],WF_A_STATE,0x80u);wf_set_u8(&rt.actors[0],WF_A_CPU_B9,0x02u);
 assert(wf_state5_execute(&rt,0,&tr)==WF_OK);assert(tr.invoked==0);assert((wf_u8(&rt.actors[0],WF_A_CPU_B9)&2u)==0);assert((wf_u8(&rt.actors[0],WF_A_CPU_B7)&0x80u)==0);
 /* Forced selector ignores suppression. Selector 52 enters exact sustained family. */
 prep5(&rt.actors[0],0x52u);wf_set_u8(&rt.actors[0],WF_A_STATE,0x80u);wf_set_u8(&rt.actors[0],WF_A_TIER_LO,0u);
 assert(wf_state5_execute(&rt,0,&tr)==WF_OK);assert(tr.invoked==1);assert(wf_be16(&rt.actors[0],WF_A_CPU_BA)==0x004cu);assert(wf_be16(&rt.actors[0],WF_A_CPU_BC)==0x0058u);assert((wf_u8(&rt.actors[0],WF_A_SELECTOR)&0x40u)!=0);
 /* Exact progress bucket and CLR.B +20 write width: main-state byte remains 5. */
 prep5(&rt.actors[0],0x4au);wf_set_be16(&rt.actors[0],WF_A_STAMINA,0x0064u);wf_set_u8(&rt.actors[0],WF_A_PROGRESS109,2u);wf_set_u8(&rt.actors[0],WF_A_STATE,0x80u);
 assert(wf_state5_execute(&rt,0,&tr)==WF_OK);assert(wf_u8(&rt.actors[0],WF_A_STATE)==0);assert(wf_u8(&rt.actors[0],WF_A_STATE+1u)==5u);assert(wf_be16(&rt.actors[0],WF_A_SELECTOR)==0x004bu);
 /* Reciprocal link is first-entry only and uses exact actor ROM pointers. */
 prep5(&rt.actors[0],0x0cu);prep5(&rt.actors[1],2u);bind_target(&rt,0,1);
 assert(wf_state5_execute(&rt,0,&tr)==WF_OK);assert(wf_be32(&rt.actors[0],WF_A_LINK)==wf_actor_rom_base(1));assert(wf_be32(&rt.actors[1],WF_A_LINK)==wf_actor_rom_base(0));
 /* Selector 01 exact state0 prefix is applied, then the unintegrated $1C5B6 tail fails closed. */
 prep5(&rt.actors[0],0x01u);assert(wf_state5_execute(&rt,0,&tr)==WF_BLOCKED);assert(tr.blocked_dependency_rom==0x001c5b6u);assert(wf_be16(&rt.actors[0],WF_A_STATE)==0u);assert(wf_be16(&rt.actors[0],WF_A_CPU_BC)==0x0080u);
 /* Selector 79 cannot invent inherited D0 for $10C60. */
 prep5(&rt.actors[0],0x79u);wf_set_u8(&rt.actors[0],WF_A_CPU_B5,0x80u);assert(wf_state5_execute(&rt,0,&tr)==WF_BLOCKED);assert(tr.blocked_dependency_rom==0x0010c60u);
 /* $1E5AE B5.bit4 link path has an extra target +$7A back-pointer absent from generic $1E6E2. */
 memset(&rt,0,sizeof(rt));prep5(&rt.actors[0],0x09u);prep5(&rt.actors[1],2u);bind_target(&rt,0,1);wf_set_u8(&rt.actors[0],WF_A_CPU_B5,0x10u);
 assert(wf_state5_execute(&rt,0,&tr)==WF_OK);assert(wf_be32(&rt.actors[1],WF_A_TARGET)==wf_actor_rom_base(0));assert(wf_be32(&rt.actors[0],WF_A_LINK)==wf_actor_rom_base(1));assert(wf_be32(&rt.actors[1],WF_A_LINK)==wf_actor_rom_base(0));
 /* $1E5AE clear-path picks $1F51E only for target X [0200,02FF], otherwise $1F45A. */
 memset(&rt,0,sizeof(rt));prep5(&rt.actors[0],0x09u);bind_target(&rt,0,1);wf_set_be16(&rt.actors[1],WF_A_X,0x0250u);assert(wf_state5_execute(&rt,0,&tr)==WF_BLOCKED&&tr.blocked_dependency_rom==0x001f51eu);
 memset(&rt,0,sizeof(rt));prep5(&rt.actors[0],0x09u);bind_target(&rt,0,1);wf_set_be16(&rt.actors[1],WF_A_X,0x0310u);assert(wf_state5_execute(&rt,0,&tr)==WF_BLOCKED&&tr.blocked_dependency_rom==0x001f45au);
 /* $1E5F6 exact facing/X gates: accepted boundary writes WORD $000A then reaches $1F45A. */
 memset(&rt,0,sizeof(rt));prep5(&rt.actors[0],0x08u);bind_target(&rt,0,1);wf_set_u8(&rt.actors[1],WF_A_FACING,0x80u);wf_set_be16(&rt.actors[1],WF_A_X,0x025fu);wf_set_u8(&rt.actors[0],WF_A_SELECTOR,0xa5u);
 assert(wf_state5_execute(&rt,0,&tr)==WF_BLOCKED&&tr.blocked_dependency_rom==0x001f45au);assert(wf_be16(&rt.actors[0],WF_A_SELECTOR)==0x000au);
 memset(&rt,0,sizeof(rt));prep5(&rt.actors[0],0x08u);bind_target(&rt,0,1);wf_set_u8(&rt.actors[1],WF_A_FACING,0x80u);wf_set_be16(&rt.actors[1],WF_A_X,0x0260u);assert(wf_state5_execute(&rt,0,&tr)==WF_BLOCKED&&tr.blocked_dependency_rom==0x001f4ceu);
 /* $1E62C uses the distinct 0250/02A0 thresholds only when $1C007C != 0. */
 memset(&rt,0,sizeof(rt));rt.g.credited_route=1u;prep5(&rt.actors[0],0x10u);bind_target(&rt,0,1);wf_set_u8(&rt.actors[1],WF_A_FACING,0u);wf_set_be16(&rt.actors[1],WF_A_X,0x02a0u);assert(wf_state5_execute(&rt,0,&tr)==WF_BLOCKED&&tr.blocked_dependency_rom==0x001f45au);assert(wf_be16(&rt.actors[0],WF_A_SELECTOR)==0x000au);
 /* CPU FINAL bridge is compiled in, exact tables visible, no synthetic seed policy added. */
 assert(arcade_cpu_main_handler(5u)==0x0001dfe2u);assert(arcade_cpu_state1_handler(10u)==0x0001d5a2u);assert(arcade_cpu_executor_entry(0x15u).force_call);
 /* CPU state 8 exact callback is CLR.B +BD; state 7 and A are RTS. */
 wf_cpu_trace_t ct; memset(&rt,0,sizeof(rt));wf_set_be16(&rt.actors[0],WF_A_STATE,8u);wf_set_u8(&rt.actors[0],WF_A_CPU_BD,0x55u);assert(wf_cpu_actor_step(&rt,0,&ct)==WF_OK);assert(wf_u8(&rt.actors[0],WF_A_CPU_BD)==0u);
 /* State9 LoD partner producer: after +BD reaches 8, partner 5/89 -> actor 5/0004 and immediate state5 dispatch. */
 memset(&rt,0,sizeof(rt));wf_set_be16(&rt.actors[0],WF_A_STATE,9u);wf_set_u8(&rt.actors[0],WF_A_CPU_BD,7u);wf_set_be32(&rt.actors[0],WF_A_TEAM_LINK,wf_actor_rom_base(1));wf_set_be32(&rt.actors[0],WF_A_TARGET,wf_actor_rom_base(2));wf_set_u8(&rt.actors[2],WF_A_INTERNAL_ID,0u);wf_set_be16(&rt.actors[1],WF_A_STATE,5u);wf_set_be16(&rt.actors[1],WF_A_SELECTOR,0x0089u);assert(wf_cpu_actor_step(&rt,0,&ct)==WF_OK);assert(wf_be16(&rt.actors[0],WF_A_STATE)==5u);assert(wf_be16(&rt.actors[0],WF_A_SELECTOR)==0x0004u);
 /* Real WP28 binding table remains concrete; later cumulative checkpoints add more owners without replacing these. */
 wf_phase_binding_t b[40];wf_gameplay_binding_context_t gc;size_t bn=wf_gameplay_bindings_build(&gc,b,40);assert(bn>=5u);assert(b[0].rom_addr==0x00514eu&&b[1].rom_addr==0x008b3au&&b[2].rom_addr==0x02088eu);
 wf_runtime_init(&rt,b,bn);rt.g.rng_state_valid=1u;rt.g.rng_state=0x13579bdfu;wf_set_u8(&rt.actors[0],WF_A_ACTIVE,0x80u);wf_set_be16(&rt.actors[0],WF_A_STATE,7u);gc.input_valid_mask=1u;gc.input_hw_low[0]=0xffu;assert(wf_dispatch_phase(&rt,0x00514eu)==WF_OK);assert(wf_dispatch_phase(&rt,0x01c150u)==WF_OK);wf_set_be16(&rt.actors[0],WF_A_STAMINA,30u);wf_set_be16(&rt.actors[0],WF_A_PENDING_DAMAGE,7u);assert(wf_dispatch_phase(&rt,0x024e58u)==WF_OK);assert(wf_be16(&rt.actors[0],WF_A_STAMINA)==23u);
 /* Cumulative runtime must still fail closed at the first mandatory owner not yet integrated. */
 memset(&rt,0,sizeof(rt));wf_runtime_init(&rt,b,bn);rt.g.rng_state_valid=1u;rt.g.rng_state=0x13579bdfu;gc.cabinet_words_valid=1u;gc.cabinet_word_140020=0xffffu;gc.cabinet_word_140022=0xffffu;gc.camera_valid=1u;gc.camera.scene_index=0u;gc.camera.scroll0_x=0x0140;gc.camera.scroll1_x=0x0140;gc.camera.scroll0_y=0x0200;gc.camera.scroll1_y=0x0200;assert(wf_runtime_frame(&rt)==WF_BLOCKED);assert(rt.trace_count>=4u);
 puts("PASS test_runtime12");return 0;
}
