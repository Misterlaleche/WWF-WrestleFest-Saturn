#ifndef WF_AUDIO2_CADENCE_H
#define WF_AUDIO2_CADENCE_H
#include <stdint.h>
typedef struct {
    uint32_t initialized,first_frame,first_vblank,last_frame,last_vblank;
    uint32_t logical_steps,elapsed_vblanks,updates,unobserved_steps,max_gap,regressions;
    uint32_t gaps[32];
} wf_audio2_cadence_t;
static inline void wf_audio2_cadence_tick(volatile wf_audio2_cadence_t *p,uint32_t frame,uint32_t vblank)
{
    if(!p->initialized){
        p->first_frame=p->last_frame=frame;p->first_vblank=p->last_vblank=vblank;
        p->initialized=1;return;
    }
    if(frame==p->last_frame)return;
    if(frame<p->last_frame){p->regressions++;return;}
    uint32_t delta=frame-p->last_frame,gap=vblank-p->last_vblank;
    p->last_frame=frame;p->last_vblank=vblank;p->elapsed_vblanks=vblank-p->first_vblank;
    p->updates++;p->unobserved_steps+=delta-1u;p->gaps[gap<31u?gap:31u]++;
    if(gap>p->max_gap)p->max_gap=gap;
    /* Publish the stop-watch field last, after the complete observation. */
    p->logical_steps+=delta;
}
#endif
