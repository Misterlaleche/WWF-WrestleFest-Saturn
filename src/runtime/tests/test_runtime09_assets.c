#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wf_graphics.h"
#include "wf_vdp1_plan.h"
static unsigned char *load(const char *p,size_t *n){FILE*f=fopen(p,"rb");assert(f);fseek(f,0,SEEK_END);long z=ftell(f);assert(z>=0);rewind(f);unsigned char*b=malloc((size_t)z);assert(b);assert(fread(b,1,(size_t)z,f)==(size_t)z);fclose(f);*n=(size_t)z;return b;}
int main(void){
 size_t ni,np,nt,nc;unsigned char*idx=load("assets/combat_pose_index_be.bin",&ni);unsigned char*pcs=load("assets/combat_pose_pieces_be.bin",&np);unsigned char*tiles=load("assets/sprites_16x16_4bpp.pak",&nt);unsigned char*pals=load("assets/sprite_palette_resources_xbgr444_be.bin",&nc);
 assert(nt==8388608u);assert(nc==8192u);
 wf_graphics_blob_t g={idx,ni,pcs,np};wf_vdp1_assets_t a={tiles,nt,pals,nc};
 wf_actor_raw_t actor;memset(&actor,0,sizeof actor);wf_set_u8(&actor,WF_A_CHARACTER,0);
 wf_actor_visual_t v;memset(&v,0,sizeof v);v.valid=1;v.pose=0;v.screen_x=160;v.screen_y=120;
 wf_draw_list_t d;memset(&d,0,sizeof d);assert(wf_graphics_emit_actor(&actor,&v,&g,&d)==WF_OK);assert(d.count==22u);
 wf_vdp1_frame_plan_t p;assert(wf_vdp1_plan_build(&d,&a,&p)==WF_OK);assert(p.cmd_count==22u);assert(p.tile_count>0u&&p.tile_count<=22u);assert(p.palette_count>0u);assert(p.tile_upload_bytes<=22u*128u);assert(p.palette_upload_bytes<=22u*32u);
 free(idx);free(pcs);free(tiles);free(pals);puts("PASS test_runtime09_assets");return 0;
}
