#ifndef WF_JO_SCENE_PREPARE_H
#define WF_JO_SCENE_PREPARE_H
#include <stddef.h>
#include <stdint.h>

/* A temporary tail loan, entirely inside the caller's sprite-cache lease:
 * 128 KiB canonical scene, 8 KiB zlib state, 32 KiB resumable dictionary.
 * One step emits at most512 bytes. This bounds work units, not SH-2 cycles. */
#define WF_JO_SCENE_PREPARE_BYTES (131072u+8192u+32768u)
#define WF_JO_SCENE_PREPARE_SLICE 512u
typedef struct wf_jo_scene_prepare_stats {
    uint32_t begun,steps,decoded_bytes,completed,hits,cancelled,errors;
    uint32_t total_ms,max_step_ms;
} wf_jo_scene_prepare_stats_t;
extern volatile wf_jo_scene_prepare_stats_t wf_jo_scene_prepare_stats;

/* One main-thread preparation at a time; no CD access or display/IRQ work.
 * Cancel before rebinding/unloading the shared asset arena. A new begin
 * cancels an older preparation. Asset-status errors remain explicit. */
int wf_jo_scene_prepare_begin(const char *name);
/* Returns1 when complete,0 when more slices are needed, or a negative asset
 * status. A private inflater allows ordinary sprite reads between slices. */
int wf_jo_scene_prepare_step(void);
/* Returns1 and a borrowed canonical blob only after all block checksums and
 * lengths passed. The pointer expires on begin/cancel. Unready/mismatch is0. */
int wf_jo_scene_prepare_view(const char *name,const void **blob,uint32_t *bytes);
int wf_jo_scene_prepare_active(const char *name);
void wf_jo_scene_prepare_cancel(void);
#endif
