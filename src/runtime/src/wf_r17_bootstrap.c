#include "wf_r17_bootstrap.h"
static void trace_clear(wf_r17_f8c_trace_t *tr){if(!tr)return;tr->rng_before=0u;tr->rng_after=0u;tr->d0_before=0u;tr->d0_after=0u;tr->rng_calls=0u;tr->branch=0u;}
wf_status_t wf_r17_f8c_enter(wf_r17_f8c_state_t *s,uint32_t *rng,wf_r17_m68k_shadow_t *regs,wf_r17_f8c_trace_t *tr){
    uint32_t out;trace_clear(tr);if(!s||!rng||!regs||!regs->valid)return WF_BLOCKED;
    /* $F8C BSET #7,$1C0076 is a memory-byte bit op at the high byte of this WORD. */
    s->control_1c0076=(uint16_t)(s->control_1c0076|0x8000u);
    /* $F94/$F9A */
    s->counter_1c0080=0u;s->sync_1c006e=0u;
    if(tr){tr->rng_before=*rng;tr->d0_before=regs->d[0];}
    out=wf_r17_rng21b4_live(rng,s->counter_1c0080,regs);
    if(tr){tr->rng_after=out;tr->d0_after=regs->d[0];tr->rng_calls=1u;tr->branch=(uint8_t)(s->branch_1c0083&1u);}
    return WF_OK;
}
int wf_r17_attract_timeout_after_increment(uint32_t counter,uint16_t threshold){return counter>(uint32_t)threshold;}
