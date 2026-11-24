#ifndef WF_JO_TILE_SLOTS_H
#define WF_JO_TILE_SLOTS_H
#include <stdint.h>
#include <string.h>

/* Persistent physical slots, plus unresolved new IDs for the current frame.
 * Assignment is postponed until every visible source has been seen: a tile
 * referenced late in the list can never be evicted by an earlier new tile.
 * At most256 resident +256 unresolved keys occupy the1024-entry index. */
typedef struct wf_jo_tile_slots {
    uint16_t index[1024],source[256],pending_source[256],pending_slot[256];
    uint8_t touched[256],epoch,building;
    uint16_t count,active_count,pending_count,hand;
} wf_jo_tile_slots_t;

static inline unsigned wf_jo_tile_slots_hash(uint16_t source)
{return ((uint32_t)source*UINT32_C(0x9e3779b1))>>22;}
static inline uint16_t wf_jo_tile_slots_key(const wf_jo_tile_slots_t *s,unsigned entry)
{return entry<=256u?s->source[entry-1u]:s->pending_source[entry-257u];}
static inline unsigned wf_jo_tile_slots_find(const wf_jo_tile_slots_t *s,uint16_t source)
{
    unsigned at=wf_jo_tile_slots_hash(source);
    while(s->index[at]&&wf_jo_tile_slots_key(s,s->index[at])!=source)at=(at+1u)&1023u;
    return at;
}
static inline void wf_jo_tile_slots_remove(wf_jo_tile_slots_t *s,unsigned hole)
{
    unsigned next=(hole+1u)&1023u;
    while(s->index[next]){
        unsigned home=wf_jo_tile_slots_hash(wf_jo_tile_slots_key(s,s->index[next]));
        if(((next-home)&1023u)>=((next-hole)&1023u)){
            s->index[hole]=s->index[next];hole=next;
        }
        next=(next+1u)&1023u;
    }
    s->index[hole]=0;
}
static inline void wf_jo_tile_slots_init(wf_jo_tile_slots_t *s)
{memset(s,0,sizeof *s);}
static inline void wf_jo_tile_slots_begin(wf_jo_tile_slots_t *s)
{
    /* A rejected/aborted plan may still have unresolved temporary entries. */
    if(s->building)for(unsigned i=0;i<s->pending_count;i++){
        unsigned at=wf_jo_tile_slots_find(s,s->pending_source[i]);
        if(s->index[at]>256u)wf_jo_tile_slots_remove(s,at);
    }
    if(!++s->epoch){memset(s->touched,0,sizeof s->touched);s->epoch=1;}
    s->active_count=s->pending_count=0;s->building=1;
}
static inline int wf_jo_tile_slots_get(wf_jo_tile_slots_t *s,uint16_t source,unsigned *logical)
{
    if(!s->building||!logical)return 0;
    unsigned at=wf_jo_tile_slots_find(s,source),entry=s->index[at];
    if(entry){
        unsigned slot=entry-1u;
        if(slot<256u&&s->touched[slot]!=s->epoch){
            if(s->active_count==256u)return 0;
            s->touched[slot]=s->epoch;++s->active_count;
        }
        *logical=slot;return 1;
    }
    if(s->active_count==256u)return 0;
    unsigned pending=s->pending_count++;
    s->pending_source[pending]=source;s->index[at]=(uint16_t)(257u+pending);
    ++s->active_count;*logical=256u+pending;return 1;
}
static inline int wf_jo_tile_slots_finish(wf_jo_tile_slots_t *s)
{
    if(!s->building)return 0;
    for(unsigned i=0;i<s->pending_count;i++){
        unsigned slot;
        if(s->count<256u)slot=s->count++;
        else{
            slot=s->hand;
            while(s->touched[slot]==s->epoch)slot=(slot+1u)&255u;
            unsigned old=wf_jo_tile_slots_find(s,s->source[slot]);
            if(s->index[old]!=slot+1u)return 0;
            wf_jo_tile_slots_remove(s,old);
        }
        uint16_t source=s->pending_source[i];
        unsigned at=wf_jo_tile_slots_find(s,source);
        if(s->index[at]!=257u+i)return 0;
        s->source[slot]=source;s->index[at]=(uint16_t)(slot+1u);
        s->pending_slot[i]=(uint16_t)slot;s->touched[slot]=s->epoch;
        s->hand=(uint16_t)((slot+1u)&255u);
    }
    s->building=0;return 1;
}
static inline unsigned wf_jo_tile_slots_resolve(const wf_jo_tile_slots_t *s,unsigned logical)
{return logical<256u?logical:s->pending_slot[logical-256u];}
static inline int wf_jo_tile_slots_active(const wf_jo_tile_slots_t *s,unsigned slot)
{return slot<s->count&&s->touched[slot]==s->epoch;}
#endif
