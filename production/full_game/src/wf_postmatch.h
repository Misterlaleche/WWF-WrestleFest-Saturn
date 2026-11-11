#ifndef WF_POSTMATCH_H
#define WF_POSTMATCH_H
#include "wf_session.h"

/* Resume only an acknowledged original TAG/Royal/attract boundary.
 * Caller has restored current raw RAM/video with wf_r18_scene_resume, rebound
 * frontend/match owners, retained live D/A/control state and loaded the actual
 * final picture plus post graphics. No reset, score, input or IRQ is supplied.
 * Original AC0 returns MATCH_READY; 6FC or accepted START returns FRONTEND.
 * TAG/Royal winners use the shared original final1B4E before records/BOOT.
 * Missing owners are terminal FATAL, never skipped.
 * Invalid entry contracts return BAD_STATE without executing a prefix. */
wf_status_t wf_postmatch_run(wf_session_t *);
#endif
