#ifndef WF_SESSION_H
#define WF_SESSION_H
#include "wf_r18_match_driver.h"

typedef enum {
 WF_SESSION_FRONTEND, WF_SESSION_MATCH_READY, WF_SESSION_GAMEPLAY,
 WF_SESSION_FATAL, WF_SESSION_POSTMATCH
} wf_session_phase_t;
typedef struct {
 uint8_t ram[0x4000];
 wf_runtime_t runtime;
 wf_r18_video_t video;
 wf_r18_frontend_driver_t frontend;
 wf_r18_match_driver_t match;
 wf_session_phase_t phase;
 uint32_t error_owner, error_pc;
 unsigned initialized;
} wf_session_t;
typedef struct {
 /* Explicit platform power-on RAM image, including lower$60/RNG. Must not
  * alias the session being initialized. No seed/default credits supplied. */
 const uint8_t *cold_ram;
 size_t cold_ram_bytes;
 uint32_t ssp, usp;
 uint16_t sr;
 wf_r18_frontend_platform_t platform;
 wf_phase_binding_t *bindings;
 size_t binding_count;
} wf_session_config_t;

/* Callbacks own actual board/graphics events. All required by the original
 * driver remain mandatory; no fallback success or fabricated IRQ is added. */
wf_status_t wf_session_init(wf_session_t *, const wf_session_config_t *);
wf_status_t wf_session_step(wf_session_t *);
/* Synchronous original AC0→F8C. On credited START diversion during attract,
 * return OK with FRONTEND phase and the new generation intact. On failure,
 * retain a terminal FATAL state; callers must not replay partial effects. */
wf_status_t wf_session_prepare_match(wf_session_t *);
/* Bind existing match/leaf owners to restored frontend metadata without
 * executing AC0. Used before the original post-combat owner is resumed. */
wf_status_t wf_session_bind_match(wf_session_t *);
#endif
