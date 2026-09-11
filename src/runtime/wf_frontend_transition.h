#ifndef WF_FRONTEND_TRANSITION_H
#define WF_FRONTEND_TRANSITION_H

#include <stdbool.h>
#include <stdint.h>

#define WF_FRONTEND_FADE_LEVELS 8u
#define WF_FRONTEND_FADE_VBLANKS_PER_LEVEL 3u
#define WF_FRONTEND_FADE_TOTAL_VBLANKS 24u
#define WF_FRONTEND_STRIPE_LEVELS 8u
#define WF_FRONTEND_STRIPE_VBLANKS_PER_LEVEL 3u
#define WF_FRONTEND_STRIPE_TOTAL_VBLANKS 24u
#define WF_FRONTEND_STRIPE_TILE_FIRST 0x04BEu
#define WF_FRONTEND_STRIPE_TILE_LAST 0x04C5u
#define WF_FRONTEND_STRIPE_PALETTE_BANK 0x0Fu
#define WF_FRONTEND_WAIT_33 33u
#define WF_FRONTEND_WAIT_17 17u
#define WF_FRONTEND_WAIT_257 257u
#define WF_FRONTEND_BOOT_HOLD 128u
#define WF_FRONTEND_ATTRACT_POSTDEMO_TOTAL 90u
#define WF_FRONTEND_SELECT_SLIDE_UPDATES 26u
#define WF_FRONTEND_SELECT_SLIDE_STEP 8u
#define WF_FRONTEND_SELECT_LEFT_X 0x0140u
#define WF_FRONTEND_SELECT_RIGHT_X 0x0210u

typedef enum wf_frontend_transition_kind {
    WF_FRONTEND_TRANSITION_NONE = 0,
    WF_FRONTEND_TRANSITION_PALETTE_FADE_BLACK,
    WF_FRONTEND_TRANSITION_STRIPE_CLOSE,
    WF_FRONTEND_TRANSITION_STRIPE_OPEN,
    WF_FRONTEND_TRANSITION_WAIT
} wf_frontend_transition_kind_t;

typedef struct wf_frontend_transition {
    wf_frontend_transition_kind_t kind;
    uint16_t remaining_vblanks;
    uint16_t total_vblanks;
    uint8_t current_level;
    uint8_t level_vblank_index;
    bool active;
} wf_frontend_transition_t;

typedef struct wf_frontend_transition_event {
    bool valid;
    uint8_t fade_level;
    bool stripe_write;
    uint16_t stripe_tile_code;
    uint8_t stripe_palette_bank;
    bool stripe_clear_fg0;
    bool complete;
} wf_frontend_transition_event_t;

uint8_t wf_frontend_fade_map_nibble(uint8_t input_nibble, uint8_t level);
void wf_frontend_transition_begin_fade(wf_frontend_transition_t *rt);
void wf_frontend_transition_begin_stripe_close(wf_frontend_transition_t *rt);
void wf_frontend_transition_begin_stripe_open(wf_frontend_transition_t *rt);
void wf_frontend_transition_begin_wait(wf_frontend_transition_t *rt, uint16_t vblanks);
void wf_frontend_transition_step_vblank(wf_frontend_transition_t *rt,
                                        wf_frontend_transition_event_t *event);
bool wf_frontend_transition_active(const wf_frontend_transition_t *rt);
bool wf_frontend_select_slide_step(uint16_t *scroll_x, bool toward_right);

#endif
