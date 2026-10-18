#ifndef WF_GRAPHICS_H
#define WF_GRAPHICS_H
#include <stddef.h>
#include <stdint.h>
#include "wf_runtime.h"
#include "wf_visual.h"

#define WF_COMBAT_POSES_PER_SET 565u
#define WF_DRAW_PIECES_MAX 512u

typedef struct wf_graphics_blob {
    const uint8_t *pose_index_be; size_t pose_index_bytes;
    const uint8_t *piece_be; size_t piece_bytes;
} wf_graphics_blob_t;

typedef struct wf_draw_piece {
    uint16_t tile;
    uint8_t palette;
    uint8_t flip_x, flip_y;
    int16_t x,y;
    uint8_t chain_index, chain;
} wf_draw_piece_t;

typedef struct wf_draw_list {
    wf_draw_piece_t p[WF_DRAW_PIECES_MAX];
    unsigned count;
} wf_draw_list_t;

uint8_t wf_graphics_set_for_internal_id(uint8_t internal_id);
/* Emit one already-resolved ROM composition.  Partner/held selection remains
 * owned by Gameplay/WP02/WP36; this function never guesses that selection. */
wf_status_t wf_graphics_emit_composition(const uint8_t *piece_be,size_t piece_bytes,uint32_t start,uint16_t count,int16_t source_x,int16_t source_y,uint8_t mirror,uint8_t facing,wf_draw_list_t *out);
wf_status_t wf_graphics_emit_actor(const wf_actor_raw_t *a,const wf_actor_visual_t *v,const wf_graphics_blob_t *blob,wf_draw_list_t *out);

#endif
