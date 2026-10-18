#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "wf_gameplay_bindings.h"
#include "wf_r17_fixed_four_10120.h"

static unsigned audio_calls;static uint16_t audio_last;
static wf_status_t audio_ok(wf_runtime_t*rt,uint16_t cmd,void*u){(void)rt;(void)u;audio_calls++;audio_last=cmd;return WF_OK;}
static void render0(wf_r17_render_ctx_t*r,wf_r17_render_queue_t*q){memset(q,0,sizeof(*q));r->scroll_a_x=0x0140u;r->scroll_a_y=0x0200u;r->queue=q;r->visibility_2188=wf_r17_visibility_2188_exact;r->visibility_user=0;}
static wf_actor_raw_t *o(wf_runtime_t*rt,unsigned i){return i==0?&rt->fixed_four_1134:i==1?&rt->referee_aux:i==2?&rt->fixed_four_1194:&rt->fixed_four_11c4;}
static void test_init(void){wf_runtime_t rt;memset(&rt,0,sizeof(rt));wf_r17_fixed_four_init_1004c(&rt);
 assert(wf_be16(o(&rt,0),WF_A_ACTIVE)==0x8000u&&wf_be16(o(&rt,0),WF_A_X)==0x01c0u&&wf_be16(o(&rt,0),WF_A_RENDER_LIST)==1u&&wf_be16(o(&rt,0),WF_A_FACING)==0u);
 assert(wf_be16(o(&rt,1),WF_A_X)==0x01c0u&&wf_be16(o(&rt,1),WF_A_RENDER_LIST)==3u);
 assert(wf_be16(o(&rt,2),WF_A_X)==0x0330u&&wf_be16(o(&rt,2),WF_A_RENDER_LIST)==1u&&wf_be16(o(&rt,2),WF_A_FACING)==0x8000u);
 assert(wf_be16(o(&rt,3),WF_A_X)==0x0330u&&wf_be16(o(&rt,3),WF_A_RENDER_LIST)==3u&&wf_be16(o(&rt,3),WF_A_FACING)==0x8000u);
 for(unsigned i=0;i<4;i++){assert(wf_be16(o(&rt,i),WF_A_CHARACTER)==0x000eu);assert(wf_be16(o(&rt,i),WF_A_Y)==0x0178u);assert(wf_be16(o(&rt,i),WF_A_Z)==0x0140u);assert(wf_be16(o(&rt,i),WF_A_LATCHED)==0u);}puts("PASS $1004C fixed-four exact initialization");}
static void test_gate_state0(void){wf_runtime_t rt;wf_r17_render_ctx_t r;wf_r17_render_queue_t q;wf_r17_fixed_four_trace_t t;memset(&rt,0,sizeof(rt));wf_r17_fixed_four_init_1004c(&rt);render0(&r,&q);rt.g.mode_0161=2u;
 assert(wf_r17_fixed_four_10120(&rt,&r,audio_ok,0,&t)==WF_OK);assert(t.processed_objects==0u);assert(wf_be16(o(&rt,0),WF_A_LATCHED)==0u);
 rt.g.mode_0161=0u;assert(wf_r17_fixed_four_10120(&rt,&r,audio_ok,0,&t)==WF_OK);assert(t.processed_objects==4u);
 assert(wf_be16(o(&rt,0),WF_A_LATCHED)==0x8000u&&wf_be16(o(&rt,0),WF_A_MIRROR)==0x0002u);
 assert(wf_be16(o(&rt,1),WF_A_LATCHED)==0x8000u&&wf_be16(o(&rt,1),WF_A_MIRROR)==0x000au);
 assert(wf_be16(o(&rt,2),WF_A_MIRROR)==0x8002u&&wf_be16(o(&rt,3),WF_A_MIRROR)==0x800au);puts("PASS $10120 mode gate + $10186 state0");}
static void test_state1_state2(void){wf_runtime_t rt;wf_r17_render_ctx_t r;wf_r17_render_queue_t q;wf_r17_fixed_four_trace_t t;memset(&rt,0,sizeof(rt));wf_r17_fixed_four_init_1004c(&rt);render0(&r,&q);rt.g.mode_0161=0u;audio_calls=0;
 wf_set_be16(o(&rt,0),WF_A_LATCHED,1u);wf_set_be16(o(&rt,1),WF_A_LATCHED,2u);assert(wf_r17_fixed_four_10120(&rt,&r,audio_ok,0,&t)==WF_OK);assert(audio_calls==2u&&audio_last==0x002cu);
 assert(wf_be16(o(&rt,0),WF_A_ANIM_INDEX)==0u&&wf_u8(o(&rt,0),WF_A_MIRROR+1u)==3u&&wf_u8(o(&rt,0),WF_A_ANIM_COUNTER+1u)==6u);
 assert(wf_be16(o(&rt,1),WF_A_ANIM_INDEX)==0u&&wf_u8(o(&rt,1),WF_A_MIRROR+1u)==7u&&wf_u8(o(&rt,1),WF_A_ANIM_COUNTER+1u)==0x10u);
 for(unsigned i=0;i<6u;i++){wf_r17_render_queue_clear(&q);assert(wf_r17_fixed_four_10120(&rt,&r,audio_ok,0,&t)==WF_OK);}assert(wf_be16(o(&rt,0),WF_A_ANIM_INDEX)==1u&&wf_u8(o(&rt,0),WF_A_MIRROR+1u)==4u&&wf_u8(o(&rt,0),WF_A_ANIM_COUNTER+1u)==6u);puts("PASS $101B2/$10228 exact sequence entry and timer step");}
