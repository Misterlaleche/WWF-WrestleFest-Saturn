#include <stdio.h>
#include <string.h>
#include "wf_r18_metasprite.h"

#define CHECK(c) do {if(!(c)){fprintf(stderr,"metasprite-cache:%u: %s\n",__LINE__,#c);return 1;}}while(0)
#define ROM_SIZE 0x80000u

typedef struct source {
    uint8_t rom[ROM_SIZE];
    unsigned reads,body_reads;
    uint32_t fail_at;
} source_t;
static source_t source;
static uint8_t palette[256];
static wf_r18_sprite_record_t plain_records[1024],cached_records[1024];

static void put16(uint32_t at,uint16_t value){source.rom[at]=(uint8_t)(value>>8);source.rom[at+1]=(uint8_t)value;}
static void put32(uint32_t at,uint32_t value){put16(at,(uint16_t)(value>>16));put16(at+2,(uint16_t)value);}
static wf_status_t read_rom(void *user,uint32_t at,void *out,size_t length){
    source_t *s=user;s->reads++;
    if(length==12u)s->body_reads++; /* The cyclic fixtures have two raw rows. */
    if(at==s->fail_at||at>ROM_SIZE||length>ROM_SIZE-at)return WF_BLOCKED;
    memcpy(out,s->rom+at,length);return WF_OK;
}
static void pose(unsigned index,uint32_t address){put16(0x39014u+index*2u,(uint16_t)(address-0x40000u));}
static void rows(uint32_t at,unsigned count,unsigned seed){
    for(unsigned row=0;row<count;row++){
        uint8_t *r=source.rom+at+row*6u;
        r[0]=(uint8_t)(row*13u);r[1]=(uint8_t)(row*7u);r[2]=(uint8_t)(row&15u);
        r[3]=(uint8_t)(seed+row);r[4]=(uint8_t)(seed+row*3u);r[5]=(uint8_t)(seed+row*5u);
    }
}
static void packed(uint32_t at,unsigned count){
    uint32_t q=at+2;
    put16(at,(uint16_t)(0x4000u|count));put16(q,0x04e4u);q+=2;
    for(unsigned i=0;i<count;i++)source.rom[q++]=(uint8_t)(i*13u);
    source.rom[q++]=5;
    q=(q+1u)&~1u;put16(q,0x200u);q+=2;
    q=(q+1u)&~1u;put16(q,0x300u);q+=2;
    for(unsigned i=0;i<count;i++)source.rom[q++]=(uint8_t)(i+7u);
    source.rom[q]=0x42u;
    for(unsigned i=0;i<count;i++){
        source.rom[0x6b2b0u+i]=(uint8_t)(i&15u);
        source.rom[0x733b0u+i]=(uint8_t)(i+3u);
    }
}
static void fixture(void){
    memset(&source,0,sizeof source);source.fail_at=0xffffffffu;
    for(unsigned i=0;i<256;i++)palette[i]=(uint8_t)(i&15u);
    put16(0x38f14u,0x100u);put32(0x38fb8u,0x40000u);
    put16(0x38f16u,0x200u);put32(0x38fbcu,0x50000u);
    put16(0x39114u,0x100u);put16(0x50100u,3u);rows(0x50102u,3u,20u);
    pose(0,0x40100u);put16(0x40100u,4u);rows(0x40102u,4u,3u);
    pose(1,0x40200u);packed(0x40200u,4u);
    pose(2,0x40400u);put16(0x40400u,33u);rows(0x40402u,33u,9u);
    /* The same prefix-bearing raw leaf is decoded with skip_prefix=0 and 1. */
    pose(3,0x43000u);put16(0x43000u,0x0202u);put32(0x43002u,0x02040100u);rows(0x43006u,2u,11u);
    pose(4,0x44000u);put16(0x44000u,0x0102u);rows(0x44002u,2u,13u);put32(0x4400eu,0x02040200u);
    pose(5,0x45000u);put16(0x45000u,0xc002u);put16(0x45002u,0x100u);put16(0x45004u,0x200u);
    pose(6,0x46000u);put16(0x46000u,0x8002u);put16(0x46002u,0u);put16(0x46004u,0x100u);
    source.rom[0x46008u]=10u;source.rom[0x46009u]=20u;
    pose(7,0x47000u);put16(0x47000u,0x0201u);put32(0x47002u,0x01043000u);rows(0x47006u,1u,15u);
    pose(8,0x48000u);packed(0x48000u,33u);
}
static wf_r18_metasprite_ctx_t context(wf_r18_sprite_record_t *records){
    wf_r18_metasprite_ctx_t c={0};
    c.read_rom=read_rom;c.user=&source;c.palette_map=palette;c.records=records;c.capacity=1024;
    return c;
}

