#include "wp2015_secondary.h"
#include "wp2015_secondary_memory.h"
#include "wp2015_cd_cache.h"
#include <string.h>

enum { SECONDARY_MAGIC=0x57505331u, GUARD=0xa6e79d35u,
       BAD_ARGUMENT=-901,BAD_HEADER=-902,BAD_METADATA=-903,BAD_SOURCE=-904,
       BAD_OUTPUT=-905,BAD_GUARD=-906,BAD_OWNER=-907 };
wp2015_secondary_workspace_t *wp2015_secondary_active;
static uint8_t *tail;
static unsigned owner_busy;
static uint32_t generation;

#ifdef WP2015_SECONDARY_HOST_TEST
extern void *wp2015_secondary_test_address(uint32_t);
extern uint16_t wp2015_secondary_test_tick(void);
#define VRAM(address) ((volatile uint16_t *)wp2015_secondary_test_address(address))
#define tick wp2015_secondary_test_tick
#else
#define VRAM(address) ((volatile uint16_t *)(uintptr_t)(address))
static uint16_t tick(void){
    volatile uint8_t *const high=(volatile uint8_t *)(uintptr_t)0xfffffe12u;
    uint16_t first,low,again;
    do {first=high[0];low=high[1];again=high[0];}while(first!=again);
    return (uint16_t)((first<<8)|low);
}
#endif
static uint16_t be16(const uint8_t *p){return (uint16_t)((unsigned)p[0]<<8|p[1]);}
static uint32_t be32(const uint8_t *p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static uint32_t hash(uint32_t value,const uint8_t *p,size_t bytes){
    while(bytes--){value^=*p++;value*=16777619u;}return value;
}
static unsigned pop8(unsigned value){
    value-=(value>>1)&0x55u;value=(value&0x33u)+((value>>2)&0x33u);
    return (value+(value>>4))&15u;
}
static void maximum(uint32_t *target,uint16_t begin){uint16_t elapsed=(uint16_t)(tick()-begin);if(elapsed>*target)*target=elapsed;}
static uintptr_t physical(uintptr_t value){
#if defined(__sh__) || defined(WF_PLATFORM_SATURN)
    return value&0x1fffffffu;
#else
    return value;
#endif
}
static int overlap(const void *a,size_t an,const void *b,size_t bn){
    uintptr_t x=physical((uintptr_t)a),y=physical((uintptr_t)b);
#if defined(__sh__) || defined(WF_PLATFORM_SATURN)
    if(an>0x20000000u-x||bn>0x20000000u-y)return 1;
#endif
    if(an>UINTPTR_MAX-x||bn>UINTPTR_MAX-y)return 1;
    return x<y+bn&&y<x+an;
}
static int guards(const wp2015_secondary_workspace_t *w){
    for(unsigned i=0;i<8u;i++){
        if(w->end_guard[i]!=GUARD)return 0;
        for(unsigned n=0;n<WP2015_SECONDARY_CACHE_BLOCKS;n++)
            if(w->output.cache[n].guard[i]!=GUARD)return 0;
    }
    return 1;
}
void wp2015_secondary_reset(void){wp2015_secondary_active=NULL;tail=NULL;++generation;}
int wp2015_secondary_in_use(void){return owner_busy!=0u;}
static int read_source(wp2015_secondary_workspace_t *w,wp2015_secondary_file_read_fn reader,
                       uint32_t offset,void *destination,size_t bytes){
    int rc=reader(WP2015_SECONDARY_FILE,offset,destination,bytes);
    if(w->state.generation!=generation)return BAD_OWNER;
    return rc?BAD_SOURCE:0;
}

/* B-bus storage is touched only as aligned halfwords. No byte stores or
 * SCU-DMA transfers target it. An odd source offset is handled explicitly. */
static void vram_write(uint32_t address,const uint8_t *data,size_t bytes){
    volatile uint16_t *out=VRAM(address);
    while(bytes){*out++=be16(data);data+=2;bytes-=2;}
}
static void vram_read(uint32_t address,uint8_t *out,size_t bytes){
    volatile uint16_t *source=VRAM(address&~1u);
    if(address&1u){*out++=(uint8_t)*source++;--bytes;}
    while(bytes>=2u){uint16_t word=*source++;out[0]=(uint8_t)(word>>8);out[1]=(uint8_t)word;out+=2;bytes-=2;}
    if(bytes)*out=(uint8_t)(*source>>8);
}

static int pool_read(wp2015_secondary_workspace_t *w,uint32_t offset,uint8_t *out,size_t bytes){
    if(offset>w->state.payload_bytes||bytes>w->state.payload_bytes-offset)return BAD_SOURCE;
    while(bytes){
        uint32_t position=offset;size_t take;
        if(position<sizeof w->resident){
            take=sizeof w->resident-position;if(take>bytes)take=bytes;
            memcpy(out,w->resident+position,take);++w->state.hw_reads;
        }else if((position-=sizeof w->resident)<WP2015_SECONDARY_LWRAM_BYTES){
            take=WP2015_SECONDARY_LWRAM_BYTES-position;if(take>bytes)take=bytes;
            memcpy(out,tail+position,take);++w->state.lw_reads;
        }else{
            position-=WP2015_SECONDARY_LWRAM_BYTES;
            unsigned region=0;
            while(region<WP2015_SECONDARY_REGIONS&&position>=wp2015_secondary_vram[region].bytes){
                position-=wp2015_secondary_vram[region].bytes;++region;
            }
            if(region==WP2015_SECONDARY_REGIONS){
                ++w->state.cd_reads;
                return wp2015_cd_cache_read(position,out,bytes,w->cd_scratch,sizeof w->cd_scratch);
            }
            take=wp2015_secondary_vram[region].bytes-position;if(take>bytes)take=bytes;
            vram_read(wp2015_secondary_vram[region].address+position,out,take);++w->state.vram_reads;
        }
        offset+=(uint32_t)take;out+=take;bytes-=take;
    }
    return 0;
}

static int metadata(wp2015_secondary_workspace_t *w,const uint8_t *header,
                    uint32_t file_bytes,wp2015_secondary_file_read_fn read_file){
    if(memcmp(header,"R18S",4)||be16(header+4)!=1u||be16(header+6)!=3u||
       be32(header+8)!=WP2015_SECONDARY_TILES||be32(header+12)!=WP2015_SECONDARY_BLOCKS||
       be32(header+20)!=WP2015_SECONDARY_METADATA_BYTES+WP2015_SECONDARY_MODEL_BYTES)
        return BAD_HEADER;
    uint32_t bytes=be32(header+16),start=WP2015_SECONDARY_HEADER_BYTES+be32(header+20);
    if(!bytes||bytes>w->state.pool_capacity||file_bytes<start||bytes!=file_bytes-start)return BAD_HEADER;
    uint8_t *raw=(uint8_t *)w+offsetof(wp2015_secondary_workspace_t,membership);
    int rc=read_source(w,read_file,WP2015_SECONDARY_HEADER_BYTES,raw,WP2015_SECONDARY_METADATA_BYTES);
    if(rc)return rc;
    uint32_t checksum=hash(2166136261u,raw,WP2015_SECONDARY_METADATA_BYTES);
    raw=(uint8_t *)&w->output;
    rc=read_source(w,read_file,WP2015_SECONDARY_HEADER_BYTES+WP2015_SECONDARY_METADATA_BYTES,
                   raw,WP2015_SECONDARY_MODEL_BYTES);
    if(rc)return rc;
    if(hash(checksum,raw,WP2015_SECONDARY_MODEL_BYTES)!=be32(header+24))return BAD_METADATA;
    unsigned total=0;
    for(unsigned page=0;page<256u;page++){
        unsigned prefix=be16((const uint8_t *)&w->prefix[page]);
        if(prefix!=total)return BAD_METADATA;
        w->prefix[page]=(uint16_t)prefix;
        for(unsigned byte=0;byte<32u;byte++)total+=pop8(w->membership[page*32u+byte]);
    }
    if(total!=WP2015_SECONDARY_TILES)return BAD_METADATA;
    uint32_t previous=0;
    for(unsigned i=0;i<=WP2015_SECONDARY_BLOCKS;i++){
        uint32_t next=be32((const uint8_t *)&w->offsets[i]);
        if(next>bytes||(i?(next<previous+4u||next-previous>sizeof w->input):next!=0u))return BAD_METADATA;
        w->offsets[i]=next;previous=next;
    }
    if(previous!=bytes)return BAD_METADATA;
    for(unsigned i=0;i<WP2015_SECONDARY_BLOCKS;i++)w->hashes[i]=be32((const uint8_t *)&w->hashes[i]);
    for(unsigned i=0;i<286u;i++)w->output.model.literal[i]=be16((const uint8_t *)&w->output.model.literal[i]);
    for(unsigned i=0;i<30u;i++)w->output.model.distance[i]=be16((const uint8_t *)&w->output.model.distance[i]);
    if(wp2015_rans_tables((uint32_t *)&w->tables,sizeof w->tables/4u,
                          w->output.model.literal,w->output.model.distance))return BAD_METADATA;
    w->state.payload_bytes=bytes;w->state.tile_count=total;w->state.block_count=WP2015_SECONDARY_BLOCKS;
    return 0;
}

int wp2015_secondary_load(wp2015_secondary_workspace_t *w,void *lwram,size_t lwram_bytes,
                         uint32_t file_bytes,wp2015_secondary_file_read_fn read_file){
    if(owner_busy)return BAD_OWNER;
    wp2015_secondary_reset();
    if(!w||!lwram||!read_file||((uintptr_t)w&31u)||((uintptr_t)lwram&127u)||
       lwram_bytes!=WP2015_SECONDARY_LWRAM_BYTES||overlap(w,sizeof *w,lwram,lwram_bytes))return BAD_ARGUMENT;
    for(unsigned i=0;i<WP2015_SECONDARY_REGIONS;i++)
        if(overlap(w,sizeof *w,(const void *)VRAM(wp2015_secondary_vram[i].address),wp2015_secondary_vram[i].bytes)||
           overlap(lwram,lwram_bytes,(const void *)VRAM(wp2015_secondary_vram[i].address),wp2015_secondary_vram[i].bytes))return BAD_ARGUMENT;
    owner_busy=1;uint32_t lease=generation;
    memset(&w->state,0,sizeof w->state);w->state.magic=SECONDARY_MAGIC;w->state.generation=lease;
    w->state.pool_capacity=sizeof w->resident+lwram_bytes+WP2015_SECONDARY_VRAM_BYTES+
        WP2015_CD_CACHE_SECTOR_BYTES*WP2015_CD_CACHE_MAX_SECTORS;
    uint32_t header_words[8];const uint8_t *header=(const uint8_t *)header_words;
    int rc=read_source(w,read_file,0,header_words,sizeof header_words);
    if(rc)goto done;
    if((rc=metadata(w,header,file_bytes,read_file)))goto done;
    for(unsigned i=0;i<8u;i++){
        w->end_guard[i]=GUARD;
        for(unsigned slot=0;slot<WP2015_SECONDARY_CACHE_BLOCKS;slot++)w->output.cache[slot].guard[i]=GUARD;
    }
    for(unsigned slot=0;slot<WP2015_SECONDARY_CACHE_BLOCKS;slot++)w->state.cache_tags[slot]=0xffffu;
    tail=lwram;
    uint32_t file_at=WP2015_SECONDARY_HEADER_BYTES+WP2015_SECONDARY_METADATA_BYTES+WP2015_SECONDARY_MODEL_BYTES;
    uint32_t payload_at=0,checksum=2166136261u;
    /* Fill ordinary RAM, then every explicitly free B-bus interval. All
     * region sizes are even; source chunks fit the disjoint startup scratch. */
    for(unsigned region=0;region<WP2015_SECONDARY_REGIONS+2u;region++){
        size_t bytes=region==0u?sizeof w->resident:region==1u?lwram_bytes:wp2015_secondary_vram[region-2u].bytes;
        if(bytes>w->state.payload_bytes-payload_at){rc=BAD_HEADER;goto done;}
        for(size_t at=0;at<bytes;){
            size_t take=bytes-at;if(take>sizeof w->cd_scratch)take=sizeof w->cd_scratch;
            if((rc=read_source(w,read_file,file_at+payload_at,w->cd_scratch,take)))goto done;
            checksum=hash(checksum,(const uint8_t *)w->cd_scratch,take);
            if(region<2u)memcpy((region?tail:w->resident)+at,w->cd_scratch,take);
            else vram_write(wp2015_secondary_vram[region-2u].address+(uint32_t)at,(const uint8_t *)w->cd_scratch,take);
            at+=take;payload_at+=(uint32_t)take;
        }
    }
    w->state.cd_base=payload_at;w->state.cd_bytes=w->state.payload_bytes-payload_at;
    if(!w->state.cd_bytes||(rc=wp2015_cd_cache_reset())){if(!rc)rc=BAD_HEADER;goto done;}
    while(payload_at<w->state.payload_bytes){
        size_t take=w->state.payload_bytes-payload_at;
        if(take>WP2015_CD_CACHE_SECTOR_BYTES)take=WP2015_CD_CACHE_SECTOR_BYTES;
        memset(w->cd_scratch,0,WP2015_CD_CACHE_SECTOR_BYTES);
        if((rc=read_source(w,read_file,file_at+payload_at,w->cd_scratch,take)))goto done;
        checksum=hash(checksum,(const uint8_t *)w->cd_scratch,take);
        if((rc=wp2015_cd_cache_append(w->cd_scratch)))goto done;
        payload_at+=(uint32_t)take;
    }
    if(checksum!=be32(header+28)){rc=BAD_SOURCE;goto done;}
    if((rc=wp2015_cd_cache_seal(w->state.cd_bytes)))goto done;
    if(wp2015_cd_cache_bytes()!=w->state.cd_bytes){rc=BAD_SOURCE;goto done;}
    if(!guards(w)){rc=BAD_GUARD;goto done;}
    if(generation!=lease){rc=BAD_OWNER;goto done;}
    w->state.generation=lease;w->state.ready=1;wp2015_secondary_active=w;
done:
    if(rc){w->state.last_error=(uint32_t)rc;w->state.ready=0;wp2015_secondary_active=NULL;tail=NULL;}
    owner_busy=0;return rc;
}

int wp2015_secondary_tile_read(uint16_t tile,uint8_t dst[128]){
    wp2015_secondary_workspace_t *w=wp2015_secondary_active;
    if(!w)return 0;
    if(owner_busy||!dst||overlap(w,sizeof *w,dst,128u)||overlap(tail,WP2015_SECONDARY_LWRAM_BYTES,dst,128u))return -1;
    for(unsigned region=0;region<WP2015_SECONDARY_REGIONS;region++)
        if(overlap((const void *)VRAM(wp2015_secondary_vram[region].address),wp2015_secondary_vram[region].bytes,dst,128u))return -1;
    owner_busy=1;uint16_t started=tick();int rc=0;
    ++w->state.requests;w->state.last_tile=tile;
    if(!guards(w)||w->state.generation!=generation||!w->state.ready){rc=BAD_GUARD;goto failed;}
    unsigned byte=tile>>3,bit=tile&7u;
    if(!(w->membership[byte]&(1u<<bit))){++w->state.absent_tiles;owner_busy=0;return 0;}
    unsigned rank=w->prefix[tile>>8];
    for(unsigned i=(tile>>8)*32u;i<byte;i++)rank+=pop8(w->membership[i]);
    rank+=pop8(w->membership[byte]&((1u<<bit)-1u));
    unsigned block=rank>>3,slot=0;
    if(block>=WP2015_SECONDARY_BLOCKS){rc=BAD_METADATA;goto failed;}
    w->state.last_block=block;
    for(;slot<WP2015_SECONDARY_CACHE_BLOCKS;slot++)if(w->state.cache_tags[slot]==block)break;
    if(slot<WP2015_SECONDARY_CACHE_BLOCKS)++w->state.cache_hits;
    else{
        slot=w->state.next_slot;w->state.next_slot=(slot+1u)&(WP2015_SECONDARY_CACHE_BLOCKS-1u);
        w->state.cache_tags[slot]=0xffffu;
        uint32_t first=w->offsets[block],bytes=w->offsets[block+1u]-first;
        if(bytes<4u||bytes>sizeof w->input){rc=BAD_METADATA;goto failed;}
        memset(w->input,0xa5,sizeof w->input);
        uint16_t begin=tick();rc=pool_read(w,first,w->input,bytes);maximum(&w->state.max_read_ticks,begin);
        if(rc||wp2015_secondary_active!=w){++w->state.source_read_errors;if(!rc)rc=BAD_OWNER;goto failed;}
        unsigned output_bytes=WP2015_SECONDARY_BLOCK_BYTES;begin=tick();
        rc=wp2015_rans_decode(w->output.cache[slot].bytes,&output_bytes,w->input,bytes,(const uint32_t *)&w->tables);
        maximum(&w->state.max_decode_ticks,begin);
        if(rc||output_bytes!=WP2015_SECONDARY_BLOCK_BYTES){rc=BAD_OUTPUT;goto failed;}
        if(hash(2166136261u,w->output.cache[slot].bytes,output_bytes)!=w->hashes[block]){
            ++w->state.output_hash_errors;rc=BAD_OUTPUT;goto failed;
        }
        for(unsigned i=bytes;i<sizeof w->input;i++)if(w->input[i]!=0xa5u){rc=BAD_GUARD;goto failed;}
        if(!guards(w)){rc=BAD_GUARD;goto failed;}
        w->state.cache_tags[slot]=(uint16_t)block;++w->state.block_decodes;
    }
    memcpy(dst,w->output.cache[slot].bytes+(rank&7u)*128u,128u);
    maximum(&w->state.max_request_ticks,started);owner_busy=0;return 1;
failed:
    w->state.last_error=(uint32_t)rc;w->state.ready=0;
    /* Keep the failed pool published: further resident requests must fail
     * visibly, never fall through to a mechanical-CD read. reset/load is
     * the only path that may intentionally make the whole pool unavailable. */
    maximum(&w->state.max_request_ticks,started);owner_busy=0;return -1;
}
