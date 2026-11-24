#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "wf_saturn_yaul_backend.h"
#include "wf_jo_video.h"

static wf_saturn_yaul_backend_t backend,other;
static uint8_t stage[WF_JO_TEXTURE_STAGE_BYTES],pixels[128];
static uint16_t sources[8]={1000,1001,1002,1003,1004,1005,1006,1007};
static uint8_t touched[8]={0,7,0,7,7,0,0,7};
static unsigned reads;
static const wf_saturn_render_ops_t *ops;
extern void wf_jo_test_reset_texture_cache(void);
static wf_status_t read_tile(uint16_t source,const uint8_t **out)
{++reads;memset(pixels,(int)(source|0x11u),sizeof pixels);*out=pixels;return WF_OK;}
static void warm(unsigned count)
{
    assert(ops->begin_frame(&backend,count,0,0)==WF_OK);
    assert(wf_jo_backend_prepare_rom_tiles(&backend,sources,touched,7,count,0,3)==WF_OK);
    assert(!wf_jo_backend_consume_prepared_rom_tiles(&backend));
    assert(wf_jo_backend_consume_rom_tiles(&backend,sources,touched,7,count,read_tile)==WF_OK);
    assert(ops->end_frame(&backend)==WF_OK);
}
static void restored(unsigned count,unsigned commands)
{
    assert(ops->begin_frame(&backend,count,0,commands)==WF_OK);
    assert(wf_jo_backend_prepare_rom_tiles(&backend,sources,touched,7,count,1,3)==WF_OK);
    assert(!backend.emitted_tiles&&!backend.dirty[0]);
}
int main(void)
{
    ops=wf_saturn_yaul_render_ops();wf_jo_test_reset_texture_cache();
    wf_saturn_yaul_backend_init(&backend,stage,sizeof stage);
    warm(8);restored(8,1);unsigned before=reads;
    backend.texture_reused=UINT32_MAX-2u;
    assert(!wf_jo_backend_consume_prepared_rom_tiles(NULL));
    assert(!wf_jo_backend_consume_prepared_rom_tiles(&other));
    assert(wf_jo_backend_consume_prepared_rom_tiles(&backend));
    assert(backend.emitted_tiles==8&&backend.texture_reused==1&&backend.dirty[0]==0x9a&&reads==before);
    assert(!wf_jo_backend_consume_prepared_rom_tiles(&backend));
    wf_vdp1_sprite_cmd_t command={.stream_tile=0};
    assert(ops->emit_sprite_16x16(&backend,&command)==WF_BAD_STATE);
    command.stream_tile=1;assert(ops->emit_sprite_16x16(&backend,&command)==WF_OK);
    assert(ops->end_frame(&backend)==WF_OK);
    for(unsigned mutation=0;mutation<7;mutation++){
        warm(8);restored(8,0);wf_jo_texture_ref_t ref;unsigned hit;
        switch(mutation){
        case 0:assert(wf_jo_backend_protect_rom_tiles(&backend,sources,touched,7,8)==WF_OK);break;
        case 1:assert(wf_jo_backend_skip_tile(&backend,0)==WF_OK);break;
        case 2:assert(wf_saturn_yaul_upload_rom_tile(&backend,0,sources[0],read_tile)==WF_OK);break;
        case 3:assert(wf_jo_backend_consume_rom_tiles(&backend,sources,touched,7,8,read_tile)==WF_OK);break;
        case 4:assert(wf_jo_video_texture_protect(1,sources[1],&ref,&hit)==WF_OK&&hit);break;
        case 5:assert(wf_jo_video_texture_ready(1,sources[1],1,0,16)==WF_BAD_STATE);break;
        case 6: /* This unallocated tail has no in-flight references. */
            assert(wf_jo_video_texture_transfer(WF_JO_GPU_TILES-1u,pixels,1)==WF_OK);break;
        }
        assert(!wf_jo_backend_consume_prepared_rom_tiles(&backend));ops->abort_frame(&backend);
        assert(!wf_jo_backend_consume_prepared_rom_tiles(&backend));
    }
    warm(8);restored(8,0);backend.expected_tiles=7;
    assert(!wf_jo_backend_consume_prepared_rom_tiles(&backend));backend.expected_tiles=8;
    assert(wf_jo_backend_consume_prepared_rom_tiles(&backend));assert(ops->end_frame(&backend)==WF_OK);
    warm(0);restored(0,0);
    assert(wf_jo_backend_consume_rom_tiles(&backend,sources,touched,7,0,read_tile)==WF_OK);
    assert(!wf_jo_backend_consume_prepared_rom_tiles(&backend));ops->abort_frame(&backend);
    warm(0);restored(0,0);
    assert(wf_jo_backend_consume_prepared_rom_tiles(&backend));
    assert(!wf_jo_backend_consume_prepared_rom_tiles(&backend));assert(ops->end_frame(&backend)==WF_OK);
    warm(8);restored(8,0);wf_saturn_yaul_backend_init(&backend,stage,sizeof stage);
    assert(!wf_jo_backend_consume_prepared_rom_tiles(&backend));
    puts("PASS private consume: one-use, owner/count/empty guards, counter wrap, exact holes, no callbacks, public/cache mutations, transfer, abort and init");
    return 0;
}
