#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "wf_vdp1_plan.h"
int main(void){
    static uint8_t tiles[4*WF_VDP1_TILE_BYTES_16X16_4BPP];
    static uint8_t pals[3*WF_VDP1_PALETTE_BYTES];
    wf_vdp1_assets_t a={tiles,sizeof tiles,pals,sizeof pals};
    wf_draw_list_t d;memset(&d,0,sizeof d);d.count=4;
    d.p[0]=(wf_draw_piece_t){.tile=1,.palette=2,.x=10,.y=20};
    d.p[1]=(wf_draw_piece_t){.tile=1,.palette=2,.x=26,.y=20,.flip_x=1};
    d.p[2]=(wf_draw_piece_t){.tile=3,.palette=2,.x=42,.y=20};
    d.p[3]=(wf_draw_piece_t){.tile=3,.palette=1,.x=58,.y=20,.flip_y=1};
    wf_vdp1_frame_plan_t p;assert(wf_vdp1_plan_build(&d,&a,&p)==WF_OK);
    assert(p.cmd_count==4);assert(p.tile_count==2);assert(p.palette_count==2);
    assert(p.tile_upload_bytes==256);assert(p.palette_upload_bytes==64);
    assert(p.cmd[0].stream_tile==p.cmd[1].stream_tile);assert(p.cmd[0].cram_slot==p.cmd[1].cram_slot);
    assert(p.cmd[2].stream_tile==p.cmd[3].stream_tile);assert(p.cmd[2].cram_slot!=p.cmd[3].cram_slot);
    d.p[0].tile=4;assert(wf_vdp1_plan_build(&d,&a,&p)==WF_BLOCKED);
    puts("PASS test_runtime09");return 0;
}
