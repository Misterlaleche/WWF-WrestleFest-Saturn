#ifndef WF_R17_REFEREE_H
#define WF_R17_REFEREE_H
#include <stdint.h>
#include "wf_runtime.h"

typedef wf_status_t (*wf_r17_ref_display_fn)(wf_runtime_t *rt,void *user);
typedef wf_status_t (*wf_r17_ref_simple_fn)(wf_runtime_t *rt,void *user);
typedef wf_status_t (*wf_r17_ref_actor_fn)(wf_runtime_t *rt,wf_actor_raw_t *actor,void *user);
typedef wf_status_t (*wf_r17_ref_audio_fn)(wf_runtime_t *rt,uint16_t command,void *user);
typedef wf_status_t (*wf_r17_ref_word_fn)(wf_runtime_t *rt,uint16_t value,void *user);

typedef struct wf_r17_referee_ctx {
 wf_r17_ref_display_fn display_2067c;
 wf_r17_ref_simple_fn clear_206fe;
 wf_r17_ref_actor_fn direction_20c8;
 wf_r17_ref_actor_fn resolver_280dc;
 wf_r17_ref_actor_fn royal_phase6_21358; /* optional override; original A0 is referee */
 wf_r17_ref_simple_fn tag_phase6_90d6;
 wf_r17_ref_simple_fn result_20156;
 wf_r17_ref_audio_fn audio_2052;
 wf_r17_ref_word_fn number_2067c;
 wf_r17_ref_word_fn text_2503c;
 void *user;
 uint32_t blocked_dependency_rom;
} wf_r17_referee_ctx_t;

extern const uint32_t wf_r17_ref_primary_table_1f952[11];
extern const uint32_t wf_r17_ref_action_table_1fb1a[11];

void wf_r17_referee_init_10718(wf_runtime_t *rt);
void wf_r17_raw48_pin_trigger_133a2(wf_actor_raw_t *wrestler);
wf_status_t wf_r17_ref_acquire_20556(wf_runtime_t *rt,wf_actor_raw_t *referee,int *found);
wf_status_t wf_r17_referee_logic_1f914(wf_runtime_t *rt,wf_actor_raw_t *referee,void *user);
struct wf_r17_render_ctx;
wf_status_t wf_r17_royal_f8e4(wf_runtime_t *rt,wf_actor_raw_t *referee,struct wf_r17_render_ctx *render);
#endif
