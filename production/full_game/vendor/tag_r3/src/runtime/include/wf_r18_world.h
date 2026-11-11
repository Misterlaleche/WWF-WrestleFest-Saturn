#ifndef WF_R18_WORLD_H
#define WF_R18_WORLD_H
#include "wf_r18_metasprite.h"
typedef struct wf_r18_world_transport {uint32_t source,destination;} wf_r18_world_transport_t;
typedef struct wf_r18_world_ctx {
 wf_r18_read_rom_fn read_rom;void *read_user;
 uint8_t animation_queue[2][48];uint16_t animation_count[2];
 wf_r18_world_transport_t cache[2][16];
} wf_r18_world_ctx_t;
wf_status_t wf_r18_world_288f0(wf_runtime_t *,uint8_t record[4],uint8_t bank,void *world_ctx);
wf_status_t wf_r18_world_26b68(wf_runtime_t *,uint8_t side,uint16_t direction,void *world_ctx);
/* $26DAC and $2946A semantic VRAM transport, after both producers finish. */
wf_status_t wf_r18_world_commit(wf_runtime_t *,wf_r18_world_ctx_t *);
#ifdef WF_R18_WORLD_DIAG
/* Fatal-only probe: reason 1=scene, 2=restore, 3/4=table reads,
 * 5/6=frame reads, 7=tile write. Disabled in release builds. */
/* Packed reason: bits31..24=reason, bits23..16=layer, bits15..0=index. */
extern volatile uint32_t wf_r18_world_error_info;
extern volatile uint32_t wf_r18_world_error_address;
#endif
#endif
