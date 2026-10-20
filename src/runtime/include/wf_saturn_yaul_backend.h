#ifndef WF_SATURN_YAUL_BACKEND_H
#define WF_SATURN_YAUL_BACKEND_H
#include <stddef.h>
#include <stdint.h>
#include "wf_saturn_render_bridge.h"
#include "wf_saturn_memory.h"

typedef struct wf_saturn_yaul_backend {
    uint8_t *hwr_stage;
    size_t hwr_stage_bytes;
    unsigned expected_tiles, expected_palettes, expected_cmds;
    unsigned emitted_tiles, emitted_palettes, emitted_cmds;
    uint8_t active;
    uint8_t defer_sync; /* caller commits VDP1 and VDP2 together before the next frame */
} wf_saturn_yaul_backend_t;

void wf_saturn_yaul_backend_init(wf_saturn_yaul_backend_t *b, void *hwr_stage, size_t hwr_stage_bytes);
const wf_saturn_render_ops_t *wf_saturn_yaul_render_ops(void);

/* Hardware setup/submission exist only in the Saturn build. Host builds retain the contract for audit/tests. */
wf_status_t wf_saturn_yaul_video_init(void);
wf_status_t wf_saturn_yaul_present_plan(wf_saturn_yaul_backend_t *b,const wf_vdp1_frame_plan_t *plan,const wf_vdp1_assets_t *assets);
#endif
