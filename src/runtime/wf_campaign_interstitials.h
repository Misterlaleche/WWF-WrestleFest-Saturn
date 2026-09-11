#ifndef WF_CAMPAIGN_INTERSTITIALS_H
#define WF_CAMPAIGN_INTERSTITIALS_H

#include <stdbool.h>
#include <stdint.h>

#define WF_CAMPAIGN_B608_ROM 0x0000B608u
#define WF_CAMPAIGN_BDA6_ROM 0x0000BDA6u
#define WF_CAMPAIGN_NEXT_PREMATCH_ROM 0x00000AC0u
#define WF_CAMPAIGN_BDA6_SYNC_ITERATIONS 142u
#define WF_CAMPAIGN_FADE_VBLANKS 24u
#define WF_CAMPAIGN_BDA6_ACTION_PHASES 5u
#define WF_CAMPAIGN_BDA6_ACTION_RELOAD 10u
#define WF_CAMPAIGN_BDA6_CARD_RELOAD 8u
#define WF_CAMPAIGN_BDA6_CARD_TERMINAL 16u
#define WF_CAMPAIGN_BDA6_IMPACT_COMMAND 0x312Au

typedef enum wf_campaign_stage {
    WF_CAMPAIGN_STAGE_B608_GATE = 0,
    WF_CAMPAIGN_STAGE_B608_WAIT,
    WF_CAMPAIGN_STAGE_BDA6_GATE,
    WF_CAMPAIGN_STAGE_BDA6_WAIT,
    WF_CAMPAIGN_STAGE_INCREMENT,
    WF_CAMPAIGN_STAGE_PREMATCH_HANDOFF,
    WF_CAMPAIGN_STAGE_DONE
} wf_campaign_stage_t;

typedef struct wf_campaign_postwin {
    uint8_t mode_0161;
    uint16_t selector_0162;
    uint8_t progression_0163;
    uint8_t auxiliary_0165;
    uint16_t config_0066;
    wf_campaign_stage_t stage;
} wf_campaign_postwin_t;

typedef struct wf_campaign_event {
    bool run_b608;
    bool run_bda6;
    uint8_t bda6_normalized_index;
    uint8_t bda6_base_frame;
    bool progression_incremented;
    bool prematch_handoff;
    uint32_t prematch_rom;
} wf_campaign_event_t;

typedef struct wf_bda6_action_event {
    bool valid;
    uint8_t frame;
    uint8_t reload_count;
    bool activate_card;
    bool audio_command;
    uint16_t audio_command_word;
    bool clear_action_active;
} wf_bda6_action_event_t;

typedef struct wf_bda6_card_state {
    uint8_t base_frame;
    uint8_t frame;
    uint16_t counter_24;
} wf_bda6_card_state_t;

typedef struct wf_bda6_card_event {
    bool valid;
    uint8_t reload_count;
    uint16_t counter_24;
    uint8_t frame;
    bool frame_advanced;
    bool terminal;
} wf_bda6_card_event_t;

typedef struct wf_bda6_sync {
    uint16_t iterations;
    uint8_t fade_remaining;
    bool loop_complete;
    bool done;
} wf_bda6_sync_t;

bool wf_campaign_b608_should_run(uint8_t mode_0161, uint16_t selector_0162);
bool wf_campaign_bda6_select(uint8_t mode_0161,
                             uint16_t selector_0162,
                             uint16_t config_0066,
                             uint8_t *normalized_index,
                             uint8_t *base_frame);
void wf_campaign_postwin_begin(wf_campaign_postwin_t *rt,
                               uint8_t mode_0161,
                               uint16_t selector_0162,
                               uint8_t progression_0163,
                               uint8_t auxiliary_0165,
                               uint16_t config_0066);
void wf_campaign_postwin_step(wf_campaign_postwin_t *rt,
                              bool current_scene_complete,
                              wf_campaign_event_t *event);

bool wf_bda6_action_expiry(unsigned phase, wf_bda6_action_event_t *event);
void wf_bda6_card_begin(wf_bda6_card_state_t *state, uint8_t base_frame);
bool wf_bda6_card_expiry(wf_bda6_card_state_t *state,
                         wf_bda6_card_event_t *event);
void wf_bda6_sync_begin(wf_bda6_sync_t *sync);
void wf_bda6_sync_iteration(wf_bda6_sync_t *sync);
void wf_bda6_sync_fade_vblank(wf_bda6_sync_t *sync);

#endif
