#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "wf_saturn_yaul_backend.h"
#include "wf_jo_video.h"

static wf_saturn_yaul_backend_t backend,other;
static uint8_t stage[WF_JO_TEXTURE_STAGE_BYTES],other_stage[128],pixels[128];
static uint16_t sources[256],reads[256];
static uint8_t touched[256];
static unsigned read_count,action,triggered,epoch,trigger_min_slot;
static const wf_saturn_render_ops_t *ops;
extern uint64_t wf_jo_backend_frame_digest(void);
#ifdef WF_JO_TEST_RESET_CACHE
/* The extended runner supplies a test-only wrapper around the real portable
 * video source. Each side starts with identical physical allocation, so a
 * cache wrap cannot create an unrelated extra DMA transfer in one run. */
extern void wf_jo_test_reset_texture_cache(void);
#endif
typedef struct result {
    int consume,end;
    unsigned read_count,emitted,expected,active;
    uint32_t bytes,transfers,reused;
    uint16_t reads[256],sources[256];
    uint8_t dirty[32],resident[32],identity[32],meta[256],height[256];
    uint8_t stage[WF_JO_TEXTURE_STAGE_BYTES];
    uint64_t digest,pixels;
} result_t;
static uint32_t state=0x12345678u;
static uint32_t random32(void){state^=state<<13;state^=state>>17;state^=state<<5;return state;}
static wf_status_t read_tile(uint16_t source,const uint8_t **out)
{
    assert(read_count<256);reads[read_count++]=source;
    unsigned slot=backend.emitted_tiles;
    if(action&&!triggered&&slot>=trigger_min_slot){
        triggered=1;
        switch(action){
        case 1:return WF_BLOCKED;
        case 2:*out=NULL;return WF_OK;
        case 3:backend.active=0;break;
        case 4:ops->abort_frame(&backend);break;
        case 5:backend.expected_tiles=slot+7u;break;
        case 6:backend.expected_tiles+=2u;break;
        case 7:++backend.emitted_tiles;break;
        case 8:if(slot+1u<48u)sources[slot+1u]^=0x4000u;break;
        case 9:if(slot+1u<48u)touched[slot+1u]=0;break;
        case 10:if(slot+1u<48u)backend.rom_source[slot+1u]^=0x4000u;break;
        case 11:if(slot+1u<48u)backend.resident[(slot+1u)>>3]&=(uint8_t)~wf_jo_cache_mask(slot+1u);break;
        case 12:assert(wf_saturn_yaul_upload_rom_tile(&backend,(uint16_t)(slot+1u),source,read_tile)==WF_BAD_STATE);break;
        case 13:
            assert(slot==0);ops->abort_frame(&backend);
            wf_saturn_yaul_backend_init(&backend,stage,sizeof stage);
            assert(ops->begin_frame(&backend,48,0,0)==WF_OK);break;
        case 14:
            ops->abort_frame(&backend);wf_saturn_yaul_backend_init(&other,other_stage,sizeof other_stage);
            assert(ops->begin_frame(&other,1,0,0)==WF_OK);break;
        case 15:backend.expected_tiles=slot;break;
        case 16:backend.texture_reused^=UINT32_MAX;break;
        case 17:backend.texture_reused=UINT32_MAX;return WF_BLOCKED;
        default:assert(0);
        }
    }
    memset(pixels,0,sizeof pixels);
    unsigned top=source%5u,height=8u+source%4u,half=(source>>3)%3u;
    for(unsigned y=top;y<top+height;y++)for(unsigned x=0;x<8;x++)
        if(half==2u||(half==0u?x<4u:x>=4u))pixels[y*8u+x]=(uint8_t)(source*19u+y*13u+x*37u+1u);
    *out=pixels;return WF_OK;
}
static wf_status_t scalar(unsigned count)
{
    for(unsigned slot=0;slot<count;slot++){
        wf_status_t status=touched[slot]==epoch?
            wf_saturn_yaul_upload_rom_tile(&backend,(uint16_t)slot,sources[slot],read_tile):
            wf_jo_backend_skip_tile(&backend,(uint16_t)slot);
        if(status!=WF_OK)return status;
    }
    return WF_OK;
}
static void run(result_t *out,unsigned bulk,uint32_t seed,unsigned mutate,unsigned stage_bytes,
                unsigned first_trigger)
{
    memset(out,0,sizeof *out);memset(stage,0xa5,sizeof stage);
    memset(sources,0,sizeof sources);memset(touched,0,sizeof touched);
    memset(reads,0,sizeof reads);read_count=action=triggered=0;epoch=1u+(seed%255u);
    trigger_min_slot=first_trigger;
#ifdef WF_JO_TEST_RESET_CACHE
    wf_jo_test_reset_texture_cache();
#endif
    wf_saturn_yaul_backend_init(&backend,stage,stage_bytes);
    assert(ops->begin_frame(&backend,48,0,0)==WF_OK);
    for(unsigned i=0;i<48;i++){sources[i]=(uint16_t)(1000u+i);touched[i]=(uint8_t)epoch;}
    assert(wf_jo_backend_protect_rom_tiles(&backend,sources,touched,epoch,48)==WF_OK);
    assert(scalar(48)==WF_OK&&ops->end_frame(&backend)==WF_OK);
    uint32_t before_bytes=backend.texture_bytes,before_transfers=backend.texture_transfers;
    state=seed;
    for(unsigned i=0;i<48;i++){
        unsigned choice=random32();sources[i]=(uint16_t)((choice&1u?1000u:2000u)+i);
        touched[i]=(uint8_t)((choice&6u)?epoch:0u);
    }
    sources[0]=2000;touched[0]=(uint8_t)epoch;
    sources[1]=1001;touched[1]=(uint8_t)epoch;
    assert(ops->begin_frame(&backend,48,0,0)==WF_OK);
    assert(wf_jo_backend_protect_rom_tiles(&backend,sources,touched,epoch,48)==WF_OK);
    memset(reads,0,sizeof reads);read_count=triggered=0;action=mutate;
    out->consume=bulk?wf_jo_backend_consume_rom_tiles(&backend,sources,touched,epoch,48,read_tile):scalar(48);
    out->read_count=read_count;out->emitted=backend.emitted_tiles;out->expected=backend.expected_tiles;out->active=backend.active;
    memcpy(out->reads,reads,sizeof reads);memcpy(out->sources,backend.rom_source,sizeof out->sources);
    memcpy(out->dirty,backend.dirty,sizeof out->dirty);memcpy(out->resident,backend.resident,sizeof out->resident);
    memcpy(out->identity,backend.rom_identity,sizeof out->identity);memcpy(out->meta,backend.crop_meta,sizeof out->meta);
    memcpy(out->height,backend.crop_height,sizeof out->height);memcpy(out->stage,stage,sizeof stage);
    out->reused=backend.texture_reused;
    if(out->consume==WF_OK){
        out->end=ops->end_frame(&backend);
        if(out->end==WF_OK){
            out->digest=wf_jo_backend_frame_digest();out->pixels=UINT64_C(14695981039346656037);
            const uint16_t *physical=wf_jo_video_texture_frame_slots();
            for(unsigned i=0;i<48;i++)if(backend.dirty[i>>3]&wf_jo_cache_mask(i)){
                assert(physical[i]<WF_JO_GPU_TILES);
                const uint8_t *p=wf_jo_video_texture_pixels(physical[i]*128u,128);assert(p);
                for(unsigned j=0;j<128;j++)out->pixels=(out->pixels^p[j])*UINT64_C(1099511628211);
            }
        }
    }
    out->bytes=backend.texture_bytes-before_bytes;
    out->transfers=backend.texture_transfers-before_transfers;
    ops->abort_frame(&backend);ops->abort_frame(&other);
}
static void prepared_guards(void)
{
    wf_saturn_yaul_backend_init(&backend,stage,sizeof stage);action=triggered=read_count=0;
    assert(ops->begin_frame(&backend,2,0,0)==WF_OK);
    assert(wf_saturn_yaul_upload_rom_tile(&backend,0,49,read_tile)==WF_OK);
    assert(wf_saturn_yaul_upload_rom_tile(&backend,1,92,read_tile)==WF_OK);
    assert(ops->end_frame(&backend)==WF_OK);
    uint16_t ids[1]={49};uint8_t active[1]={1};
    for(unsigned which=0;which<2;which++){
        assert(ops->begin_frame(&backend,1,0,1)==WF_OK);ids[0]=49;
        assert(wf_jo_backend_protect_rom_tiles(&backend,ids,active,1,1)==WF_OK);
        ids[0]=which?92:12345;
        assert(wf_jo_backend_protect_rom_tiles(&backend,ids,active,1,1)==WF_BAD_STATE);
        assert(backend.rom_source[0]==49&&!(backend.dirty[0]&1u));
        assert(ops->upload_tile_16x16_4bpp(&backend,0,pixels,128)==WF_BAD_STATE);
        unsigned before=read_count;
        assert(wf_saturn_yaul_upload_rom_tile(&backend,0,49,read_tile)==WF_OK&&read_count==before);
        wf_vdp1_sprite_cmd_t c={.stream_tile=0};
        assert(ops->emit_sprite_16x16(&backend,&c)==WF_OK&&ops->end_frame(&backend)==WF_OK);
    }
    assert(ops->begin_frame(&backend,1,0,1)==WF_OK);ids[0]=49;
    assert(wf_jo_backend_protect_rom_tiles(&backend,ids,active,1,1)==WF_OK);
    assert(wf_jo_backend_skip_tile(&backend,0)==WF_OK);
    wf_vdp1_sprite_cmd_t c={.stream_tile=0};
    assert(ops->emit_sprite_16x16(&backend,&c)==WF_BAD_STATE);
    ops->abort_frame(&backend);
    wf_saturn_yaul_backend_init(&backend,stage,127);
    assert(ops->begin_frame(&backend,1,0,0)==WF_BLOCKED);
}
int main(void)
{
    ops=wf_saturn_yaul_render_ops();prepared_guards();unsigned cases=0;
    static result_t scalar_result,bulk_result;
    unsigned phases=1,actions=16;
#ifdef WF_JO_TEST_RESET_CACHE
    phases=2;actions=18;
#endif
    for(unsigned late=0;late<phases;late++)for(unsigned mutate=0;mutate<actions;mutate++)for(unsigned n=1;n<=24;n++){
        if(late&&mutate==13)continue; /* This vector explicitly reinitializes at slot0. */
        unsigned stage_bytes=n%3u==0u?128u:n%3u==1u?256u:sizeof stage;
        uint32_t seed=0x1234u+n*0x1765u;
        run(&scalar_result,0,seed,mutate,stage_bytes,late*8u);
        run(&bulk_result,1,seed,mutate,stage_bytes,late*8u);
        if(memcmp(&scalar_result,&bulk_result,sizeof scalar_result)){
            fprintf(stderr,"bulk mismatch action=%u seed=%u stage=%u late=%u\n",mutate,seed,stage_bytes,late);
            for(size_t byte=0;byte<sizeof scalar_result;byte++)
                if(((uint8_t *)&scalar_result)[byte]!=((uint8_t *)&bulk_result)[byte]){
                    fprintf(stderr,"first byte=%zu scalar=%u bulk=%u; reused %u/%u reads %u/%u emitted %u/%u\n",
                        byte,((uint8_t *)&scalar_result)[byte],((uint8_t *)&bulk_result)[byte],
                        scalar_result.reused,bulk_result.reused,scalar_result.read_count,bulk_result.read_count,
                        scalar_result.emitted,bulk_result.emitted);break;
                }
            assert(0);
        }
        ++cases;
    }
    printf("PASS prepared/bulk scalar differential: %u cases; read failures/reentry/reinit/mutations, consumed prefixes, pixels, staging, identity guards\n",cases);
    return 0;
}
