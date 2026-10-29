#include "wp0_sequence_cache.h"
#include <stddef.h>
#include <string.h>

/* Every source packet begins on a longword boundary. Use an explicit wide
 * load for Saturn B-bus storage; host tests read the same big-endian bytes. */
static uint32_t packet_bytes(const uint8_t *p) {
    uint32_t v=*(const volatile uint32_t *)p;
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    v=__builtin_bswap32(v);
#endif
    return v;
}

bool wp0_sequence_cache_prepare(wp0_sequence_cache_t *cache,
    const void *sequence,uint32_t bytes,void *storage,uint32_t capacity) {
    if(!cache) return false;
    memset(cache,0,sizeof *cache);
    if(!sequence || !storage || bytes<32U || (bytes&3U) ||
       (((uintptr_t)sequence|(uintptr_t)storage)&3U)) return false;
    cache->sequence=sequence; cache->storage=storage;
    cache->sequence_bytes=bytes; cache->storage_bytes=capacity;
    for(uint32_t offset=0;offset<bytes;) {
        if(bytes-offset<32U) return false;
        const uint32_t size=packet_bytes(cache->sequence+offset);
        if(size<32U || (size&3U) || size>bytes-offset) return false;
        if(offset && size>=WP0_SEQUENCE_CACHE_THRESHOLD) {
            if(cache->count==WP0_SEQUENCE_CACHE_SLOTS ||
               size>capacity-cache->used) return false;
            wp0_sequence_cache_entry_t *entry=&cache->entries[cache->count++];
            entry->source_offset=offset; entry->size=size;
            entry->cache_offset=cache->used;
            memcpy(cache->storage+cache->used,cache->sequence+offset,size);
            cache->used+=size;
        }
        offset+=size;
    }
    cache->ready=true;
    return true;
}

const uint8_t *wp0_sequence_cache_resolve(const wp0_sequence_cache_t *cache,
    uint32_t offset) {
    if(!cache || !cache->ready || offset>=cache->sequence_bytes) return NULL;
    for(uint32_t i=0;i<cache->count;++i) {
        const wp0_sequence_cache_entry_t *entry=&cache->entries[i];
        if(offset==entry->source_offset) return cache->storage+entry->cache_offset;
    }
    return cache->sequence+offset;
}
