#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "wf_saturn_yaul_backend.h"
#include "wf_jo_video.h"

static wf_saturn_yaul_backend_t backend,other;
static uint8_t stage[WF_JO_TEXTURE_STAGE_BYTES],other_stage[128],pixels[128];
static uint16_t sources[256];
static uint8_t touched[256];
static unsigned read_count,protect_count,fail_read,optimized,records,restored_frames;
static unsigned restore_count,private_consumptions;
static const wf_saturn_render_ops_t *ops;
extern uint64_t wf_jo_backend_frame_digest(void);
extern void wf_jo_test_reset_texture_cache(void);
extern uint64_t wf_jo_test_cache_digest(void);
extern wf_status_t __real_wf_jo_video_texture_protect(unsigned,uint16_t,wf_jo_texture_ref_t *,unsigned *);
wf_status_t __wrap_wf_jo_video_texture_protect(unsigned logical,uint16_t source,wf_jo_texture_ref_t *ref,unsigned *hit)
{++protect_count;return __real_wf_jo_video_texture_protect(logical,source,ref,hit);}
extern int __real_wf_jo_video_texture_restore_preparation(const void *,uint32_t,const uint8_t *);
int __wrap_wf_jo_video_texture_restore_preparation(const void *owner,uint32_t generation,const uint8_t *active)
{
    int restored=__real_wf_jo_video_texture_restore_preparation(owner,generation,active);
    restore_count+=!!restored;return restored;
}
static int consume_prepared(void)
{
#ifdef TEST_NO_PREPARED_CONSUME
    return 0;
#else
    return wf_jo_backend_consume_prepared_rom_tiles(&backend);
#endif
}

