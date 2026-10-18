#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "wf_saturn_memory.h"
#include "wf_saturn_yaul_backend.h"
int main(void){
 assert(wf_saturn_memory_contract_validate()==WF_OK);
 uint32_t off=0;assert(wf_saturn_stream_tile_offset(0,&off)==WF_OK&&off==0x4B580u);assert(wf_saturn_stream_tile_offset(255,&off)==WF_OK&&off==0x53500u);assert(wf_saturn_stream_tile_offset(511,&off)==WF_OK&&off==0x5B500u);assert(wf_saturn_stream_tile_offset(512,&off)==WF_BLOCKED);
 uint16_t e=0;assert(wf_saturn_sprite_cram_entry(0,0,&e)==WF_OK&&e==0x100u);assert(wf_saturn_sprite_cram_entry(15,15,&e)==WF_OK&&e==0x1FFu);assert(wf_saturn_sprite_cram_entry(16,0,&e)==WF_BLOCKED);
 assert(wf_saturn_xbgr444_to_rgb888_raw(0x000u)==0x00000000u);assert(wf_saturn_xbgr444_to_rgb888_raw(0xF00u)==0x00FF0000u);assert(wf_saturn_xbgr444_to_rgb888_raw(0x0F0u)==0x0000FF00u);assert(wf_saturn_xbgr444_to_rgb888_raw(0x00Fu)==0x000000FFu);assert(wf_saturn_xbgr444_to_rgb888_raw(0xABCu)==0x00AABBCCu);
 assert(WF_VDP1_STREAM_PALETTES_MAX==16u);
 uint8_t tiles[128]={0}, pal[32]={0}; wf_vdp1_frame_plan_t p;memset(&p,0,sizeof(p));p.tile_count=1;p.palette_count=1;p.cmd_count=1;p.tile_upload_bytes=128;p.palette_upload_bytes=32;p.tile_upload[0]=(wf_vdp1_tile_upload_t){.source_tile=0,.stream_slot=0,.src_byte_offset=0,.dst_byte_offset=0};p.palette_upload[0]=(wf_vdp1_palette_upload_t){.source_resource=0,.cram_slot=0,.src_byte_offset=0};p.cmd[0]=(wf_vdp1_sprite_cmd_t){.x=10,.y=20,.stream_tile=0,.cram_slot=0};wf_vdp1_assets_t a={tiles,sizeof(tiles),pal,sizeof(pal)};wf_saturn_yaul_backend_t b;wf_saturn_yaul_backend_init(&b,0,0);assert(wf_saturn_yaul_present_plan(&b,&p,&a)==WF_OK);assert(!b.active&&b.emitted_tiles==1&&b.emitted_palettes==1&&b.emitted_cmds==1);
 puts("PASS test_runtime10");return 0;
}
