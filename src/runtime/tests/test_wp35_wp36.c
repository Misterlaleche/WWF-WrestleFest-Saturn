#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wf_graphics.h"
#include "wf_renderer_contract.h"
#include "wf_vdp1_plan.h"
#include "wf_visual.h"
#include "wf_combat_frame.h"

static unsigned char *load(const char *p,size_t *n){
 FILE*f=fopen(p,"rb");long z;unsigned char*b;assert(f);assert(fseek(f,0,SEEK_END)==0);z=ftell(f);assert(z>=0);rewind(f);b=(unsigned char*)malloc((size_t)z);assert(b);assert(fread(b,1,(size_t)z,f)==(size_t)z);fclose(f);*n=(size_t)z;return b;
}
static void actor_for_set(wf_actor_raw_t *a,uint8_t internal_id){memset(a,0,sizeof(*a));wf_set_u8(a,WF_A_CHARACTER,internal_id);}
static void view_at(wf_actor_visual_t *v,uint16_t pose,int16_t sx,int16_t sy){memset(v,0,sizeof(*v));v->valid=1u;v->pose=pose;v->screen_x=sx;v->screen_y=sy;}

static void test_wp35_world_to_intermediate_chain(void){
 wf_runtime_t rt;wf_visual_state_t v;memset(&rt,0,sizeof(rt));wf_visual_init(&v);
 wf_actor_raw_t *a=&rt.actors[0];wf_set_u8(a,WF_A_ACTIVE,1u);wf_set_be16(a,WF_A_X,0x0200u);wf_set_be16(a,WF_A_Y,0x0200u);wf_set_be16(a,WF_A_Z,0u);
 v.actor[0].valid=1u;v.camera.scroll0_x=0x0140;v.camera.scroll0_y=0x0200;
 assert(wf_visual_build_screen_positions(&rt,&v)==WF_OK);
 assert(wf_be16(a,WF_A_SCREEN_X)==0x00c0u);assert(wf_be16(a,WF_A_SCREEN_Y)==0u);
 assert(v.actor[0].screen_x==0x00c0);assert(v.actor[0].screen_y==0);
 puts("PASS WP35 world -> $247C intermediate anchor path");
}

static void test_wp35_intermediate_to_raster(void){
 uint8_t one[10]={0,1,1,0,0,0,0,0,0,0};wf_draw_list_t d;memset(&d,0,sizeof(d));
 assert(wf_graphics_emit_composition(one,sizeof(one),0u,1u,0x0030,0x0030,0u,0u,&d)==WF_OK);
 assert(d.count==1u);assert(d.p[0].x==0x0030);assert(d.p[0].y==192);
 /* $247C +$14/+16 must never be passed directly as final VDP1 Y. */
 assert(d.p[0].y!=0x0030);
 puts("PASS WP35 intermediate $247C anchor -> WP26 raster decode");
}

static void test_facing_preserves_anchor(void){
 uint8_t one[10]={0,1,1,0,0,10,0,0,0,0};wf_draw_list_t l,r;memset(&l,0,sizeof(l));memset(&r,0,sizeof(r));
 assert(wf_graphics_emit_composition(one,sizeof(one),0u,1u,100,0x0030,0u,0u,&l)==WF_OK);
 assert(wf_graphics_emit_composition(one,sizeof(one),0u,1u,100,0x0030,0u,1u,&r)==WF_OK);
 assert(l.p[0].x==110&&r.p[0].x==90);assert(l.p[0].flip_x==0u&&r.p[0].flip_x==1u);
 puts("PASS WP35 facing mirrors local offset without moving logical anchor");
}

static void check_warrior(const wf_graphics_blob_t*g,uint16_t pose){
 wf_actor_raw_t a;wf_actor_visual_t v;wf_draw_list_t d;actor_for_set(&a,1u);view_at(&v,pose,100,0x0030);memset(&d,0,sizeof(d));
 assert(wf_graphics_emit_actor(&a,&v,g,&d)==WF_OK);assert(d.count==23u);
 const int ax=wf_sprite_x_decode(100u),ay=wf_sprite_y_decode(0x0030u);int minx=999,miny=999,maxx=-999,maxy=-999;
 for(unsigned i=0;i<d.count;i++){int rx=d.p[i].x-ax,ry=d.p[i].y-ay;assert(d.p[i].palette==1u);if(rx<minx)minx=rx;if(ry<miny)miny=ry;if(rx+16>maxx)maxx=rx+16;if(ry+16>maxy)maxy=ry+16;}
 assert(minx==-34&&miny==-98&&maxx-minx==64&&maxy-miny==112);
}

