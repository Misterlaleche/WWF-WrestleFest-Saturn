#ifndef WF_R18_RUNTIME_HANDOFF_H
#define WF_R18_RUNTIME_HANDOFF_H
#include "wf_runtime.h"
#include "wf_r18_video.h"
#include "wf_r18_world.h"
/* Import the proven frontend's original RAM into the typed combat view.
 * Bindings/trace ownership remains with the caller; no selection or seed is
 * substituted. Compact buffers are copied by their original record spans.
 * The caller must have initialized the runtime once before frontend entry. */
wf_status_t wf_r18_runtime_import(wf_runtime_t *,const uint8_t *,size_t,
                                  const wf_r17_m68k_shadow_t *);
/* Literal F8C..FA4 on shared raw RAM, including CLR.L80 and its byte82/83
 * aliases, followed by import. Calls original RNG once and hands execution
 * to FA4 (frame_stage3), preserving selected actors, mode and paid state. */
wf_status_t wf_r18_runtime_handoff_f8c(wf_runtime_t *,uint8_t *,size_t,
                                     wf_r17_m68k_shadow_t *);
/* Return current combat fields to shared raw RAM before a scene controller.
 * Does not reset runtime, credits, RNG, frame, participants or bindings.
 * Keeps unrepresented raw bytes; the caller must retain their persistent
 * contents separately while the raw-RAM lease is used by combat caches.
 * Named LONG/WORD fields own their byte aliases (80,7E,169A); named Royal
 * slots/15CE own their overlap with selector1576. Export after palette-map
 * bytes so overlapping gameplay globals remain current. */
wf_status_t wf_r18_runtime_export(const wf_runtime_t *,uint8_t *,size_t);
/* Restore the retained complete arcade RAM image before resuming a scene.
 * Then overlay current allocator, typed globals, world caches and original
 * sprite prefix in that alias order. Requires drained render/animation
 * queues. Never initializes cabinet state or manufactures a match result.
 * Invalid/pending contracts leave RAM and video untouched. */
wf_status_t wf_r18_scene_resume(const wf_runtime_t *,uint8_t *,size_t,
 const uint8_t *retained,size_t,wf_r18_video_t *,const wf_r18_world_ctx_t *,
 const wf_r18_sprite_record_t *,size_t,const uint16_t render_counts[6]);
#endif
