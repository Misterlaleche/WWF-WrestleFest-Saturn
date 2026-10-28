#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "wf_gameplay_bindings.h"
#include "wf_r17_referee.h"
#include "wf_r17_fixed_four_10120.h"
static wf_status_t released_input(wf_runtime_t*r,uint32_t at,uint16_t*out,void*u){
 (void)r;(void)u;assert(at>=0x140020u&&at<=0x140026u&&!(at&1u));*out=0xffffu;return WF_OK;
}

static wf_status_t render_ok(wf_runtime_t *rt,wf_actor_raw_t *const *actors,size_t count,void *user){(void)rt;(void)actors;(void)count;(void)user;return WF_OK;}

int main(void){
    wf_runtime_t rt;wf_phase_binding_t b[40];wf_gameplay_binding_context_t c;size_t n=wf_gameplay_bindings_build(&c,b,40u);uint32_t rng_after_frame0;
    assert(n>=35u);wf_runtime_init(&rt,b,n);
    rt.g.rng_state_valid=1u;rt.g.rng_state=0x2468ace1u;
    rt.g.pipeline_mode=1u;             /* WP28 branch B */
    rt.g.credited_route=1u;            /* credited gameplay */
    rt.g.mode_0161=0u;                 /* non-Royal post-ACK route */
    rt.g.global_16c4=0x0001u;          /* exact low-byte gate for $7548/$C04A; $C710 word gate */
    rt.g.global_16d2=0x0001u;          /* exact $26A42 already-active early return */
    rt.branchb_prev_185a[0]=0xffu;     /* $285DA empty previous list */
    rt.g.global_007e=3u;               /* $2867A entry -> immediate $FF list */

    c.cabinet_words_valid=1u;c.cabinet_word_140020=0xffffu;c.cabinet_word_140022=0xffffu;
    c.camera_valid=1u;c.camera.scene_index=0u;c.camera.scroll0_x=0x0140;c.camera.scroll1_x=0x0140;c.camera.scroll0_y=0x0200;c.camera.scroll1_y=0x0200;
    c.post7_render_2836_batch=render_ok;
    c.participant_input_word=released_input;

    for(unsigned i=0u;i<2u;i++){
        wf_set_be32(&rt.actors[i],WF_A_INPUT_SOURCE,0x140020u+i*2u);
        wf_set_u8(&rt.actors[i],WF_A_ACTIVE,0x80u);wf_set_u8(&rt.actors[i],WF_A_FLAGS32,0x80u);wf_set_u8(&rt.actors[i],WF_A_FLAGS33,0x01u);wf_set_u8(&rt.actors[i],WF_A_STATE+1u,0x07u);
        wf_set_be16(&rt.actors[i],WF_A_X,(uint16_t)(0x0200u+i*0x20u));wf_set_be16(&rt.actors[i],WF_A_Y,0x0180u);c.input_valid_mask=(uint16_t)(c.input_valid_mask|(1u<<i));c.input_hw_low[i]=0xffu;
    }
    wf_r17_referee_init_10718(&rt);wf_r17_fixed_four_init_1004c(&rt);

    assert(wf_runtime_frame(&rt)==WF_BLOCKED);
    if(rt.trace_count!=26u){fprintf(stderr,"branchB blocked trace_count=%zu last=%06X dep=%06X\n",rt.trace_count,rt.trace_count?rt.trace[rt.trace_count-1]:0u,c.last_blocked_dependency);return 2;}
    assert(rt.trace[0]==0x0285dau&&rt.trace[1]==0x026a42u);assert(rt.trace[21]==0x00bcfeu&&rt.trace[22]==0x00c04au&&rt.trace[23]==0x00c710u&&rt.trace[24]==0x002836u&&rt.trace[25]==0x0298ecu);
    assert(rt.frame_ready==1u&&rt.frame_stage==1u&&(rt.g.frame_handshake[1]&1u)!=0u);rng_after_frame0=rt.g.rng_state;
    wf_runtime_vblank_ack(&rt);assert(wf_runtime_frame(&rt)==WF_OK);assert(rt.g.frame_counter==1u&&rt.frame_stage==0u&&rt.frame_ready==0u);assert(rt.g.rng_state==rng_after_frame0);
    assert(wf_runtime_frame(&rt)==WF_BLOCKED);assert(rt.g.rng_state!=rng_after_frame0);
    puts("PASS branch B real bindings: 26 owners -> VBlank ACK -> post-ACK -> next $FA0 RNG");return 0;
}
