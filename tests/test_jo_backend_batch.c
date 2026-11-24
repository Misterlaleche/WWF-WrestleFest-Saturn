#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "wf_saturn_yaul_backend.h"

/* A weak declaration lets the pre-change binary fail on the absent batch
 * contract, instead of mistaking a compile/link failure for a test result. */
extern wf_status_t wf_jo_backend_emit_sprites(wf_saturn_yaul_backend_t *,
    const wf_vdp1_sprite_cmd_t *,unsigned) __attribute__((weak));
extern uint64_t wf_jo_backend_frame_digest(void);
static uint8_t pixels[5][128],stage[256*128];
static wf_saturn_yaul_backend_t backend;
static wf_vdp1_sprite_cmd_t sprites[256];

static const wf_saturn_render_ops_t *start(unsigned count)
{
    const wf_saturn_render_ops_t *ops=wf_saturn_yaul_render_ops();
    wf_saturn_yaul_backend_init(&backend,stage,sizeof stage);
    assert(ops->begin_frame(&backend,5,0,count)==WF_OK);
    for(unsigned i=0;i<5;i++)
        assert(ops->upload_tile_16x16_4bpp(&backend,(uint16_t)i,pixels[i],128)==WF_OK);
    return ops;
}

int main(void)
{
    assert(wf_jo_backend_emit_sprites && "Jo batch command contract is missing");
    /* Four crop geometries and a fully transparent tile. Pen F is real. */
    for(unsigned tile=0;tile<4;tile++)for(unsigned y=1+tile;y<15-tile;y++)
        for(unsigned x=0;x<16;x++)if(tile>1||(tile==0?x<8:x>=8))
            pixels[tile][y*8+x/2]|=(uint8_t)((x&1)?15:0xf0);
    for(unsigned i=0;i<256;i++)sprites[i]=(wf_vdp1_sprite_cmd_t){
        .x=(int16_t)((int)(i*37u%352u)-16),.y=(int16_t)((int)(i*61u%272u)-16),
        .stream_tile=(uint16_t)(i%5u),.cram_slot=(uint8_t)(i%16u),
        .flip_x=(uint8_t)(i&1u),.flip_y=(uint8_t)((i>>1)&1u)};
    const wf_saturn_render_ops_t *ops=start(256);
    for(unsigned i=0;i<256;i++)assert(ops->emit_sprite_16x16(&backend,sprites+i)==WF_OK);
    assert(ops->end_frame(&backend)==WF_OK);
    uint64_t expected=wf_jo_backend_frame_digest();assert(expected);
    ops=start(256);
    /* Batches can follow one another without changing draw order. */
    assert(wf_jo_backend_emit_sprites(&backend,sprites,73)==WF_OK);
    assert(wf_jo_backend_emit_sprites(&backend,sprites+73,183)==WF_OK);
    assert(backend.emitted_cmds==256);
    assert(ops->end_frame(&backend)==WF_OK);
    assert(wf_jo_backend_frame_digest()==expected);
    assert(wf_jo_backend_emit_sprites(&backend,sprites,1)==WF_BAD_STATE);
    ops=start(1);
    assert(wf_jo_backend_emit_sprites(&backend,sprites,2)==WF_BAD_STATE);
    assert(wf_jo_backend_emit_sprites(&backend,NULL,1)==WF_BAD_STATE);
    sprites[0].stream_tile=5;
    assert(wf_jo_backend_emit_sprites(&backend,sprites,1)==WF_BAD_STATE);
    sprites[0].stream_tile=0;sprites[0].cram_slot=16;
    assert(wf_jo_backend_emit_sprites(&backend,sprites,1)==WF_BLOCKED);
    ops->abort_frame(&backend);
    puts("Jo command batches: identical order, palettes, flips, cropped pixels and checked bounds: PASS");
    return 0;
}
