#ifndef WF_JO_TEXTURE_CACHE_H
#define WF_JO_TEXTURE_CACHE_H
#include <stdint.h>
#include <string.h>

#define WF_JO_GPU_TILES 3520u
#define WF_JO_GPU_INDEX 8192u
#define WF_JO_GPU_FRAME_TILES 256u
#define WF_JO_GPU_NONE 0xffffu
#define WF_JO_GPU_TAGGED 0x80u

/* Pixel identity and publication lifetime are independent. Invalidating a
 * source must never make a texture used by an older command list writable. */
typedef struct wf_jo_texture_entry {
    uint32_t last_tag;
    uint16_t source;
    uint8_t crop_meta,crop_height;
} wf_jo_texture_entry_t;
_Static_assert(sizeof(wf_jo_texture_entry_t)==8u,"GPU texture entry budget");

typedef struct wf_jo_texture_cache {
    wf_jo_texture_entry_t entries[WF_JO_GPU_TILES];
    uint16_t index[WF_JO_GPU_INDEX];
    uint8_t valid[WF_JO_GPU_TILES/8u],recent[WF_JO_GPU_TILES/8u];
    uint8_t pinned[WF_JO_GPU_TILES/8u],ready[WF_JO_GPU_FRAME_TILES/8u];
    uint16_t frame[WF_JO_GPU_FRAME_TILES],repeat[WF_JO_GPU_FRAME_TILES];
    uint16_t allocated,hand,frame_count,repeat_count;
    uint8_t building,repeat_valid;
    /* A completed immutable preparation can replace only the next pristine
     * builder. These saved pins never grant permission to overwrite VRAM. */
    uint8_t preparation_pinned[WF_JO_GPU_TILES/8u];
    uint8_t preparation_ready[WF_JO_GPU_FRAME_TILES/8u];
    const void *preparation_owner;
    uint32_t preparation_generation;
    uint8_t preparation_valid,preparation_complete,preparation_restored;
} wf_jo_texture_cache_t;

enum { WF_JO_CACHE_ERROR=-1,WF_JO_CACHE_BLOCKED=0,
       WF_JO_CACHE_HIT=1,WF_JO_CACHE_MISS=2 };

static inline unsigned wf_jo_cache_mask(unsigned slot)
{static const uint8_t masks[8]={1,2,4,8,16,32,64,128};return masks[slot&7u];}
static inline unsigned wf_jo_cache_bit(const uint8_t *bits,unsigned slot)
{return bits[slot>>3]&wf_jo_cache_mask(slot);}
static inline void wf_jo_cache_set(uint8_t *bits,unsigned slot)
{bits[slot>>3]|=(uint8_t)wf_jo_cache_mask(slot);}
static inline void wf_jo_cache_clear(uint8_t *bits,unsigned slot)
{bits[slot>>3]&=(uint8_t)~wf_jo_cache_mask(slot);}
static inline unsigned wf_jo_cache_hash(uint16_t source)
{return ((uint32_t)source*UINT32_C(0x9e3779b1))>>19;}
static inline unsigned wf_jo_cache_find(const wf_jo_texture_cache_t *c,uint16_t source)
{
    unsigned at=wf_jo_cache_hash(source);
    while(c->index[at]&&c->entries[c->index[at]-1u].source!=source)
        at=(at+1u)&(WF_JO_GPU_INDEX-1u);
    return at;
}
static inline void wf_jo_cache_remove(wf_jo_texture_cache_t *c,unsigned hole)
{
    unsigned next=(hole+1u)&(WF_JO_GPU_INDEX-1u);
    while(c->index[next]){
        unsigned home=wf_jo_cache_hash(c->entries[c->index[next]-1u].source);
        if(((next-home)&(WF_JO_GPU_INDEX-1u))>=((next-hole)&(WF_JO_GPU_INDEX-1u))){
            c->index[hole]=c->index[next];hole=next;
        }
        next=(next+1u)&(WF_JO_GPU_INDEX-1u);
    }
    c->index[hole]=0;
}
static inline void wf_jo_cache_init(wf_jo_texture_cache_t *c)
{
    memset(c,0,sizeof *c);
    c->repeat_valid=1; /* The initial empty display can be repeated. */
}
static inline void wf_jo_cache_forget_preparation(wf_jo_texture_cache_t *c)
{c->preparation_valid=0;c->preparation_complete=0;c->preparation_restored=0;}
static inline int wf_jo_cache_invalidate_sources(wf_jo_texture_cache_t *c)
{
    if(c->building)return 0;
    wf_jo_cache_forget_preparation(c);
    memset(c->index,0,sizeof c->index);memset(c->valid,0,sizeof c->valid);
    memset(c->recent,0,sizeof c->recent);
    return 1;
}
static inline int wf_jo_cache_begin(wf_jo_texture_cache_t *c,unsigned count)
{
    if(c->building||count>WF_JO_GPU_FRAME_TILES)return 0;
    for(unsigned i=0;i<count;++i)c->frame[i]=WF_JO_GPU_NONE;
    memset(c->ready,0,sizeof c->ready);
    c->frame_count=(uint16_t)count;c->building=1;c->preparation_complete=0;c->preparation_restored=0;
    return 1;
}
static inline void wf_jo_cache_abort(wf_jo_texture_cache_t *c)
{
    wf_jo_cache_forget_preparation(c);
    memset(c->pinned,0,sizeof c->pinned);
    c->frame_count=0;c->building=0;
}
/* Protect every existing source in the complete frame before allocating any
 * misses. An early miss must not evict a source referenced later in the list. */
