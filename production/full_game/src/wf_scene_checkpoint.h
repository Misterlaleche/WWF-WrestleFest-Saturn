#ifndef WF_SCENE_CHECKPOINT_H
#define WF_SCENE_CHECKPOINT_H
#include "wf_r18_runtime_handoff.h"
/* Capture the live producers while the combat image owns them. The original
 * typed-to-raw export is deferred until the frontend image is resident. */
wf_status_t wf_scene_checkpoint_capture(const wf_runtime_t *,uint8_t *,size_t,
 wf_r18_video_t *,const wf_r18_world_ctx_t *,const wf_r18_sprite_record_t *,size_t,const uint16_t [6]);
wf_status_t wf_scene_checkpoint_restore(const wf_runtime_t *,uint8_t *,size_t,wf_r18_video_t *);
#endif
