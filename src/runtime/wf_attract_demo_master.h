#ifndef WF_ATTRACT_DEMO_MASTER_H
#define WF_ATTRACT_DEMO_MASTER_H

#include <stdbool.h>
#include <stdint.h>

#define WF_ATTRACT_DEMO_STATE_COUNT 14u
#define WF_ATTRACT_DEMO_MAX_ACTORS 6u
#define WF_ATTRACT_DEMO_STATE_LAST 0x0Du
#define WF_ATTRACT_DEMO_NO_BIO 0xFFu
#define WF_ATTRACT_DEMO_FADE_VBLANKS 24u
#define WF_ATTRACT_DEMO_WAIT_A_VBLANKS 33u
#define WF_ATTRACT_DEMO_WAIT_B_VBLANKS 33u
#define WF_ATTRACT_DEMO_TRANSITION_VBLANKS 90u
#define WF_ATTRACT_DEMO_FRONTEND_RETURN_ROM 0x000006FCu
#define WF_ATTRACT_DEMO_CREDITED_START_ROM 0x000052BEu

#define WF_ATTRACT_SETUP_SPECIAL_01_02 0x01u
#define WF_ATTRACT_SETUP_PREHOOK_8490 0x02u

typedef enum wf_attract_demo_mode {
    WF_ATTRACT_DEMO_TAG = 0,
    WF_ATTRACT_DEMO_RUMBLE = 1
} wf_attract_demo_mode_t;

typedef enum wf_attract_override_policy {
    WF_ATTRACT_OVERRIDE_STATE0_SPECIAL = 0,
    WF_ATTRACT_OVERRIDE_DISABLED_RUMBLE,
    WF_ATTRACT_OVERRIDE_CHARACTER_12
} wf_attract_override_policy_t;

typedef struct wf_attract_demo_config {
    uint8_t state;
    uint16_t threshold_updates;
    wf_attract_demo_mode_t mode;
    uint8_t actor_count;
    uint8_t lineup[WF_ATTRACT_DEMO_MAX_ACTORS];
    uint8_t bio_featured_id;
    wf_attract_override_policy_t override_policy;
    uint8_t setup_flags;
} wf_attract_demo_config_t;

typedef enum wf_attract_demo_phase {
    WF_ATTRACT_DEMO_GAMEPLAY = 0,
    WF_ATTRACT_DEMO_FADE_24,
    WF_ATTRACT_DEMO_WAIT_33_A,
    WF_ATTRACT_DEMO_WAIT_33_B,
    WF_ATTRACT_DEMO_HANDOFF
} wf_attract_demo_phase_t;

typedef struct wf_attract_demo_runtime {
    const wf_attract_demo_config_t *config;
    uint32_t gameplay_updates;
    wf_attract_demo_phase_t phase;
    uint16_t phase_vblanks_remaining;
} wf_attract_demo_runtime_t;

typedef struct wf_attract_demo_event {
    bool threshold_reached;
    bool begin_palette_fade;
    bool next_attract_state;
    uint8_t next_state;
    bool frontend_return;
    uint32_t frontend_return_rom;
} wf_attract_demo_event_t;

const wf_attract_demo_config_t *wf_attract_demo_config(unsigned state);
bool wf_attract_demo_begin(wf_attract_demo_runtime_t *rt, unsigned state);
void wf_attract_demo_gameplay_update(wf_attract_demo_runtime_t *rt,
                                     wf_attract_demo_event_t *event);
void wf_attract_demo_transition_vblank(wf_attract_demo_runtime_t *rt,
                                       wf_attract_demo_event_t *event);
bool wf_attract_demo_is_live_gameplay(const wf_attract_demo_runtime_t *rt);
bool wf_attract_demo_credited_start_is_external(void);

#endif
