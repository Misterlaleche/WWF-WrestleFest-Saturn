#ifndef WP1980_VRAM_CACHE_H
#define WP1980_VRAM_CACHE_H
#include <stddef.h>
#include <stdint.h>
#ifndef WP1980_VRAM_CACHE_SLOTS
#define WP1980_VRAM_CACHE_SLOTS 2048u
#endif
#define WP1980_VRAM_FRAME_MAX 256u
#if WP1980_VRAM_CACHE_SLOTS>2048u
#define WP1980_VRAM_HASH_BUCKETS 4096u
#else
#define WP1980_VRAM_HASH_BUCKETS (WP1980_VRAM_CACHE_SLOTS*2u)
#endif
enum {WP1980_VRAM_ASSIGNED=1u,WP1980_VRAM_COMMITTED=2u,
      WP1980_VRAM_PINNED=4u,WP1980_VRAM_RECENT=8u};
typedef struct {
    uint16_t head[WP1980_VRAM_HASH_BUCKETS];
    uint16_t source[WP1980_VRAM_CACHE_SLOTS];
    uint16_t next[WP1980_VRAM_CACHE_SLOTS];
    uint8_t flags[WP1980_VRAM_CACHE_SLOTS];
    uint8_t crop_meta[WP1980_VRAM_CACHE_SLOTS];
    uint8_t crop_height[WP1980_VRAM_CACHE_SLOTS];
    uint16_t frame_slot[WP1980_VRAM_FRAME_MAX];
    unsigned frame_count,hand;
    uint32_t hits,misses,evictions;
} wp1980_vram_cache_t;
void wp1980_vram_cache_init(wp1980_vram_cache_t *);
/* A generic caller may reuse overlapping VRAM; preserve lifetime counters
 * while discarding every source/texture association before that write. */
void wp1980_vram_cache_invalidate(wp1980_vram_cache_t *);
/* Pin all existing frame IDs before allocating any new IDs. Source order is
 * retained by frame_slot; duplicate IDs refer to the same physical slot. */
int wp1980_vram_cache_prepare(wp1980_vram_cache_t *,const uint16_t *,unsigned);
/* Publish residency only after the real texture transfer has completed. */
int wp1980_vram_cache_commit(wp1980_vram_cache_t *,unsigned,uint8_t,uint8_t);
#endif