static void test_state3_state4_terminal(void){wf_runtime_t rt;wf_r17_render_ctx_t r;wf_r17_render_queue_t q;wf_r17_fixed_four_trace_t t;memset(&rt,0,sizeof(rt));wf_r17_fixed_four_init_1004c(&rt);render0(&r,&q);rt.g.mode_0161=0u;
 wf_set_be16(o(&rt,0),WF_A_LATCHED,3u);audio_calls=0;assert(wf_r17_fixed_four_10120(&rt,&r,audio_ok,0,&t)==WF_OK);assert(audio_calls==1u&&wf_u8(o(&rt,0),WF_A_MIRROR+1u)==4u&&wf_u8(o(&rt,0),WF_A_ANIM_COUNTER+1u)==0x0au);
 for(unsigned f=0;f<26u;f++){wf_r17_render_queue_clear(&q);assert(wf_r17_fixed_four_10120(&rt,&r,audio_ok,0,&t)==WF_OK);}assert((wf_be16(o(&rt,0),WF_A_LATCHED)&0x000fu)==0u);
 wf_set_be16(o(&rt,0),WF_A_LATCHED,4u);audio_calls=0;wf_r17_render_queue_clear(&q);assert(wf_r17_fixed_four_10120(&rt,&r,audio_ok,0,&t)==WF_OK);assert(audio_calls==1u&&wf_u8(o(&rt,0),WF_A_MIRROR+1u)==6u&&wf_u8(o(&rt,0),WF_A_ANIM_COUNTER+1u)==0x0eu);
 for(unsigned f=0;f<26u;f++){wf_r17_render_queue_clear(&q);assert(wf_r17_fixed_four_10120(&rt,&r,audio_ok,0,&t)==WF_OK);}assert((wf_be16(o(&rt,0),WF_A_LATCHED)&0x000fu)==0u);puts("PASS $1029E/$102F6 exact terminal frame counts");}
static void test_fail_closed(void){wf_runtime_t rt,before;wf_r17_render_ctx_t r;wf_r17_render_queue_t q;wf_r17_fixed_four_trace_t t;memset(&rt,0,sizeof(rt));wf_r17_fixed_four_init_1004c(&rt);render0(&r,&q);rt.g.mode_0161=0u;wf_set_be16(o(&rt,0),WF_A_LATCHED,1u);before=rt;
 assert(wf_r17_fixed_four_10120(&rt,&r,0,0,&t)==WF_BLOCKED&&t.blocked_dependency_rom==0x00002052u);assert(memcmp(&rt,&before,sizeof(rt))==0);
 wf_set_be16(o(&rt,0),WF_A_LATCHED,5u);assert(wf_r17_fixed_four_10120(&rt,&r,audio_ok,0,&t)==WF_BLOCKED&&t.blocked_dependency_rom==0x00010172u);puts("PASS $10120 invalid/audio dependency fail-closed");}
static void test_scheduler(void){wf_runtime_t rt;wf_gameplay_binding_context_t c;wf_phase_binding_t b[40];size_t n=wf_gameplay_bindings_build(&c,b,40u);assert(n>=29u);wf_runtime_init(&rt,b,n);rt.g.rng_state_valid=1u;rt.g.rng_state=0x13579bdfu;c.cabinet_words_valid=1u;c.cabinet_word_140020=c.cabinet_word_140022=0xffffu;c.camera_valid=1u;c.camera.scroll0_x=0x0140;c.camera.scroll1_x=0x0140;c.camera.scroll0_y=0x0200;c.camera.scroll1_y=0x0200;rt.g.mode_0161=1u;wf_r17_referee_init_10718(&rt);wf_r17_fixed_four_init_1004c(&rt);
 assert(wf_runtime_frame(&rt)==WF_BLOCKED);assert(rt.trace_count==23u&&rt.trace[13]==0x00fdeeu&&rt.trace[14]==0x010120u&&rt.trace[15]==0x010e6au&&rt.trace[16]==0x007a00u);assert(c.last_blocked_phase==0u||c.last_blocked_phase!=0x010120u);puts("PASS scheduler crosses $10120 and crosses $10E6A and crosses $7A00 and blocks at $8CF8");}
static void test_shadow(void){
 wf_runtime_t rt;wf_r17_render_ctx_t r;wf_r17_render_queue_t q;wf_r17_fixed_four_trace_t t;
 for(unsigned state=0;state<5;state++){
  memset(&rt,0,sizeof(rt));wf_r17_fixed_four_init_1004c(&rt);render0(&r,&q);
  for(unsigned i=0;i<8;i++)rt.m68k_regs.d[i]=0x12345678u+i;
  wf_set_be16(o(&rt,3),WF_A_LATCHED,(uint16_t)state);
  assert(wf_r17_fixed_four_10120(&rt,&r,audio_ok,0,&t)==WF_OK);
  assert(rt.m68k_regs.d[0]==(state?0x12340000u:0x12345678u));
  assert(rt.m68k_regs.d[1]==0x12340010u&&rt.m68k_regs.d[2]==0x12340000u+state*4u);
  for(unsigned i=3;i<8;i++)assert(rt.m68k_regs.d[i]==0x12345678u+i);
  wf_set_be16(o(&rt,3),WF_A_ANIM_COUNTER,2u);wf_r17_render_queue_clear(&q);
  rt.m68k_regs.d[0]=0x98765432u;
  assert(wf_r17_fixed_four_10120(&rt,&r,audio_ok,0,&t)==WF_OK);
  assert(rt.m68k_regs.d[0]==0x98765432u); /* timer-only/state0 return */
 }
 puts("PASS $10120 D0-D7 WORD preservation and all five state exits");
}
int main(void){test_shadow();test_init();test_gate_state0();test_state1_state2();test_state3_state4_terminal();test_fail_closed();test_scheduler();puts("PASS RUNTIME-17 $10120 fixed-four owner");return 0;}