static wf_r18_metasprite_cache_t cache;
static int compare(unsigned index,unsigned mirror,uint16_t x,uint16_t y,unsigned partner,unsigned seed){
    wf_r18_metasprite_ctx_t a=context(plain_records),b=context(cached_records);
    wf_status_t first,second;
    a.d6=b.d6=0x12340000u+seed;a.d7=b.d7=0x98760000u+seed;
    a.count=b.count=2;memset(plain_records,0xa5,sizeof plain_records);memset(cached_records,0xa5,sizeof cached_records);
    b.cache=&cache;
    first=wf_r18_metasprite_emit(&a,0,(uint16_t)(index|(mirror<<15)),x,y,(uint16_t)partner);
    second=wf_r18_metasprite_emit(&b,0,(uint16_t)(index|(mirror<<15)),x,y,(uint16_t)partner);
    CHECK(first==second&&a.count==b.count&&a.d6==b.d6&&a.d7==b.d7);
    CHECK(!memcmp(plain_records,cached_records,a.count*sizeof *plain_records));
    return 0;
}

int main(void){
    wf_r18_metasprite_ctx_t c;
    unsigned cold,warm;
    fixture();c=context(cached_records);
    wf_r18_metasprite_cache_reset(&cache);c.cache=&cache;
    CHECK(wf_r18_metasprite_emit(&c,0,1,150,100,1)==WF_OK);cold=source.reads;
    source.reads=0;c.count=0;
    CHECK(wf_r18_metasprite_emit(&c,0,1,150,100,1)==WF_OK);warm=source.reads;
    printf("Repeated packed body: cold=%u reads, warm=%u reads\n",cold,warm);fflush(stdout);
    CHECK(warm<cold);
    CHECK(sizeof cache<=7u*1024u);
    /* Palette, clipping, mirror, coordinates, D6/D7, references, partner
     * selection and composition order are recomputed on every cache hit. */
    for(unsigned index=0;index<9;index++){
        static const uint16_t positions[][2]={{150,100},{0,0},{1000,100},{100,1000},{65535,65535}};
        for(unsigned pass=0;pass<2;pass++){
            for(unsigned i=0;i<256;i++)palette[i]=(uint8_t)((i+pass*7u)&15u);
            for(unsigned position=0;position<5;position++)for(unsigned mirror=0;mirror<2;mirror++){
                CHECK(!compare(index,mirror,positions[position][0],positions[position][1],1,index*20u+position));
            }
        }
    }
    CHECK(!compare(6,0,150,100,0,42));CHECK(!compare(6,1,150,100,2,43));
    /* Raw and packed bodies over 32 rows always take the original reader. */
    for(unsigned index=2;index<=8;index+=6){
        wf_r18_metasprite_cache_reset(&cache);c=context(cached_records);c.cache=&cache;source.reads=0;
        CHECK(wf_r18_metasprite_emit(&c,0,(uint16_t)index,150,100,1)==WF_OK);cold=source.reads;
        source.reads=0;c.count=0;
        CHECK(wf_r18_metasprite_emit(&c,0,(uint16_t)index,150,100,1)==WF_OK);CHECK(source.reads==cold);
        for(unsigned i=0;i<WF_R18_METASPRITE_CACHE_ENTRIES;i++)CHECK(!cache.entries[i].valid);
    }
    /* Failed decode rolls context back and does not make a partial body reusable. */
    wf_r18_metasprite_cache_reset(&cache);c=context(cached_records);c.cache=&cache;c.d6=123;c.d7=456;
    source.fail_at=0x40204u;CHECK(wf_r18_metasprite_emit(&c,0,1,150,100,1)==WF_BLOCKED);
    CHECK(c.count==0&&c.d6==123&&c.d7==456);
    for(unsigned i=0;i<WF_R18_METASPRITE_CACHE_ENTRIES;i++)CHECK(!cache.entries[i].valid);
    source.fail_at=0xffffffffu;CHECK(!compare(1,0,150,100,1,99));
    /* A source change becomes visible after the caller's explicit reset. */
    source.rom[0x40204u]=77u;wf_r18_metasprite_cache_reset(&cache);
    CHECK(!compare(1,0,150,100,1,100));CHECK(!compare(1,1,0,0,1,101));
    wf_r18_metasprite_cache_reset(NULL);
    /* Sixteen changing actors/compositions must stay warm across draw rounds.
     * Each hit still needs five metadata reads, but no raw-body read. */
    for(unsigned i=0;i<33;i++){
        uint32_t at=0x4a000u+i*0x100u;pose(10u+i,at);put16(at,2u);rows(at+2u,2u,40u+i);
    }
    wf_r18_metasprite_cache_reset(&cache);
    c=context(cached_records);c.cache=&cache;
    for(unsigned i=0;i<16;i++){
        c.count=0;CHECK(wf_r18_metasprite_emit(&c,0,(uint16_t)(10u+i),150,100,1)==WF_OK);
    }
    source.reads=source.body_reads=0;
    for(unsigned round=0;round<3;round++)for(unsigned i=0;i<16;i++){
        c.count=0;CHECK(wf_r18_metasprite_emit(&c,0,(uint16_t)(10u+i),150,100,1)==WF_OK);
    }
    printf("Cyclic sixteen-body cache: warm=%u metadata reads, %u body reads\n",source.reads,source.body_reads);
    CHECK(source.body_reads==0u&&source.reads==3u*16u*5u);
    /* The complete working set remains resident; insertion 33 evicts only
     * the oldest FIFO entry. Equal headers at different addresses cannot alias. */
    wf_r18_metasprite_cache_reset(&cache);c=context(cached_records);c.cache=&cache;
    for(unsigned i=0;i<32;i++){
        c.count=0;CHECK(wf_r18_metasprite_emit(&c,0,(uint16_t)(10u+i),150,100,1)==WF_OK);
    }
    source.reads=source.body_reads=0;
    for(unsigned i=0;i<32;i++){
        c.count=0;CHECK(wf_r18_metasprite_emit(&c,0,(uint16_t)(10u+i),150,100,1)==WF_OK);
    }
    CHECK(source.body_reads==0u&&source.reads==32u*5u);
    for(unsigned i=0;i<32;i++)CHECK(!compare(10u+i,0,150,100,1,i));
    c.count=0;CHECK(wf_r18_metasprite_emit(&c,0,42,150,100,1)==WF_OK);
    source.reads=source.body_reads=0;c.count=0;
    CHECK(wf_r18_metasprite_emit(&c,0,11,150,100,1)==WF_OK);
    CHECK(source.body_reads==0u&&source.reads==5u); /* Second entry is still warm. */
    c=context(cached_records);c.cache=&cache;source.reads=0;
    source.body_reads=0;
    CHECK(wf_r18_metasprite_emit(&c,0,10,150,100,1)==WF_OK);cold=source.reads;
    CHECK(source.body_reads==1u);
    c.count=0;source.reads=0;CHECK(wf_r18_metasprite_emit(&c,0,10,150,100,1)==WF_OK);
    CHECK(source.reads<cold); /* Insertion 33 evicted the first FIFO entry. */
    /* The header is part of the key, even at the same address. */
    wf_r18_metasprite_cache_reset(&cache);cache.entries[0].valid=1;
    cache.entries[0].address=0x40100u;cache.entries[0].header=5u;cache.entries[0].skip_prefix=1u;
    memset(cache.entries[0].rows,0xff,sizeof cache.entries[0].rows);
    CHECK(!compare(0,0,150,100,1,7));
    puts("PASS metasprite cache: <=7KiB, cyclic sixteen bodies, transforms, palettes, references, FIFO eviction33, >32 rows, failure and reset");
    return 0;
}
