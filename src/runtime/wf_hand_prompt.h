#ifndef WF_HAND_PROMPT_H
#define WF_HAND_PROMPT_H
#include <stdbool.h>
#include <stdint.h>

#define WF_HAND_PROMPT_POSES 2u
#define WF_HAND_PROMPT_WIDTH 80u
#define WF_HAND_PROMPT_HEIGHT 96u
#define WF_HAND_PROMPT_BYTES (WF_HAND_PROMPT_WIDTH*WF_HAND_PROMPT_HEIGHT)
#define WF_HAND_PROMPT_ORIGIN_X 0
#define WF_HAND_PROMPT_ORIGIN_Y 120
#define WF_HAND_PROMPT_CAPTURE_FG_PALETTE 14u
#define WF_HAND_PROMPT_MODEL_PALETTE_INDEX 16u

typedef enum wf_hand_prompt_pose {
    WF_HAND_PROMPT_FINGER_UP=0,
    WF_HAND_PROMPT_FINGER_DOWN=1
} wf_hand_prompt_pose_t;

typedef struct wf_hand_prompt_frame_meta {
    wf_hand_prompt_pose_t pose;
    uint16_t width,height;
    uint16_t bbox_x0,bbox_y0,bbox_x1,bbox_y1;
    int16_t origin_x,origin_y;
    uint8_t capture_fg_palette;
    uint8_t model_palette_index;
    const char *idx8_sha256;
} wf_hand_prompt_frame_meta_t;

typedef struct wf_hand_prompt_visual {
    wf_hand_prompt_pose_t pose;
    bool visible;
} wf_hand_prompt_visual_t;

extern const wf_hand_prompt_frame_meta_t wf_hand_prompt_frames[WF_HAND_PROMPT_POSES];

bool wf_hand_prompt_select_pose(wf_hand_prompt_visual_t *visual,wf_hand_prompt_pose_t pose);
void wf_hand_prompt_hide(wf_hand_prompt_visual_t *visual);

#endif
