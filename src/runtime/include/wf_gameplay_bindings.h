#ifndef WF_GAMEPLAY_BINDINGS_H
#define WF_GAMEPLAY_BINDINGS_H
#include "wf_cpu_runtime.h"
#include "wf_input.h"
#include "wf_camera.h"
#include "wf_r17_contract.h"
#include "wf_r17_referee.h"
#include "wf_r17_postack_2017a.h"
#include "wf_r17_actions_r11.h"
#include "wf_r17_dispatch_1c03e.h"
#include "wf_r17_collision_24062.h"
#include "wf_r17_ringside_fdee.h"
#include "wf_r17_fixed_four_10120.h"
#include "wf_r17_transient_reset_10e6a.h"
#include "wf_r17_effects_7a00.h"
#include "wf_r17_post7a00.h"
#include "wf_r17_participant_io.h"

typedef wf_status_t (*wf_r17_platform_transition_fn)(wf_runtime_t *rt,uint8_t royal_path,void *user);
typedef wf_status_t (*wf_r17_audio_command_fn)(wf_runtime_t *rt,uint16_t raw_command,void *user);

typedef struct wf_gameplay_binding_context {
 uint8_t input_hw_low[WF_ACTOR_SLOTS];
 uint16_t input_valid_mask;
 uint16_t cabinet_word_140020;
 uint16_t cabinet_word_140022;
 uint8_t cabinet_words_valid;
 wf_cpu_trace_t cpu_trace[WF_ACTOR_SLOTS];
 wf_camera_state_t camera;
 uint8_t camera_valid;
 wf_r17_platform_transition_fn transition_reset; /* native equivalent of $1F6C/$1FDE */
 wf_r17_platform_transition_fn transition_barrier; /* native sync/transfer sequence */
 wf_r17_audio_command_fn audio_command; /* WP24 transport owner */
 wf_r17_ref_display_fn referee_display_2067c; /* exact event boundary; renderer join remains external */
 wf_r17_ref_simple_fn referee_clear_206fe;
 wf_r17_ref_actor_fn referee_direction_20c8;
 wf_r17_ref_actor_fn referee_resolver_280dc;
 wf_r17_ref_actor_fn referee_royal_phase6_21358;
 wf_r17_ref_simple_fn referee_tag_phase6_90d6;
 wf_r17_ref_simple_fn referee_result_20156;
 wf_r17_render_queue_t render_queue;
 /* Persistent state required by the already-audited R11/raw-$23 action owner. */
 wf_r17_raw23_ctx_t f4c2_raw23_ctx;
 wf_r17_actions_r11_bridge_t f4c2_actions;
 wf_r17_dispatch_1c03e_ctx_t f4c2_dispatch;
 wf_runtime_t *f4c2_active_rt;
 wf_r17_collision_24062_trace_t collision_24062;
 wf_r17_fdee_trace_t ringside_fdee;
 wf_r17_fixed_four_trace_t fixed_four_10120;
 wf_r17_effects_7a00_trace_t effects_7a00;
 /* Post-$7A00 external ownership joins. Gameplay emits raw state/events only. */
 wf_r17_fg0_batch_fn post7_fg0_batch;
 wf_r17_fg0_batch_fn post7_video_batch;
 wf_r17_raw_word_fn post7_resource_2aea;
 wf_r17_raw_word_fn post7_resource_2b58;
 wf_r17_raw_word_fn post7_raw_2503c;
 wf_r17_raw_word_fn post7_wait_21e6;
 wf_r17_raw_9aca_fn post7_raw_9aca;
 wf_r17_fg0_peek_long_fn post7_fg0_peek_long;
 wf_r17_branchb_288f0_fn branchb_288f0;
 wf_r17_branchb_26b68_fn branchb_26b68;
 wf_r17_render_2836_batch_fn post7_render_2836_batch;
 wf_r17_tag_6e34_fn post7_tag_6e34;
 wf_r17_royal_9132_fn post7_royal_9132;
 wf_r17_participant_input_fn participant_input_word;
 wf_r17_participant_commit_fn participant_commit_batch;
 void *r17_owner_user;
 uint32_t last_blocked_phase;
 uint32_t last_blocked_dependency;
} wf_gameplay_binding_context_t;

wf_status_t wf_phase_postack_2017a(wf_runtime_t*,uint32_t,void*);
size_t wf_gameplay_bindings_build(wf_gameplay_binding_context_t *ctx,wf_phase_binding_t *out,size_t cap);
wf_status_t wf_phase_branchb_285da(wf_runtime_t*,uint32_t,void*);
wf_status_t wf_phase_branchb_26a42(wf_runtime_t*,uint32_t,void*);
wf_status_t wf_phase_branchb_7548(wf_runtime_t*,uint32_t,void*);
wf_status_t wf_phase_branchb_bcfe(wf_runtime_t*,uint32_t,void*);
wf_status_t wf_phase_branchb_c04a(wf_runtime_t*,uint32_t,void*);
wf_status_t wf_phase_branchb_c710(wf_runtime_t*,uint32_t,void*);
wf_status_t wf_phase_input_real(wf_runtime_t*,uint32_t,void*);
wf_status_t wf_phase_cpu_real(wf_runtime_t*,uint32_t,void*);
wf_status_t wf_phase_refill_8b3a(wf_runtime_t*,uint32_t,void*);
wf_status_t wf_phase_camera_26936(wf_runtime_t*,uint32_t,void*);
wf_status_t wf_phase_camera_2983c(wf_runtime_t*,uint32_t,void*);
wf_status_t wf_phase_referee_1f914(wf_runtime_t*,uint32_t,void*);
wf_status_t wf_phase_wrestlers_f4c2(wf_runtime_t*,uint32_t,void*);
wf_status_t wf_phase_collision_24062(wf_runtime_t*,uint32_t,void*);
wf_status_t wf_phase_ringside_fdee(wf_runtime_t*,uint32_t,void*);
wf_status_t wf_phase_fixed_four_10120(wf_runtime_t*,uint32_t,void*);
wf_status_t wf_phase_transient_reset_10e6a(wf_runtime_t*,uint32_t,void*);
wf_status_t wf_phase_effects_7a00(wf_runtime_t*,uint32_t,void*);
wf_status_t wf_phase_post7_88a2(wf_runtime_t*,uint32_t,void*);
wf_status_t wf_phase_post7_8710(wf_runtime_t*,uint32_t,void*);
wf_status_t wf_phase_post7_262d2(wf_runtime_t*,uint32_t,void*);
wf_status_t wf_phase_post7_bb14(wf_runtime_t*,uint32_t,void*);
wf_status_t wf_phase_post7_7366(wf_runtime_t*,uint32_t,void*);
wf_status_t wf_phase_post7_8cf8(wf_runtime_t*,uint32_t,void*);
wf_status_t wf_phase_post7_9052(wf_runtime_t*,uint32_t,void*);
wf_status_t wf_phase_post7_899c(wf_runtime_t*,uint32_t,void*);
wf_status_t wf_phase_post7_8ecc(wf_runtime_t*,uint32_t,void*);
wf_status_t wf_phase_post7_2836(wf_runtime_t*,uint32_t,void*);
wf_status_t wf_phase_post7_18c4(wf_runtime_t*,uint32_t,void*);
wf_status_t wf_phase_post7_298ec(wf_runtime_t*,uint32_t,void*);
#endif
