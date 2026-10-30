#ifndef WF_R18_MATCH_DRIVER_H
#define WF_R18_MATCH_DRIVER_H
#include "wf_r18_frontend_driver.h"
#include "wf_r18_match_flow.h"
typedef wf_status_t (*wf_r18_match_extension_fn)(void *,uint32_t,wf_r18_prematch_t *);
typedef struct wf_r18_match_driver {
 wf_r18_frontend_driver_t *frontend;
 wf_r18_prematch_t machine;
 wf_r18_match_flow_t flow;
 wf_r18_match_extension_fn extension;void *user;
 uint32_t blocked_dependency,blocked_pc;
 uint32_t generation;
} wf_r18_match_driver_t;
/* Compose original AC0 through card7B70, setupC98, A654, C6C and8AE6.
 * Shared D/rawRAM/video remain authoritative; A registers are synchronized
 * around every IRQ wait and external owner. No default selection, clock,
 * input or success is supplied. Missing extension owners stop immediately. */
/* A credited START may discard an uncredited attract call stack through
 * the original IRQ diversion. A changed frontend.generation cancels this
 * cursor with WF_BLOCKED and zero blocked_dependency; the outer owner must
 * resume the frontend's reported phase, never retry the abandoned prefix. */
wf_status_t wf_r18_match_driver_init(wf_r18_match_driver_t *,wf_r18_frontend_driver_t *,wf_r18_match_extension_fn,void *);
wf_status_t wf_r18_match_driver_owner(wf_r18_match_driver_t *,uint32_t);
wf_status_t wf_r18_match_driver_run(wf_r18_match_driver_t *);
#endif