typedef struct result {
    wf_saturn_yaul_backend_t backend;
    uint64_t commands,cache;
    unsigned reads;
    uint8_t stage[WF_JO_TEXTURE_STAGE_BYTES];
} result_t;
static result_t reference[64];
static wf_status_t read_tile(uint16_t source,const uint8_t **out)
{
    ++read_count;if(fail_read)return WF_BLOCKED;
    memset(pixels,0,sizeof pixels);
    unsigned top=source%5u,height=8u+source%4u,half=(source>>3)%3u;
    for(unsigned y=top;y<top+height;y++)for(unsigned x=0;x<8;x++)
        if(half==2u||(half==0u?x<4u:x>=4u))pixels[y*8u+x]=(uint8_t)(source*19u+y*13u+x*37u+1u);
    *out=pixels;return WF_OK;
}
static void capture(void)
{
    result_t got;memset(&got,0,sizeof got);got.backend=backend;
    got.commands=wf_jo_backend_frame_digest();got.cache=wf_jo_test_cache_digest();
    got.reads=read_count;memcpy(got.stage,stage,sizeof stage);
    assert(records<64);
    if(!optimized)reference[records]=got;
    else if(memcmp(reference+records,&got,sizeof got)){
        fprintf(stderr,"preparation differential mismatch at frame %u\n",records);assert(0);
    }
    ++records;
}
static wf_status_t prepare(unsigned unchanged,uint32_t generation,unsigned count)
{
#ifndef TEST_BASELINE
    if(optimized)return wf_jo_backend_prepare_rom_tiles(&backend,sources,touched,7,count,unchanged,generation);
#else
    (void)unchanged;(void)generation;
#endif
    return wf_jo_backend_protect_rom_tiles(&backend,sources,touched,7,count);
}
static void frame(unsigned unchanged,uint32_t generation,unsigned count,unsigned expect_restore)
{
    unsigned active=0;for(unsigned i=0;i<count;i++)active+=touched[i]==7;
    assert(ops->begin_frame(&backend,count,0,active)==WF_OK);
    unsigned before=protect_count,restores=restore_count;
    assert(prepare(unchanged,generation,count)==WF_OK);
    if(optimized&&expect_restore&&active){
        assert(protect_count==before&&"unchanged preparation still walks physical protections");++restored_frames;
    }else if(active)assert(protect_count>before);
    assert(backend.emitted_tiles==0);
    for(unsigned i=0;i<sizeof backend.dirty;i++)assert(!backend.dirty[i]);
    unsigned consumed=0;
    if(optimized==2){
        consumed=(unsigned)consume_prepared();
        assert(consumed==(restore_count!=restores)&&"restored preparation still consumes each logical slot");
        assert(!consume_prepared()); /* The private permission is one-use. */
        private_consumptions+=consumed;
    }
    if(!consumed)assert(wf_jo_backend_consume_rom_tiles(&backend,sources,touched,7,count,read_tile)==WF_OK);
    wf_vdp1_sprite_cmd_t commands[256];unsigned n=0;
    for(unsigned i=0;i<count;i++)if(touched[i]==7)
        commands[n++]=(wf_vdp1_sprite_cmd_t){.stream_tile=(uint16_t)i,.x=(int16_t)(i*3u+records),
            .y=(int16_t)(8u+i%200u),.flip_x=(uint8_t)(records&1u),.flip_y=(uint8_t)(i&1u),.cram_slot=(uint8_t)(i&15u)};
    assert(wf_jo_backend_emit_sprites(&backend,commands,n)==WF_OK);
    assert(ops->end_frame(&backend)==WF_OK);capture();
}
static void run(unsigned fast)
{
    optimized=fast;records=read_count=protect_count=fail_read=0;
    wf_jo_test_reset_texture_cache();memset(stage,0xa5,sizeof stage);
    wf_saturn_yaul_backend_init(&backend,stage,sizeof stage);
    for(unsigned i=0;i<256;i++){sources[i]=(uint16_t)(1000u+i);touched[i]=(uint8_t)(i%3u?7:0);}
    frame(0,1,48,0);frame(1,1,48,1);frame(1,1,48,1);
    touched[3]=7;frame(1,1,48,0);frame(1,1,48,1);
    sources[4]=60000;frame(0,1,48,0);frame(1,1,48,1);
    frame(1,2,48,0);frame(1,2,48,1); /* A changed asset binding revokes the token. */
    frame(1,0,48,0);frame(1,0,48,0);frame(1,2,48,0);frame(1,2,48,1);
    assert(wf_jo_video_texture_invalidate()==WF_OK);frame(1,2,48,0);frame(1,2,48,1);
    assert(ops->begin_frame(&backend,48,0,0)==WF_OK);
    assert(prepare(1,2,48)==WF_OK);ops->abort_frame(&backend);
    frame(1,2,48,0);frame(1,2,48,1);
    /* A failed callback must not bless its partially changed logical plan. */
    sources[1]=60001;assert(ops->begin_frame(&backend,48,0,0)==WF_OK);
    assert(prepare(0,2,48)==WF_OK);fail_read=1;
    assert(wf_jo_backend_consume_rom_tiles(&backend,sources,touched,7,48,read_tile)==WF_BLOCKED);
    fail_read=0;ops->abort_frame(&backend);frame(1,2,48,0);frame(1,2,48,1);
    /* Prepared but unconsumed slots must not be remembered as a whole frame. */
    assert(ops->begin_frame(&backend,48,0,0)==WF_OK);assert(prepare(1,2,48)==WF_OK);
    for(unsigned i=0;i<48;i++)assert(wf_jo_backend_skip_tile(&backend,(uint16_t)i)==WF_OK);
    assert(ops->end_frame(&backend)==WF_OK);capture();frame(1,2,48,0);frame(1,2,48,1);
    /* A separate public preparation revokes a private claim even on hits. */
    assert(ops->begin_frame(&backend,48,0,0)==WF_OK);assert(prepare(1,2,48)==WF_OK);
    assert(wf_jo_backend_protect_rom_tiles(&backend,sources,touched,7,48)==WF_OK);
    assert(wf_jo_backend_consume_rom_tiles(&backend,sources,touched,7,48,read_tile)==WF_OK);
    assert(ops->end_frame(&backend)==WF_OK);capture();frame(1,2,48,0);frame(1,2,48,1);
    /* Owner replacement and initialization keep their old invalidation ABI. */
    wf_saturn_yaul_backend_init(&other,other_stage,sizeof other_stage);
    assert(ops->begin_frame(&other,0,0,0)==WF_OK&&ops->end_frame(&other)==WF_OK);
    frame(1,2,48,0);frame(1,2,48,1);
    wf_saturn_yaul_backend_init(&backend,stage,sizeof stage);frame(1,2,48,0);frame(1,2,48,1);
    /* An anonymous byte-upload publication cannot supply immutable metadata. */
    assert(ops->begin_frame(&backend,1,0,0)==WF_OK);
    assert(ops->upload_tile_16x16_4bpp(&backend,0,pixels,128)==WF_OK&&ops->end_frame(&backend)==WF_OK);
    capture();frame(1,2,48,0);frame(1,2,48,1);
    frame(1,2,0,0);frame(1,2,0,0);frame(1,2,256,0);frame(1,2,256,1);
    frame(1,2,2,0);frame(1,2,2,1);
}
int main(void)
{
    ops=wf_saturn_yaul_render_ops();run(0);unsigned frames=records;run(1);run(2);
    assert(frames==records&&restored_frames>=15);
    assert(private_consumptions>=15);
    assert(reference[1].commands!=reference[2].commands); /* Positions/flips still rebuild. */
    printf("PASS preparation backend: %u complete frames per path, %u restored, %u private consumes; exact commands/pixels/cache/counters, failures and invalidations\n",frames,restored_frames,private_consumptions);
    return 0;
}
