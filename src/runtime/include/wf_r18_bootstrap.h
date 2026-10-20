#ifndef WF_R18_BOOTSTRAP_H
#define WF_R18_BOOTSTRAP_H
#include "wf_runtime.h"
#include "wf_r17_post7a00.h"
/* Exact setup work area; caller supplies real selected-team/history state.
 * This owner does not claim to implement the entire $C98 -> $F8C route. */
typedef struct wf_r18_bootstrap_context {
 uint16_t history_0598[12]; /* includes possible pair write at +20 */
 uint16_t roster_0380[12];
 uint16_t replay_008c;
 uint32_t blocked_pc;
 uint8_t priority_140010;
 wf_status_t (*map_26e66)(wf_runtime_t *,void *);
 wf_status_t (*palette_reset)(wf_runtime_t *,void *);
 wf_status_t (*palette_release_2b58)(wf_runtime_t *,uint16_t,void *);
 wf_status_t (*palette_2aea)(wf_runtime_t *,uint16_t,void *);
 wf_status_t (*text_owner)(wf_runtime_t *,uint32_t owner_pc,void *);
 wf_r17_fg0_batch_fn video_batch;
 wf_r17_fg0_batch_fn fg0_batch;
 void *user;
} wf_r18_bootstrap_context_t;
wf_status_t wf_r18_tag_setup_1086c(wf_runtime_t *,wf_r18_bootstrap_context_t *);
wf_status_t wf_r18_live_finalize_8ae6(wf_runtime_t *,wf_r18_bootstrap_context_t *);
typedef struct wf_r18_launch_config {
 uint8_t human,partner;
 uint16_t cabinet_config;
 uint32_t rng_state;
 wf_r17_m68k_shadow_t registers;
} wf_r18_launch_config_t;
/* Stage-zero TAG direct launch; menu selection is explicit platform policy. */
wf_status_t wf_r18_direct_boot(wf_runtime_t *,wf_r18_bootstrap_context_t *,const wf_r18_launch_config_t *);
#endif
