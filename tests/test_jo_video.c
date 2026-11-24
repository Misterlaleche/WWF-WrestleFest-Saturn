#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "wf_jo_video.h"

/* Equality is deliberately unsafe: automatic VDP1 can draw that table again. */
static void bank_lifetime(void)
{
    assert(wf_jo_bank_reusable(0, 0, 0, 0));
    assert(!wf_jo_bank_reusable(1, 10, 0, 0));
    assert(!wf_jo_bank_reusable(1, 10, 1, 9));
    assert(!wf_jo_bank_reusable(1, 10, 1, 10));
    assert(wf_jo_bank_reusable(1, 10, 1, 11));
    assert(wf_jo_bank_reusable(1, 0xffffffffu, 1, 0));
    assert(!wf_jo_bank_reusable(1, 1, 1, 0xffffffffu));
}

static void texture_bounds(void)
{
    uint32_t address=0xdeadbeefu;
    assert(wf_jo_texture_offset(0,0,&address)==WF_OK && address==0x10000u);
    assert(wf_jo_texture_offset(3,255,&address)==WF_OK && address==0x2ff80u);
    assert(wf_jo_texture_offset(4,0,&address)==WF_BLOCKED);
    assert(wf_jo_texture_offset(0,256,&address)==WF_BLOCKED);
    assert(wf_jo_texture_offset(0,0,NULL)==WF_BAD_STATE);
}

static unsigned pixel(const uint8_t *p,unsigned pitch,unsigned x,unsigned y)
{ unsigned v=p[y*pitch+x/2]; return x&1u?v&15u:v>>4; }

/* An independently rendered 16x16 image must survive packing and every flip.
 * A coordinate adjustment, SRCA, row-pitch or pen-F regression breaks this. */
static void cropped_image(unsigned left,unsigned right,unsigned top,unsigned bottom)
{
    uint8_t source[128]={0},packed[128];
    for(unsigned y=top;y<bottom;y++)for(unsigned x=left;x<right;x++){
        unsigned value=1+(x+y)%15;
        source[y*8+x/2]|=(uint8_t)(x&1u?value:value<<4);
    }
    memcpy(packed,source,128);
    wf_jo_crop_t crop;
    wf_jo_crop_tile(packed,&crop);
    assert(crop.height==bottom-top);
    for(unsigned flip=0;flip<4;flip++){
        wf_vdp1_sprite_cmd_t in={0}; wf_jo_command_t out;
        memset(&out,0xa5,sizeof out);
        in.x=-9;in.y=7;in.cram_slot=3;in.flip_x=flip&1;in.flip_y=flip>>1;
        assert(wf_jo_sprite_build(&in,&crop,0x18000,&out)==WF_OK);
        assert(out.pmod==0x80 && out.colr==0x130);
        assert(!out.link && !out.xb && !out.yb && !out.xc && !out.yc &&
               !out.xd && !out.yd && !out.grda && !out.reserved);
        unsigned width=(out.size>>8)*8,height=out.size&255;
        unsigned address=((unsigned)out.srca*8)-0x18000;
        for(unsigned y=0;y<16;y++)for(unsigned x=0;x<16;x++){
            unsigned expected=pixel(source,8,in.flip_x?15-x:x,in.flip_y?15-y:y);
            int dx=in.x+(int)x-out.xa,dy=in.y-8+(int)y-out.ya;
            unsigned actual=0;
            if(dx>=0&&dy>=0&&(unsigned)dx<width&&(unsigned)dy<height)
                actual=pixel(packed+address,width/2,
                             in.flip_x?width-1-(unsigned)dx:(unsigned)dx,
                             in.flip_y?height-1-(unsigned)dy:(unsigned)dy);
            assert(actual==expected);
        }
    }
}

int main(void)
{
    bank_lifetime();texture_bounds();
    cropped_image(8,16,3,14);cropped_image(0,8,1,15);
    cropped_image(3,12,2,12);cropped_image(0,16,0,16);
    uint8_t empty[128]={0};wf_jo_crop_t crop;
    wf_jo_crop_tile(empty,&crop);assert(crop.height==0);
    puts("Jo video lifetime, bounds, crops and flip pixels: PASS");
    return 0;
}
