/* Private diagnostic only. The normal entry never includes this file.
 * Its records borrow only the unused tail after the exact114918-byte ring
 * blob, not any secondary/graphics/ROM cache or HWRAM allocator storage. */
#include "wp2015_secondary_memory.h"
typedef struct {uint32_t ticks,hash;} secondary_probe_record_t;
typedef struct {
    uint32_t magic,phase,done,error,tiles,blocks,bytes,hash_errors;
    uint32_t first_vblank,last_vblank,max_request_fields,max_first_ticks,sum_first_ticks;
    uint32_t record_address,record_bytes,alias_rejections,guard_errors,frt_tcr,ccr;
    uint32_t postload_decode_count,final_decode_count,final_cache_hits;
    uint32_t repeat_blocks,repeat_hash_errors;
} secondary_probe_t;
static volatile secondary_probe_t wp2015_secondary_probe_observer;
#define SECONDARY_PROBE_GUARD 0x6c9a35e1u
#define SECONDARY_PROBE_BEGIN (WF_SATURN_LWRAM_SCENE+((114918u+31u)&~31u))
#define SECONDARY_PROBE_RECORDS (SECONDARY_PROBE_BEGIN+32u)
_Static_assert(SECONDARY_PROBE_RECORDS+WP2015_SECONDARY_BLOCKS*sizeof(secondary_probe_record_t)+32u<=
               WF_SATURN_LWRAM_SCENE+WF_SATURN_LWRAM_SCENE_BYTES,"Probe fits unused ring-blob tail");
static uint16_t secondary_probe_tick(void){
    volatile uint8_t *high=(volatile uint8_t *)(uintptr_t)0xfffffe12u;
    unsigned a,b,c;do{a=high[0];b=high[1];c=high[0];}while(a!=c);
    return (uint16_t)((a<<8)|b);
}
static uint32_t secondary_probe_hash(uint32_t value,const uint8_t *p,unsigned n){
    while(n--)value=(value^*p++)*16777619u;return value;
}
static void wp2015_secondary_probe(wp2015_secondary_workspace_t *workspace,void *leased_tail){
    volatile secondary_probe_t *o=&wp2015_secondary_probe_observer;
    volatile secondary_probe_record_t *records=(volatile secondary_probe_record_t *)(uintptr_t)SECONDARY_PROBE_RECORDS;
    volatile uint32_t *before=(volatile uint32_t *)(uintptr_t)SECONDARY_PROBE_BEGIN;
    volatile uint32_t *after=(volatile uint32_t *)(uintptr_t)(SECONDARY_PROBE_RECORDS+
        WP2015_SECONDARY_BLOCKS*sizeof(secondary_probe_record_t));
    o->magic=0x53505231u;o->phase=1;o->record_address=SECONDARY_PROBE_RECORDS;
    o->record_bytes=WP2015_SECONDARY_BLOCKS*sizeof(secondary_probe_record_t);
    o->ccr=*(volatile uint8_t *)(uintptr_t)0xfffffe92u;
    o->frt_tcr=*(volatile uint8_t *)(uintptr_t)0xfffffe16u;
    for(unsigned i=0;i<8u;i++)before[i]=after[i]=SECONDARY_PROBE_GUARD;
    uint16_t first_tile=0;
    while(!(workspace->membership[first_tile>>3]&(1u<<(first_tile&7u))))++first_tile;
    uint8_t *aliases[]={
        (uint8_t *)((uintptr_t)workspace|0x20000000u),
        (uint8_t *)((uintptr_t)leased_tail|0x20000000u),
        (uint8_t *)(uintptr_t)wp2015_secondary_vram[0].address,
        (uint8_t *)(uintptr_t)0x1fffffe0u
    };
    for(unsigned i=0;i<4u;i++){
        if(wp2015_secondary_tile_read(first_tile,aliases[i])!=-1){o->error=1;goto done;}
        ++o->alias_rejections;
    }
    if(wp2015_secondary_active!=workspace||!workspace->state.ready){o->error=2;goto done;}
    o->phase=2;o->first_vblank=video_clock;o->postload_decode_count=workspace->state.block_decodes;
    uint32_t assembled=2166136261u,first_ticks=0;
    for(unsigned tile=0;tile<65536u;tile++)if(workspace->membership[tile>>3]&(1u<<(tile&7u))){
        const uint8_t *pixels=NULL;
        uint32_t field=video_clock;uint16_t begin=secondary_probe_tick();
        if(wf_r18_tile_read((uint16_t)tile,&pixels)!=WF_OK||!pixels){o->error=3;goto done;}
        uint16_t elapsed=(uint16_t)(secondary_probe_tick()-begin);
        uint32_t fields=video_clock-field;if(fields>o->max_request_fields)o->max_request_fields=fields;
        if(fields>8u){o->error=4;goto done;}
        if(!(o->tiles&7u)){
            assembled=2166136261u;first_ticks=elapsed;
            if(elapsed>o->max_first_ticks)o->max_first_ticks=elapsed;
            o->sum_first_ticks+=elapsed;
        }
        assembled=secondary_probe_hash(assembled,pixels,128u);++o->tiles;o->bytes+=128u;
        if(!(o->tiles&7u)){
            unsigned block=o->blocks;records[block]=(secondary_probe_record_t){first_ticks,assembled};
            if(assembled!=workspace->hashes[block]){++o->hash_errors;o->error=5;goto done;}
            ++o->blocks;
        }
    }
    if(o->tiles!=WP2015_SECONDARY_TILES||o->blocks!=WP2015_SECONDARY_BLOCKS){o->error=6;goto done;}
    /* Revisit block starts in a coprime order, forcing retained-CD Gets
     * after the complete sequential pass, without deleting any sectors. */
    o->phase=3;
    for(unsigned pass=0;pass<64u;pass++){
        unsigned wanted=(pass*997u)%WP2015_SECONDARY_BLOCKS,rank=0;
        uint32_t checksum=2166136261u;unsigned copied=0;
        for(unsigned tile=0;tile<65536u&&copied<8u;tile++)if(workspace->membership[tile>>3]&(1u<<(tile&7u))){
            if(rank/8u==wanted){
                uint8_t pixels[128];
                if(wp2015_secondary_tile_read((uint16_t)tile,pixels)!=1){o->error=7;goto done;}
                checksum=secondary_probe_hash(checksum,pixels,128u);++copied;
            }
            ++rank;
        }
        if(copied!=8u||checksum!=workspace->hashes[wanted]){++o->repeat_hash_errors;o->error=8;goto done;}
        ++o->repeat_blocks;
    }
    for(unsigned i=0;i<8u;i++)if(before[i]!=SECONDARY_PROBE_GUARD||after[i]!=SECONDARY_PROBE_GUARD)++o->guard_errors;
    if(o->guard_errors||workspace->state.last_error||!workspace->state.ready)o->error=9;
    o->phase=4;
done:
    o->last_vblank=video_clock;o->final_decode_count=workspace->state.block_decodes;
    o->final_cache_hits=workspace->state.cache_hits;o->done=1;
}
