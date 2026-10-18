#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wf_graphics.h"
static unsigned char *load(const char *p,size_t *n){FILE*f=fopen(p,"rb");assert(f);fseek(f,0,SEEK_END);long z=ftell(f);rewind(f);unsigned char*b=malloc((size_t)z);assert(b);assert(fread(b,1,(size_t)z,f)==(size_t)z);fclose(f);*n=(size_t)z;return b;}
int main(void){
 size_t ni,np;unsigned char*idx=load("assets/combat_pose_index_be.bin",&ni);unsigned char*pcs=load("assets/combat_pose_pieces_be.bin",&np);
 wf_graphics_blob_t b={idx,ni,pcs,np};wf_actor_raw_t a;memset(&a,0,sizeof(a));wf_set_u8(&a,WF_A_CHARACTER,0);wf_actor_visual_t v;memset(&v,0,sizeof(v));v.valid=1;v.pose=0;v.screen_x=100;v.screen_y=100;wf_draw_list_t d;memset(&d,0,sizeof(d));
 assert(wf_graphics_emit_actor(&a,&v,&b,&d)==WF_OK);assert(d.count==22);assert(d.p[0].tile==0x0000);assert(d.p[0].x==65);assert(d.p[0].y==90);assert(d.p[0].chain==1);
 assert(wf_graphics_set_for_internal_id(4)==10);assert(wf_graphics_set_for_internal_id(5)==11);
 free(idx);free(pcs);puts("PASS test_graphics_pack");return 0;
}
