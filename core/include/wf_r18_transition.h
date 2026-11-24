#ifndef WF_R18_TRANSITION_H
#define WF_R18_TRANSITION_H
#include "wf_r18_display.h"
#include "wf_r18_world.h"
#include "wf_r17_contract.h"
#include "wf_camera.h"
typedef int (*wf_r18_transition_size_fn)(const char *,uint32_t *);
typedef int (*wf_r18_transition_read_fn)(const char *,uint32_t,void *,size_t);
/* The binding user remains the canonical video pointer. All buffers are
 * borrowed; scene_blob must not overlap the ROM cache or video staging. */
typedef struct wf_r18_transition_context {
 wf_r18_video_t *video;
 wf_r18_world_ctx_t *world;
 wf_r18_display_t *display;
 wf_camera_state_t *camera;
 wf_r17_render_queue_t *render_queue;
 void *scene_blob;size_t scene_capacity;
 wf_r18_transition_size_fn file_size;
 wf_r18_transition_read_fn file_read;
 uint8_t *priority_140010;
 uint16_t sprite_control_140008;
 uint32_t blocked_pc;
 uint8_t unblank_pending;
} wf_r18_transition_context_t;
void wf_r18_transition_bind(wf_r18_transition_context_t *);
wf_status_t wf_r18_transition_reset(wf_runtime_t *,uint8_t,void *);
wf_status_t wf_r18_transition_barrier(wf_runtime_t *,uint8_t,void *);
#endif
