#ifndef WF_HAND_PROMPT_H
#define WF_HAND_PROMPT_H

#include <stdbool.h>
#include <stdint.h>

#define WF_HAND_PROMPT_POSES 2u
#define WF_HAND_PROMPT_WIDTH 80u
#define WF_HAND_PROMPT_HEIGHT 96u
#define WF_HAND_PROMPT_BYTES (WF_HAND_PROMPT_WIDTH * WF_HAND_PROMPT_HEIGHT)
#define WF_HAND_PROMPT_ORIGIN_X 0
#define WF_HAND_PROMPT_ORIGIN_Y 120
#define WF_HAND_PROMPT_CAPTURE_FG_PALETTE 14u
#define WF_HAND_PROMPT_MODEL_PALETTE_INDEX 16u
#define WF_HAND_PROMPT_FLAG_SUPPRESS 0x01u
#define WF_HAND_PROMPT_FLAG_CLEAR 0x40u
#define WF_HAND_PROMPT_FLAG_REQUEST 0x80u
#define WF_HAND_PROMPT_TEXT_BASE 0x000C0000u
#define WF_HAND_PROMPT_TEXT_START 0x000C1008u
#define WF_HAND_PROMPT_TEXT_PALETTE 14u
#define WF_HAND_PROMPT_ROW_COUNT 13u
#define WF_HAND_PROMPT_WRITE_COUNT 78u
#define WF_HAND_PROMPT_SATURN_EQUIV_PRIORITY 4u

typedef enum wf_hand_prompt_pose {
    WF_HAND_PROMPT_FINGER_UP = 0,
    WF_HAND_PROMPT_FINGER_DOWN = 1
} wf_hand_prompt_pose_t;

typedef struct wf_hand_prompt_frame_meta {
    wf_hand_prompt_pose_t pose;
    uint16_t width, height;
    uint16_t bbox_x0, bbox_y0, bbox_x1, bbox_y1;
    int16_t origin_x, origin_y;
    uint8_t capture_fg_palette;
    uint8_t model_palette_index;
    const char *idx8_sha256;
} wf_hand_prompt_frame_meta_t;

typedef enum wf_hand_prompt_pass_kind {
    WF_HAND_PROMPT_PASS_NONE = 0,
    WF_HAND_PROMPT_PASS_DRAW,
    WF_HAND_PROMPT_PASS_CLEAR
} wf_hand_prompt_pass_kind_t;

typedef struct wf_hand_prompt_state {
    uint16_t cadence_counter;
    uint16_t pose_word;
    uint8_t previous_flags;
} wf_hand_prompt_state_t;

typedef struct wf_hand_prompt_tile_write {
    uint32_t source_text_address;
    uint16_t tile_code;
    uint8_t palette;
    bool clear;
} wf_hand_prompt_tile_write_t;

typedef struct wf_hand_prompt_output {
    wf_hand_prompt_pass_kind_t kind;
    wf_hand_prompt_pose_t pose;
    uint16_t write_count;
    wf_hand_prompt_tile_write_t writes[WF_HAND_PROMPT_WRITE_COUNT];
} wf_hand_prompt_output_t;

extern const wf_hand_prompt_frame_meta_t wf_hand_prompt_frames[WF_HAND_PROMPT_POSES];

void wf_hand_prompt_reset(wf_hand_prompt_state_t *state);
bool wf_hand_prompt_step(wf_hand_prompt_state_t *state,
                         uint8_t gameplay_current_flags,
                         wf_hand_prompt_output_t *out);

#endif