static void test_wp36_pinned_regressions(const wf_graphics_blob_t*g){
 wf_actor_raw_t a;wf_actor_visual_t v;wf_draw_list_t d;
 check_warrior(g,0x0062u);check_warrior(g,0x0063u);
 actor_for_set(&a,4u);view_at(&v,0x005bu,100,0x0030);memset(&d,0,sizeof(d));assert(wf_graphics_emit_actor(&a,&v,g,&d)==WF_OK);assert(d.count==20u);
 actor_for_set(&a,10u);view_at(&v,0x0068u,100,0x0030);memset(&d,0,sizeof(d));assert(wf_graphics_emit_actor(&a,&v,g,&d)==WF_OK);assert(d.count==23u);
 actor_for_set(&a,3u);view_at(&v,0x0178u,100,0x0030);memset(&d,0,sizeof(d));assert(wf_graphics_emit_actor(&a,&v,g,&d)==WF_BLOCKED);assert(d.count==0u);
 puts("PASS WP36 Warrior/Demolition/Hawk pins; partner-required Bossman fails closed");
}

static void test_active_combat_render_path(const wf_graphics_blob_t*g,const wf_vdp1_assets_t*a){
 wf_runtime_t rt;wf_visual_state_t v;wf_combat_frame_output_t o;memset(&rt,0,sizeof(rt));wf_visual_init(&v);
 wf_actor_raw_t *ar=&rt.actors[0];wf_set_u8(ar,WF_A_ACTIVE,1u);wf_set_u8(ar,WF_A_CHARACTER,0u);
 wf_set_be16(ar,WF_A_X,0x01a4u);wf_set_be16(ar,WF_A_Y,0x0264u);wf_set_be16(ar,WF_A_Z,0u);
 v.actor[0].valid=1u;v.actor[0].pose=0u;v.camera.scroll0_x=0x0140;v.camera.scroll0_y=0x0200;
 assert(wf_combat_build_render_frame(&rt,&v,g,a,&o)==WF_OK);
 assert(wf_be16(ar,WF_A_SCREEN_X)==100u&&wf_be16(ar,WF_A_SCREEN_Y)==100u);
 assert(o.draw.count==22u);assert(o.draw.p[0].x==65&&o.draw.p[0].y==90);assert(o.vdp1.cmd_count==22u);
 puts("PASS active combat render consumes WP35 intermediate + WP26 raster decode");
}

static void test_bossman_86_capacity(const wf_vdp1_assets_t*a){
 size_t n=0;unsigned char*pcs=load("authority/wp35-wp36/bossman_g03_pose0178_partner01_86_be.dat",&n);wf_draw_list_t d;wf_vdp1_frame_plan_t p;memset(&d,0,sizeof(d));
 assert(n==860u);assert(wf_graphics_emit_composition(pcs,n,0u,86u,100,0x0030,0u,0u,&d)==WF_OK);assert(d.count==86u);assert(d.p[0].tile==0x350bu&&d.p[85].tile==0xe2fdu);
 assert(wf_vdp1_plan_build(&d,a,&p)==WF_OK);assert(p.cmd_count==86u);assert(p.tile_count==66u);assert(p.palette_count==2u);
 free(pcs);puts("PASS WP36 86-cell composition survives WP26 command planning without truncation");
}

int main(void){
 size_t ni,np,nt,nc;unsigned char*idx=load("assets/combat_pose_index_be.bin",&ni);unsigned char*pcs=load("assets/combat_pose_pieces_be.bin",&np);unsigned char*tiles=load("assets/sprites_16x16_4bpp.pak",&nt);unsigned char*pals=load("assets/sprite_palette_resources_xbgr444_be.bin",&nc);
 wf_graphics_blob_t g={idx,ni,pcs,np};wf_vdp1_assets_t a={tiles,nt,pals,nc};
 test_wp35_world_to_intermediate_chain();test_wp35_intermediate_to_raster();test_facing_preserves_anchor();test_wp36_pinned_regressions(&g);test_active_combat_render_path(&g,&a);test_bossman_86_capacity(&a);
 free(idx);free(pcs);free(tiles);free(pals);puts("PASS test_wp35_wp36");return 0;
}
