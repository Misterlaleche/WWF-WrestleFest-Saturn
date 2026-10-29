#ifndef WF_R18_FRONTEND_LEAF_H
#define WF_R18_FRONTEND_LEAF_H
#include "wf_r18_frontend_render.h"
#include "wf_r18_metasprite.h"
typedef struct wf_r18_frontend_leaf {
 wf_r18_read_rom_fn read_rom;void *user;
 wf_r18_sprite_record_t *records;size_t capacity;
 wf_r18_metasprite_cache_t *cache;
} wf_r18_frontend_leaf_t;
/* Native graphics leaf for the enclosing $2836 contract: emits normalized
 * hardware-consumed sprite fields, advances A2, retains D6/D7. The enclosing
 * owner saves/restores D0-D5/A0-A5; this is not a standalone full-D1FC ABI.
 * Raw title objects are read by bounded offsets, never cast to large actors. */
wf_status_t wf_r18_frontend_leaf_emit(void *,wf_r18_frontend_render_t *);
wf_status_t wf_r18_frontend_leaf_clear(void *,uint32_t);
#endif
