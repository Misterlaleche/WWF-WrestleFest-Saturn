#ifndef WF_CAMERA_H
#define WF_CAMERA_H
#include <stdint.h>
#include "wf_runtime.h"

typedef struct wf_camera_state {
    int16_t scroll0_x, scroll0_y;
    int16_t scroll1_x, scroll1_y;
    int16_t target_x, target_y;
    uint16_t scene_index;
    uint16_t shake_v, shake_h;
} wf_camera_state_t;

wf_status_t wf_camera_track(wf_runtime_t *rt, wf_camera_state_t *c);
wf_status_t wf_camera_clamp(wf_camera_state_t *c);
void wf_world_to_screen(const wf_actor_raw_t *a,const wf_camera_state_t *c,int16_t *sx,int16_t *sy);

#endif
