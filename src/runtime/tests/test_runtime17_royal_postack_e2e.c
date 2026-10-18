#include <assert.h>
#include <stdio.h>
#include "wf_gameplay_bindings.h"
/* Start at the genuine wait boundary; the next frame uses production bindings,
 * and may block later on missing external renderer/scene owners. */
static void init(wf_runtime_t*r,wf_gameplay_binding_context_t*c,wf_phase_binding_t*b){size_t n=wf_gameplay_bindings_build(c,b,40);assert(n==36);wf_runtime_init(r,b,n);r->g.rng_state_valid=1;r->g.rng_state=0x13579bdf;r->g.credited_route=1;r->g.mode_0161=1;r->frame_stage=1;r->frame_ready=1;r->g.frame_handshake[1]=1;}
int main(void){wf_runtime_t r;wf_gameplay_binding_context_t c;wf_phase_binding_t b[40];uint32_t rng;wf_r17_m68k_shadow_t expected;
 init(&r,&c,b);r.g.royal_slots_1586[3]=1;
 assert(wf_runtime_frame(&r)==WF_BLOCKED&&r.g.frame_counter==0&&r.trace_count==0);
 wf_runtime_vblank_ack(&r);assert(wf_runtime_frame(&r)==WF_OK);assert(r.g.frame_counter==1&&r.frame_stage==0&&r.frame_ready==0&&r.trace_count==1&&r.trace[0]==0x2017a);
 rng=r.g.rng_state;expected=r.m68k_regs;wf_r17_rng21b4_live(&rng,1,&expected);
 assert(wf_runtime_frame(&r)==WF_BLOCKED);assert(r.g.rng_state==rng&&r.g.frame_handshake[0]==0&&r.g.frame_handshake[1]==0);
 init(&r,&c,b);r.g.global_16c4=0x100;r.g.royal_slots_1586[3]=1;wf_runtime_vblank_ack(&r);assert(wf_runtime_frame(&r)==WF_BLOCKED&&r.frame_stage==2&&r.g.frame_counter==1);assert(wf_runtime_frame(&r)==WF_BLOCKED&&r.g.frame_counter==1);
 init(&r,&c,b);r.g.global_16c4=1;wf_runtime_vblank_ack(&r);assert(wf_runtime_frame(&r)==WF_BLOCKED&&r.g.global_16c4==3&&r.frame_stage==2&&r.g.frame_counter==1);
 puts("PASS Royal wait -> ACK -> $10F2/$2017A -> $1172/$F9A -> next $FA0 RNG; $1B4E/$1AFC blocked");return 0;}