static inline int wf_jo_cache_protect(wf_jo_texture_cache_t *c,unsigned logical,uint16_t source)
{
    if(!c->building||logical>=c->frame_count)return 0;
    wf_jo_cache_forget_preparation(c);
    unsigned entry=0;
    /* Persistent logical slots usually keep their source. Reuse the last
     * published mapping only after checking the current physical identity:
     * reset, a changed logical source or an evicted identity use the index. */
    if(logical<c->repeat_count){
        unsigned previous=c->repeat[logical];
        if(previous<WF_JO_GPU_TILES&&wf_jo_cache_bit(c->valid,previous)&&
           c->entries[previous].source==source)entry=previous+1u;
    }
    if(!entry)entry=c->index[wf_jo_cache_find(c,source)];
    if(entry){
        unsigned slot=entry-1u;
        if(c->frame[logical]!=WF_JO_GPU_NONE&&c->frame[logical]!=slot)return 0;
        c->frame[logical]=(uint16_t)slot;wf_jo_cache_set(c->pinned,slot);
        wf_jo_cache_set(c->recent,slot);wf_jo_cache_set(c->ready,logical);
    }else if(c->frame[logical]!=WF_JO_GPU_NONE)return 0;
    return 1;
}
static inline int wf_jo_cache_reusable(const wf_jo_texture_cache_t *c,unsigned slot,
                                      unsigned shown_valid,uint32_t shown_tag)
{
    const wf_jo_texture_entry_t *e=&c->entries[slot];
    return !wf_jo_cache_bit(c->pinned,slot)&&
        (!(e->crop_meta&WF_JO_GPU_TAGGED)||
         (shown_valid&&(int32_t)(shown_tag-e->last_tag)>0));
}
/* immutable=0 reserves an anonymous tile for the generic byte-upload ABI.
 * Its bytes are fenced exactly like ROM tiles, but never enter the ID index. */
static inline int wf_jo_cache_resolve(wf_jo_texture_cache_t *c,unsigned logical,
                                     uint16_t source,unsigned immutable,
                                     unsigned shown_valid,uint32_t shown_tag,
                                     unsigned *physical)
{
    if(!physical||!c->building||logical>=c->frame_count)return WF_JO_CACHE_ERROR;
    wf_jo_cache_forget_preparation(c);
    if(c->frame[logical]!=WF_JO_GPU_NONE){
        unsigned slot=c->frame[logical];
        if(!immutable||!wf_jo_cache_bit(c->ready,logical)||
           !wf_jo_cache_bit(c->valid,slot)||c->entries[slot].source!=source)
            return WF_JO_CACHE_ERROR;
        *physical=slot;return WF_JO_CACHE_HIT;
    }
    if(immutable){
        unsigned entry=c->index[wf_jo_cache_find(c,source)];
        if(entry){
            unsigned slot=entry-1u;
            c->frame[logical]=(uint16_t)slot;wf_jo_cache_set(c->pinned,slot);
            wf_jo_cache_set(c->recent,slot);wf_jo_cache_set(c->ready,logical);
            *physical=slot;return WF_JO_CACHE_HIT;
        }
    }
    unsigned slot;
    if(c->allocated<WF_JO_GPU_TILES)slot=c->allocated++;
    else{
        unsigned checked=0;
        for(;;){
            if(checked++==WF_JO_GPU_TILES*2u)return WF_JO_CACHE_BLOCKED;
            slot=c->hand;c->hand=(uint16_t)(slot+1u==WF_JO_GPU_TILES?0u:slot+1u);
            if(!wf_jo_cache_reusable(c,slot,shown_valid,shown_tag))continue;
            if(wf_jo_cache_bit(c->recent,slot)){wf_jo_cache_clear(c->recent,slot);continue;}
            break;
        }
        if(wf_jo_cache_bit(c->valid,slot)){
            unsigned at=wf_jo_cache_find(c,c->entries[slot].source);
            if(c->index[at]!=slot+1u)return WF_JO_CACHE_ERROR;
            wf_jo_cache_remove(c,at);wf_jo_cache_clear(c->valid,slot);
        }
    }
    c->frame[logical]=(uint16_t)slot;wf_jo_cache_set(c->pinned,slot);
    wf_jo_cache_set(c->recent,slot);*physical=slot;
    return WF_JO_CACHE_MISS;
}
/* Publish an identity only after its exact cropped bytes reached VRAM. */
static inline int wf_jo_cache_filled(wf_jo_texture_cache_t *c,unsigned logical,
                                    uint16_t source,unsigned immutable,
                                    uint8_t meta,uint8_t height)
{
    if(!c->building||logical>=c->frame_count||c->frame[logical]==WF_JO_GPU_NONE||
       (meta&0x80u)||height>16u||(meta&31u)+height>16u||
       (!(meta&32u)&&(meta&64u)))return 0;
    wf_jo_cache_forget_preparation(c);
    unsigned slot=c->frame[logical];wf_jo_texture_entry_t *e=&c->entries[slot];
    if(wf_jo_cache_bit(c->ready,logical))return 0;
    if(immutable){
        unsigned at=wf_jo_cache_find(c,source);
        if(c->index[at])return 0; /* Plans contain unique immutable source IDs. */
        e->source=source;c->index[at]=(uint16_t)(slot+1u);wf_jo_cache_set(c->valid,slot);
    }
    e->crop_meta=(uint8_t)((e->crop_meta&WF_JO_GPU_TAGGED)|meta);e->crop_height=height;
    wf_jo_cache_set(c->ready,logical);
    return 1;
}
static inline int wf_jo_cache_can_publish(const wf_jo_texture_cache_t *c)
{
    if(!c->building)return 0;
    for(unsigned i=0;i<c->frame_count;++i)
        if(c->frame[i]!=WF_JO_GPU_NONE&&!wf_jo_cache_bit(c->ready,i))return 0;
    return 1;
}
static inline int wf_jo_cache_publish(wf_jo_texture_cache_t *c,uint32_t tag)
{
    if(!wf_jo_cache_can_publish(c))return 0;
    unsigned immutable=1;
    for(unsigned i=0;i<c->frame_count;++i){
        unsigned slot=c->frame[i];c->repeat[i]=(uint16_t)slot;
        if(slot==WF_JO_GPU_NONE)continue;
        if(!wf_jo_cache_bit(c->valid,slot))immutable=0;
        c->entries[slot].last_tag=tag;c->entries[slot].crop_meta|=WF_JO_GPU_TAGGED;
    }
    if(immutable){
        memcpy(c->preparation_pinned,c->pinned,sizeof c->pinned);
        memcpy(c->preparation_ready,c->ready,sizeof c->ready);
    }
    c->repeat_count=c->frame_count;c->repeat_valid=1;
    wf_jo_cache_abort(c);
    c->preparation_complete=(uint8_t)immutable;
    return 1;
}
/* Claim only the just-published immutable frame, with every prepared slot
 * consumed. Foreign publications and abandoned builders cannot be claimed. */
