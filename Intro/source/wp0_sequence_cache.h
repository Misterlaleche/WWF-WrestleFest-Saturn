#ifndef WP0_SEQUENCE_CACHE_H
#define WP0_SEQUENCE_CACHE_H
#include <stdbool.h>
#include <stdint.h>

enum { WP0_SEQUENCE_CACHE_THRESHOLD=4096, WP0_SEQUENCE_CACHE_SLOTS=6 };
typedef struct {
    uint32_t source_offset,size,cache_offset;
} wp0_sequence_cache_entry_t;
typedef struct {
    const uint8_t *sequence;
    uint8_t *storage;
    uint32_t sequence_bytes,storage_bytes,used,count;
    bool ready;
    wp0_sequence_cache_entry_t entries[WP0_SEQUENCE_CACHE_SLOTS];
} wp0_sequence_cache_t;

/* Build once while hidden. Sequence/storage must be aligned to four bytes.
 * The initial full-state packet remains outside the active deadline. */
bool wp0_sequence_cache_prepare(wp0_sequence_cache_t *cache,
    const void *sequence,uint32_t bytes,void *storage,uint32_t capacity);
const uint8_t *wp0_sequence_cache_resolve(const wp0_sequence_cache_t *cache,
    uint32_t offset);
#endif
