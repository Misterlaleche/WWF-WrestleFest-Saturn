#ifndef WF_FINALE_BOUNDARY_H
#define WF_FINALE_BOUNDARY_H
#include "wf_runtime.h"
/* Original Royal post-ACK branch $115C->$1B4E. The winner bit is in the
 * high byte of $16C4; the low-byte value4 only starts the winner animation.
 * Export only after the same ready/ACK boundary as the original loss path. */
static inline int wf_royal_finale_ready(const wf_runtime_t *r){
 return r&&r->frame_stage==2u&&r->frame_ready&&r->vblank_ack&&
  r->g.credited_route&&(r->g.mode_0161&1u)&&(r->g.global_16c4>>8)!=0u;
}
#endif
