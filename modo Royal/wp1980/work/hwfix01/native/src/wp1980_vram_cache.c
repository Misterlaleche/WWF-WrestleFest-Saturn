#include "wp1980_vram_cache.h"
#include <string.h>

_Static_assert(WP1980_VRAM_CACHE_SLOTS>0u&&WP1980_VRAM_CACHE_SLOTS<32768u,"slot links fit16bits");
_Static_assert((WP1980_VRAM_HASH_BUCKETS&(WP1980_VRAM_HASH_BUCKETS-1u))==0u,"power-of-two hash table");

static unsigned bucket(uint16_t source){
    return ((unsigned)(source^(source>>7))*40503u)&(WP1980_VRAM_HASH_BUCKETS-1u);
}
static unsigned find(const wp1980_vram_cache_t *c,uint16_t source){
    unsigned link=c->head[bucket(source)];
    while(link){unsigned slot=link-1u;if(c->source[slot]==source)return slot;link=c->next[slot];}
    return WP1980_VRAM_CACHE_SLOTS;
}
static void remove_slot(wp1980_vram_cache_t *c,unsigned slot){
    uint16_t *link=&c->head[bucket(c->source[slot])];
    while(*link!=slot+1u)link=&c->next[*link-1u];
    *link=c->next[slot];
}
void wp1980_vram_cache_init(wp1980_vram_cache_t *c){if(c)memset(c,0,sizeof *c);}
void wp1980_vram_cache_invalidate(wp1980_vram_cache_t *c){
    if(!c)return;
    uint32_t hits=c->hits,misses=c->misses,evictions=c->evictions;
    wp1980_vram_cache_init(c);
    c->hits=hits;c->misses=misses;c->evictions=evictions;
}

int wp1980_vram_cache_prepare(wp1980_vram_cache_t *c,const uint16_t *source,unsigned count){
    if(!c||(!source&&count)||count>WP1980_VRAM_FRAME_MAX||count>WP1980_VRAM_CACHE_SLOTS)return -1;
    for(unsigned i=0;i<c->frame_count;i++)c->flags[c->frame_slot[i]]&=(uint8_t)~WP1980_VRAM_PINNED;
    c->frame_count=count;
    /* First protect every resident tile required anywhere in this frame. */
    for(unsigned i=0;i<count;i++){
        unsigned slot=find(c,source[i]);c->frame_slot[i]=(uint16_t)slot;
        if(slot<WP1980_VRAM_CACHE_SLOTS){
            c->flags[slot]|=WP1980_VRAM_PINNED|WP1980_VRAM_RECENT;
            if(c->flags[slot]&WP1980_VRAM_COMMITTED)++c->hits;else ++c->misses;
        }
    }
    for(unsigned i=0;i<count;i++)if(c->frame_slot[i]==WP1980_VRAM_CACHE_SLOTS){
        /* A duplicate formerly missing ID may have been assigned earlier in
         * this pass. It shares the same staging/physical identity. */
        unsigned slot=find(c,source[i]);
        if(slot==WP1980_VRAM_CACHE_SLOTS){
            for(unsigned examined=0;examined<WP1980_VRAM_CACHE_SLOTS*2u;examined++){
                slot=c->hand;if(++c->hand==WP1980_VRAM_CACHE_SLOTS)c->hand=0u;
                if(!(c->flags[slot]&WP1980_VRAM_ASSIGNED))break;
                if(c->flags[slot]&WP1980_VRAM_PINNED)continue;
                if(c->flags[slot]&WP1980_VRAM_RECENT){c->flags[slot]&=(uint8_t)~WP1980_VRAM_RECENT;continue;}
                break;
            }
            if(c->flags[slot]&WP1980_VRAM_PINNED)return -1;
            if(c->flags[slot]&WP1980_VRAM_ASSIGNED){remove_slot(c,slot);++c->evictions;}
            unsigned hash=bucket(source[i]);
            c->source[slot]=source[i];c->next[slot]=c->head[hash];c->head[hash]=(uint16_t)(slot+1u);
            c->flags[slot]=WP1980_VRAM_ASSIGNED|WP1980_VRAM_PINNED|WP1980_VRAM_RECENT;
            c->crop_meta[slot]=c->crop_height[slot]=0u;
        }
        c->frame_slot[i]=(uint16_t)slot;++c->misses;
    }
    return 0;
}

int wp1980_vram_cache_commit(wp1980_vram_cache_t *c,unsigned slot,uint8_t meta,uint8_t height){
    if(!c||slot>=WP1980_VRAM_CACHE_SLOTS||height>16u||meta>127u||
       !(c->flags[slot]&WP1980_VRAM_ASSIGNED))return -1;
    c->crop_meta[slot]=meta;c->crop_height[slot]=height;
    c->flags[slot]|=WP1980_VRAM_COMMITTED;
    return 0;
}
