#ifndef WF_VISUAL_H
#define WF_VISUAL_H
#include <stdint.h>
#include "wf_runtime.h"
#include "wf_animation.h"
#include "wf_camera.h"

typedef struct wf_actor_visual {
    wf_anim_view_t anim;
    uint8_t valid;
    uint8_t domain;
    uint8_t selector;
    uint16_t pose;
    /* Intermediate arcade $247C anchors (+$14/+16), never final raster X/Y. */
    int16_t screen_x, screen_y;
} wf_actor_visual_t;

typedef struct wf_visual_state {
    wf_actor_visual_t actor[WF_ACTOR_SLOTS];
    wf_camera_state_t camera;
} wf_visual_state_t;

void wf_visual_init(wf_visual_state_t *v);
wf_status_t wf_visual_actor_step(wf_runtime_t *rt,wf_visual_state_t *v,unsigned slot);
wf_status_t wf_visual_build_screen_positions(wf_runtime_t *rt,wf_visual_state_t *v);

#endif