static inline int wf_jo_cache_remember_preparation(wf_jo_texture_cache_t *c,
    const void *owner,uint32_t generation,const uint8_t *active)
{
    c->preparation_valid=0;
    if(c->building||!c->preparation_complete||!owner||!generation||!active||
       memcmp(active,c->preparation_ready,sizeof c->ready))return 0;
    c->preparation_owner=owner;c->preparation_generation=generation;
    c->preparation_valid=1;return 1;
}
static inline int wf_jo_cache_preparation_available(const wf_jo_texture_cache_t *c,
    const void *owner,uint32_t generation,unsigned count)
{
    return c->building&&c->preparation_valid&&generation&&
        c->preparation_owner==owner&&c->preparation_generation==generation&&
        count==c->frame_count&&count==c->repeat_count;
}
static inline int wf_jo_cache_restore_preparation(wf_jo_texture_cache_t *c,
    const void *owner,uint32_t generation,const uint8_t *active)
{
    if(!wf_jo_cache_preparation_available(c,owner,generation,c->frame_count)||
       !active||memcmp(active,c->preparation_ready,sizeof c->ready))return 0;
    wf_jo_cache_forget_preparation(c);
    /* Only begin/repeat may intervene, so no identity, crop or recent bit
     * can have changed. All saved pins are still recent. Retain the exposed
     * frame tail exactly as normal begin/protect do for shorter frames. */
    memcpy(c->frame,c->repeat,c->frame_count*sizeof c->frame[0]);
    memcpy(c->pinned,c->preparation_pinned,sizeof c->pinned);
    memcpy(c->ready,c->preparation_ready,sizeof c->ready);
    c->preparation_restored=1;
    return 1;
}
/* A private consumer may claim this proof once, before any intervening cache
 * operation. It does not change pixels, mappings, readiness or display tags. */
static inline int wf_jo_cache_consume_preparation(wf_jo_texture_cache_t *c,
    const void *owner,uint32_t generation,unsigned count)
{
    if(!c->building||!c->preparation_restored||!generation||
       c->preparation_owner!=owner||c->preparation_generation!=generation||
       c->frame_count!=count||c->repeat_count!=count)return 0;
    c->preparation_restored=0;return 1;
}
static inline int wf_jo_cache_repeat(wf_jo_texture_cache_t *c,uint32_t tag)
{
    if(c->building||!c->repeat_valid)return 0;
    for(unsigned i=0;i<c->repeat_count;++i){
        unsigned slot=c->repeat[i];if(slot==WF_JO_GPU_NONE)continue;
        c->entries[slot].last_tag=tag;c->entries[slot].crop_meta|=WF_JO_GPU_TAGGED;
    }
    return 1;
}
#endif
