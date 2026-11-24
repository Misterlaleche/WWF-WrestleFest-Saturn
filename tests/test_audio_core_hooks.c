/* Exercise actual bulk-write/bind routines with the real audio adapter.
 * Unused cabinet owners are removed by section GC; this file introduces no
 * production-only test entry points. */
#include <assert.h>
#include <string.h>
#include "wf_r18_frontend_driver.c"
#include "wf_r18_ring.h"
#include "wf_jo_video.h"

/* The audio fixture has no submitted video epoch. Scene binding must still
 * call the production closure boundary, which is immediately complete here. */
wf_status_t wf_jo_video_drain_overlap(wf_status_t (*service)(void *),void *user)
{assert(!service&&!user);return WF_OK;}

void wf_audio_test_bulk_writes(void)
{
    static wf_runtime_t runtime;
    static wf_r18_video_t video;
    wf_r18_frontend_driver_t driver;
    memset(&driver,0,sizeof driver);memset(&runtime,0,sizeof runtime);
    memset(&video,0,sizeof video);driver.runtime=&runtime;driver.video=&video;
    runtime.g.frame_counter=123;
    for(unsigned i=0;i<512u;++i)
        assert(store_bus(&driver,0xc0000u+i,(i*3u)&255u,1)==WF_OK);
    for(unsigned i=0;i<512u;++i)assert(video.fg0[i]==(uint8_t)(i*3u));
    assert(runtime.g.frame_counter==123u);
}

#define TEST_TILES 256u
#define TEST_PAYLOAD 1824u
static uint8_t pack[TEST_PAYLOAD+TEST_TILES*130u+1024u] __attribute__((aligned(4)));
static uint8_t characters[TEST_TILES*128u+1u] __attribute__((aligned(4)));
static void be16(uint8_t *p,unsigned v){p[0]=(uint8_t)(v>>8);p[1]=(uint8_t)v;}
static void be32(uint8_t *p,unsigned v){be16(p,v>>16);be16(p+2,v);}

void wf_audio_test_ring_bind(void)
{
    memset(pack,0,sizeof pack);memcpy(pack,"R18R",4);be16(pack+4,2);
    be16(pack+10,TEST_TILES);be32(pack+12,32);be32(pack+16,TEST_PAYLOAD);
    be32(pack+20,TEST_PAYLOAD+TEST_TILES*130u);be32(pack+24,160);be32(pack+28,128);
    for(unsigned page=0;page<128u;++page)be32(pack+32+page*4u,544);
    for(unsigned cell=0;cell<320u;++cell)be16(pack+544+cell*4u+2u,cell%TEST_TILES);
    for(unsigned tile=0;tile<TEST_TILES;++tile){
        be16(pack+TEST_PAYLOAD+TEST_TILES*128u+tile*2u,tile);
        for(unsigned i=0;i<128u;++i)pack[TEST_PAYLOAD+tile*128u+i]=(uint8_t)(tile+i*7u);
    }
    assert(wf_r18_ring_bind(pack,sizeof pack)==0);
}

void wf_audio_test_ring_convert(unsigned unaligned)
{
    assert(unaligned<=1u);uint8_t *out=characters+unaligned;
    assert(wf_r18_ring_characters(out,sizeof characters-unaligned)==0);
    for(unsigned tile=0;tile<TEST_TILES;++tile)
        for(unsigned quadrant=0;quadrant<4u;++quadrant)
            for(unsigned row=0;row<8u;++row)
                for(unsigned byte=0;byte<4u;++byte){
                    unsigned original=(quadrant/2u*8u+row)*8u+quadrant%2u*4u+byte;
                    assert(out[tile*128u+quadrant*32u+row*4u+byte]==(uint8_t)(tile+original*7u));
                }
}
