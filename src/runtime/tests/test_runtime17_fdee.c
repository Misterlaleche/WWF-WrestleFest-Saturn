#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "wf_gameplay_bindings.h"
#include "wf_r17_ringside_fdee.h"

static void active(wf_actor_raw_t *a){wf_set_be16(a,WF_A_ACTIVE,0x8000u);}
static void init_obj(wf_actor_raw_t *a,uint16_t type,uint16_t state){
 memset(a,0,sizeof(*a));active(a);wf_set_be16(a,WF_A_REL_FLAGS,type);wf_set_be16(a,WF_A_LATCHED,state);wf_set_be16(a,WF_A_FLAGS32,4u);
}
static wf_r17_render_ctx_t render_ctx(wf_r17_render_queue_t *q){
 wf_r17_render_ctx_t r;memset(&r,0,sizeof(r));r.queue=q;r.visibility_2188=wf_r17_visibility_2188_exact;r.scroll_a_x=0x0280u;r.scroll_a_y=0x0140u;return r;
}
static void test_gate_and_state0(void){
 wf_runtime_t rt;wf_r17_render_queue_t q;wf_r17_fdee_trace_t t;wf_r17_render_ctx_t r;memset(&rt,0,sizeof(rt));wf_r17_render_queue_clear(&q);r=render_ctx(&q);
 init_obj(&rt.fixed_object0,0u,0u);init_obj(&rt.fixed_object1,1u,0u);wf_set_be16(&rt.fixed_object0,WF_A_MIRROR,0x1234u);
 assert(wf_r17_ringside_fdee(&rt,&r,&t)==WF_OK);assert(wf_be16(&rt.fixed_object0,WF_A_MIRROR)==0x1234u);assert(t.processed_objects==0u);
 rt.g.mode_0161=2u;assert(wf_r17_ringside_fdee(&rt,&r,&t)==WF_OK);assert(wf_be16(&rt.fixed_object0,WF_A_MIRROR)==0xffffu);assert(wf_be16(&rt.fixed_object1,WF_A_MIRROR)==0xffffu);assert(t.processed_objects==2u);
 puts("PASS $FDEE mode gate + state0 $FE36");
}
static void test_state1_follow(void){
 wf_runtime_t rt;wf_r17_render_queue_t q;wf_r17_fdee_trace_t t;wf_r17_render_ctx_t r;wf_actor_raw_t *p;memset(&rt,0,sizeof(rt));wf_r17_render_queue_clear(&q);r=render_ctx(&q);rt.g.mode_0161=2u;
 p=&rt.actors[0];active(p);wf_set_u8(p,WF_A_FACING,0x80u);wf_set_be16(p,WF_A_X,0x0200u);wf_set_be16(p,WF_A_Y,0x0130u);wf_set_be16(p,WF_A_Z,0x0100u);
 init_obj(&rt.fixed_object0,0u,1u);wf_set_be32(&rt.fixed_object0,WF_A_REL_LINK,wf_actor_rom_base(0));init_obj(&rt.fixed_object1,1u,0u);
 assert(wf_r17_ringside_fdee(&rt,&r,&t)==WF_OK);assert(wf_u8(&rt.fixed_object0,WF_A_FACING)==0x80u);assert(wf_be16(&rt.fixed_object0,WF_A_X)==0x0220u);assert(wf_be16(&rt.fixed_object0,WF_A_Y)==0x0130u);assert(wf_be16(&rt.fixed_object0,WF_A_Z)==0x0150u);assert(q.count[0]==0u&&q.count[2]==0u);
 puts("PASS $FE3E linked-object follow");
}
static void test_state2_setup_motion_and_contact(void){
 wf_runtime_t rt;wf_r17_render_queue_t q;wf_r17_fdee_trace_t t;wf_r17_render_ctx_t r;memset(&rt,0,sizeof(rt));wf_r17_render_queue_clear(&q);r=render_ctx(&q);rt.g.mode_0161=2u;rt.g.global_007e=3u;
 init_obj(&rt.fixed_object0,0u,2u);wf_set_be16(&rt.fixed_object0,WF_A_X,0x0300u);wf_set_be16(&rt.fixed_object0,WF_A_Y,0x0140u);wf_set_be16(&rt.fixed_object0,WF_A_Z,0x0100u);init_obj(&rt.fixed_object1,1u,0u);
 assert(wf_r17_ringside_fdee(&rt,&r,&t)==WF_OK);assert((wf_u8(&rt.fixed_object0,WF_A_LATCHED)&0x80u)!=0u);assert(wf_u8(&rt.fixed_object0,WF_A_MOTION_MODE)==2u);assert(wf_be16(&rt.fixed_object0,WF_A_VX)==0x00c0u);assert(wf_be16(&rt.fixed_object0,WF_A_VY)==0u);assert(wf_be16(&rt.fixed_object0,WF_A_VZ)==0x02c8u);assert(wf_be16(&rt.fixed_object0,WF_A_VZ_DEC)==0x0038u);assert(wf_be16(&rt.fixed_object0,WF_A_Z)==0x0103u);assert(q.count[0]==1u&&q.list[0][0]==&rt.fixed_object0);
 /* Mode 2/side 1, Z remains below $100 after $2208: low-byte flag bit4 makes TST.W +$36 succeed and state advances to 4. */
 wf_r17_render_queue_clear(&q);init_obj(&rt.fixed_object0,0u,2u);wf_set_be16(&rt.fixed_object0,WF_A_X,0x0300u);wf_set_be16(&rt.fixed_object0,WF_A_Y,0x0140u);wf_set_be16(&rt.fixed_object0,WF_A_Z,0x00fcu);rt.g.global_007e=2u;
 assert(wf_r17_ringside_fdee(&rt,&r,&t)==WF_OK);assert(wf_be16(&rt.fixed_object0,WF_A_LATCHED)==4u);assert(wf_be16(&rt.fixed_object0,WF_A_CONTACT)==0u);assert(wf_be16(&rt.fixed_object0,WF_A_Z)==0x0100u);
 puts("PASS $FE82 $258E/$2208/two-probe/contact transition");
}
static void test_state3_and_state4_clock(void){
 wf_runtime_t rt;wf_r17_render_queue_t q;wf_r17_fdee_trace_t t;wf_r17_render_ctx_t r;unsigned i;memset(&rt,0,sizeof(rt));wf_r17_render_queue_clear(&q);r=render_ctx(&q);rt.g.mode_0161=2u;
 init_obj(&rt.fixed_object0,0u,3u);wf_set_be16(&rt.fixed_object0,WF_A_X,0x0300u);wf_set_be16(&rt.fixed_object0,WF_A_Y,0x0140u);wf_set_be16(&rt.fixed_object0,WF_A_Z,0x0100u);wf_set_u8(&rt.fixed_object0,WF_A_FACING,0x80u);wf_set_u8(&rt.fixed_object0,WF_A_MOTION_MODE,2u);wf_set_be32(&rt.fixed_object0,WF_A_REL_LINK,0x12345678u);init_obj(&rt.fixed_object1,1u,0u);
 assert(wf_r17_ringside_fdee(&rt,&r,&t)==WF_OK);assert(wf_u8(&rt.fixed_object0,WF_A_MOTION_MODE)==0u);assert(wf_be16(&rt.fixed_object0,WF_A_RENDER_LIST)==2u);assert(wf_be32(&rt.fixed_object0,WF_A_REL_LINK)==0u);assert(wf_u8(&rt.fixed_object0,WF_A_MIRROR)==0x80u);assert(wf_u8(&rt.fixed_object0,WF_A_MIRROR+1u)==0u);assert(q.count[2]==1u);
 wf_r17_render_queue_clear(&q);init_obj(&rt.fixed_object0,0u,4u);wf_set_be16(&rt.fixed_object0,WF_A_X,0x0300u);wf_set_be16(&rt.fixed_object0,WF_A_Y,0x0140u);wf_set_be16(&rt.fixed_object0,WF_A_Z,0x0100u);wf_set_u8(&rt.fixed_object0,WF_A_FACING,0x80u);for(i=1u;i<=49u;i++){wf_r17_render_queue_clear(&q);assert(wf_r17_ringside_fdee(&rt,&r,&t)==WF_OK);if(i<49u)assert((wf_be16(&rt.fixed_object0,WF_A_LATCHED)&0x000fu)==4u);}assert(wf_be16(&rt.fixed_object0,WF_A_LATCHED)==3u);assert(q.count[2]==1u);
 puts("PASS $FF1E state3 + $FF54 49-call state4 clock");
}
static void test_fail_closed(void){
 wf_runtime_t rt;wf_r17_render_queue_t q;wf_r17_fdee_trace_t t;wf_r17_render_ctx_t r;memset(&rt,0,sizeof(rt));wf_r17_render_queue_clear(&q);r=render_ctx(&q);rt.g.mode_0161=2u;
 init_obj(&rt.fixed_object0,0u,5u);init_obj(&rt.fixed_object1,1u,0u);assert(wf_r17_ringside_fdee(&rt,&r,&t)==WF_BLOCKED);assert(t.blocked_dependency_rom==0x0000fe22u);
 init_obj(&rt.fixed_object0,2u,2u);assert(wf_r17_ringside_fdee(&rt,&r,&t)==WF_BLOCKED);assert(t.blocked_dependency_rom==0x0000258eu);
 puts("PASS $FDEE unresolved state/type fail-closed");
}
static void test_scheduler_boundary(void){
 wf_runtime_t rt;wf_gameplay_binding_context_t c;wf_phase_binding_t b[40];size_t n=wf_gameplay_bindings_build(&c,b,40u);assert(n>=29u);wf_runtime_init(&rt,b,n);rt.g.rng_state_valid=1u;rt.g.rng_state=0x13579bdfu;c.cabinet_words_valid=1u;c.cabinet_word_140020=0xffffu;c.cabinet_word_140022=0xffffu;c.camera_valid=1u;c.camera.scroll0_x=0x0140;c.camera.scroll0_y=0x0200;rt.g.mode_0161=1u;wf_r17_referee_init_10718(&rt);wf_r17_fixed_four_init_1004c(&rt);
 assert(wf_runtime_frame(&rt)==WF_BLOCKED);assert(rt.trace_count==23u);assert(rt.trace[22]==0x008cf8u);assert(rt.trace[13]==0x00fdeeu&&rt.trace[14]==0x010120u&&rt.trace[15]==0x010e6au&&rt.trace[16]==0x007a00u);assert(c.last_blocked_phase==0u||c.last_blocked_phase!=0x00fdeeu);puts("PASS scheduler crosses $FDEE and crosses $10E6A and crosses $7A00 and blocks at $8CF8");
}
int main(void){test_gate_and_state0();test_state1_follow();test_state2_setup_motion_and_contact();test_state3_and_state4_clock();test_fail_closed();test_scheduler_boundary();puts("PASS RUNTIME-17 $FDEE ringside owner");return 0;}
