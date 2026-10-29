#ifndef WF_R18_FRONTEND_PALETTE_H
#define WF_R18_FRONTEND_PALETTE_H
#include "wf_r18_video.h"

/* Original $2A06 palette payload: world0..6 and text configuration0..3.
 * The platform owns the enclosing VBlank-high/low synchronization. Reads
 * exact ROM pointers, preserves arcade128-byte bank stride and live D0..D7.
 * Sprite resource allocation is separate ($2AEA). */
wf_status_t wf_r18_frontend_palette_2a06(wf_runtime_t *,wf_r18_video_t *,
                                      uint16_t world,uint16_t text);
#endif
