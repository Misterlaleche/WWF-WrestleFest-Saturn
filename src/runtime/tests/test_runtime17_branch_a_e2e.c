#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "wf_gameplay_bindings.h"
#include "wf_r17_referee.h"
#include "wf_r17_fixed_four_10120.h"
static wf_status_t released_input(wf_runtime_t*r,uint32_t at,uint16_t*out,void*u){
 (void)r;(void)u;assert(at>=0x140020u&&at<=0x140026u&&!(at&1u));*out=0xffffu;return WF_OK;
}

static wf_status_t render_ok(wf_runtime_t *rt,wf_actor_raw_t *const *actors,size_t count,void *user){
    (void)rt;(void)actors;(void)count;(void)user;return WF_OK;
}
static wf_status_t tag_ok(wf_runtime_t *rt,void *user){(void)rt;(void)user;return WF_OK;}

int main(void){
    wf_runtime_t rt;
    wf_phase_binding_t b[40];
    wf_gameplay_binding_context_t c;
    size_t n=wf_gameplay_bindings_build(&c,b,40u);
    uint32_t rng_after_frame0;
    assert(n>=29u);
    wf_runtime_init(&rt,b,n);

    rt.g.rng_state_valid=1u;
    rt.g.rng_state=0x13579bdfu;
    rt.g.pipeline_mode=0u;              /* WP28 branch A */
    rt.g.credited_route=1u;             /* normal credited route */
    rt.g.mode_0161=0u;                  /* non-Royal post-ACK route */
    rt.g.global_169e=0x3000u;           /* exact early gates for $262D2/$9052 */
    rt.g.global_167b=0x01u;             /* exact $899C early return */

    c.cabinet_words_valid=1u;
    c.cabinet_word_140020=0xffffu;
    c.cabinet_word_140022=0xffffu;
    c.camera_valid=1u;
    c.camera.scene_index=0u;
    c.camera.scroll0_x=0x0140;
    c.camera.scroll1_x=0x0140;
    c.camera.scroll0_y=0x0200;
    c.camera.scroll1_y=0x0200;
    c.post7_render_2836_batch=render_ok;
    c.post7_tag_6e34=tag_ok;
    c.participant_input_word=released_input;

    /* Non-Royal referee needs two active +$33.bit0 wrestlers. Keep them in
     * CPU state7 and F4C2-suppressed via +$32.bit7 so this fixture exercises
     * the real scheduler without fabricating action callbacks. */
    for(unsigned i=0u;i<2u;i++){
        wf_set_u8(&rt.actors[i],WF_A_ACTIVE,0x80u);
        wf_set_be32(&rt.actors[i],WF_A_INPUT_SOURCE,0x140020u+i*2u);
        wf_set_u8(&rt.actors[i],WF_A_FLAGS32,0x80u);
        wf_set_u8(&rt.actors[i],WF_A_FLAGS33,0x01u);
        wf_set_u8(&rt.actors[i],WF_A_STATE+1u,0x07u);
        wf_set_be16(&rt.actors[i],WF_A_X,(uint16_t)(0x0200u+i*0x20u));
        wf_set_be16(&rt.actors[i],WF_A_Y,0x0180u);
        c.input_valid_mask=(uint16_t)(c.input_valid_mask|(1u<<i));
        c.input_hw_low[i]=0xffu;
    }
    wf_r17_referee_init_10718(&rt);
    wf_r17_fixed_four_init_1004c(&rt);

    assert(wf_runtime_frame(&rt)==WF_BLOCKED);
    if(rt.trace_count!=29u){
        fprintf(stderr,"blocked trace_count=%zu last=%06X dep=%06X\n",rt.trace_count,
            rt.trace_count?rt.trace[rt.trace_count-1]:0u,c.last_blocked_dependency);
        return 2;
    }
    assert(rt.trace[28]==0x0298ecu);
    assert(rt.frame_ready==1u&&rt.frame_stage==1u);
    assert((rt.g.frame_handshake[1]&0x01u)!=0u);
    rng_after_frame0=rt.g.rng_state;

    wf_runtime_vblank_ack(&rt);
    assert(wf_runtime_frame(&rt)==WF_OK);
    assert(rt.g.frame_counter==1u);
    assert(rt.g.pipeline_mode==1u&&rt.g.global_0082==0u); /* same ROM LONG */
    assert(rt.frame_stage==0u&&rt.frame_ready==0u);
    assert(rt.g.rng_state==rng_after_frame0); /* post-ACK itself must not call RNG */

    /* A new iteration must execute the mandatory $FA0->$21B4 again. */
    assert(wf_runtime_frame(&rt)==WF_BLOCKED);
    assert(rt.g.rng_state!=rng_after_frame0);

    puts("PASS branch A real bindings: 29 owners -> VBlank ACK -> post-ACK -> next $FA0 RNG");
    return 0;
}
