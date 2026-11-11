#ifndef WF_VRAM_READER_H
#define WF_VRAM_READER_H
#include "wp1980_vram_cache.h"
enum { WF_VRAM_READER=16u };
_Static_assert(!(WF_VRAM_READER&(WP1980_VRAM_ASSIGNED|WP1980_VRAM_COMMITTED|WP1980_VRAM_PINNED|WP1980_VRAM_RECENT)),"reader lease has its own bit");
typedef struct {uint16_t slot[WP1980_VRAM_FRAME_MAX];unsigned count;} wf_vram_reader_t;
/* Called after the new root generation is ACKed, before prepare overwrites
 * frame_slot. Only the prior reader expires; the current reader stays pinned
 * independently of the next frame's ordinary allocation pins. */
void wf_vram_reader_hold(wf_vram_reader_t *,wp1980_vram_cache_t *);
#endif
