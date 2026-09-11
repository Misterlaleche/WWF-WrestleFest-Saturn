#ifndef WF_CREDITS_H
#define WF_CREDITS_H

#include <stdbool.h>
#include <stdint.h>

#define WF_CREDITS_FIXED_PAGE_COUNT 9u
#define WF_CREDITS_STAFF_BLOCK_COUNT 6u
#define WF_CREDITS_FIXED_HOLD_VBLANKS 129u
#define WF_CREDITS_STAFF_UPDATES 1281u
#define WF_CREDITS_SCROLL_DY_16_16 0x0000C000L
#define WF_CREDITS_SKIP_MASK 0x0030u
#define WF_CREDITS_ENTRY_SOUND_CMD 0x11u
#define WF_CREDITS_EXIT_SOUND_CMD 0x1Fu
#define WF_CREDITS_COMMON_FADE_VBLANKS 24u
#define WF_CREDITS_FRONTEND_RETURN_ROM 0x000006FCu

typedef enum wf_credits_state {
    WF_CREDITS_IDLE = 0,
    WF_CREDITS_ENTRY_FADE,
    WF_CREDITS_FIXED_PAGES,
    WF_CREDITS_STAFF_SCROLL,
    WF_CREDITS_EXIT_FADE,
    WF_CREDITS_FRONTEND_RETURN,
    WF_CREDITS_DONE
} wf_credits_state_t;

typedef struct wf_credits_page {
    uint8_t text_id;
    uint8_t pose;
    uint8_t sprite_type;
    int16_t anchor_x;
    int16_t anchor_y;
    int16_t text_x1;
    int16_t text_y1;
    int16_t text_x2;
    int16_t text_y2;
} wf_credits_page_t;

typedef struct wf_credits_staff_block {
    uint8_t pose;
    uint8_t sprite_type;
    int16_t world_x;
    int16_t world_y;
    int16_t initial_screen_x;
    int16_t initial_screen_y;
    uint8_t palette_resource;
} wf_credits_staff_block_t;

typedef struct wf_credits_events {
    bool setup_background;
    bool load_fixed_page;
    uint8_t fixed_page_index;
    bool setup_staff;
    bool staff_scroll_update;
    int32_t staff_scroll_accum_16_16;
    bool audio_command;
    uint8_t audio_command_byte;
    bool cleanup;
    bool frontend_return;
    uint32_t frontend_return_rom;
} wf_credits_events_t;

typedef struct wf_credits_runtime {
    wf_credits_state_t state;
    uint16_t fade_remaining;
    uint16_t page_hold_remaining;
    uint16_t page_sync_remaining;
    uint16_t staff_updates_remaining;
    uint8_t page_index;
    int32_t staff_scroll_accum_16_16;
    bool exit_command_sent;
} wf_credits_runtime_t;

extern const wf_credits_page_t wf_credits_pages[WF_CREDITS_FIXED_PAGE_COUNT];
extern const wf_credits_staff_block_t wf_credits_staff_blocks[WF_CREDITS_STAFF_BLOCK_COUNT];

void wf_credits_begin(wf_credits_runtime_t *rt);
bool wf_credits_skip_from_raw(const uint16_t player_words_active_low[4]);
void wf_credits_step_vblank(wf_credits_runtime_t *rt,
                            const uint16_t player_words_active_low[4],
                            wf_credits_events_t *events);

#endif
