#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "wf_saturn_render_bridge.h"
#include "wf_saturn_yaul_backend.h"
typedef struct C{unsigned b,t,p,c,e,a;int fail_tile;}C;
static wf_status_t b(void*x,unsigned t,unsigned p,unsigned c){C*q=x;q->b++;assert(t==1&&p==1&&c==2);return WF_OK;}
static wf_status_t t(void*x,uint16_t s,const uint8_t*d,size_t n){C*q=x;q->t++;assert(s==0&&d&&n==128);return q->fail_tile?WF_BLOCKED:WF_OK;}
static wf_status_t p(void*x,uint8_t s,const uint8_t*d,size_t n){C*q=x;q->p++;assert(s==0&&d&&n==32);return WF_OK;}
static wf_status_t c(void*x,const wf_vdp1_sprite_cmd_t*d){C*q=x;q->c++;assert(d->stream_tile==0&&d->cram_slot==0);return WF_OK;}
static wf_status_t e(void*x){C*q=x;q->e++;return WF_OK;}
static void a(void*x){C*q=x;q->a++;}
int main(void){
 uint8_t tiles[128]={0},pals[32]={0};wf_vdp1_assets_t as={tiles,sizeof tiles,pals,sizeof pals};wf_draw_list_t d;memset(&d,0,sizeof d);d.count=2;d.p[0].tile=0;d.p[1].tile=0;wf_vdp1_frame_plan_t plan;assert(wf_vdp1_plan_build(&d,&as,&plan)==WF_OK);
 wf_saturn_render_ops_t o={b,t,p,c,e,a};
 C x={0};assert(wf_saturn_render_submit(&plan,&as,&o,&x)==WF_OK);assert(x.b==1&&x.t==1&&x.p==1&&x.c==2&&x.e==1&&x.a==0);
 C y={0};y.fail_tile=1;assert(wf_saturn_render_submit(&plan,&as,&o,&y)==WF_BLOCKED);assert(y.b==1&&y.t==1&&y.p==0&&y.c==0&&y.e==0&&y.a==1);
 wf_vdp1_frame_plan_t retained;memset(&retained,0,sizeof retained);
 retained.tile_count=3;retained.cmd_count=3;
 retained.tile_upload[0].source_tile=30;retained.tile_upload[1].source_tile=10;retained.tile_upload[2].source_tile=40;
 for(unsigned i=0;i<3;i++)retained.cmd[i].stream_tile=(uint16_t)i;
 const uint16_t old_tiles[3]={10,20,30};const uint8_t valid[1]={7};
 assert(wf_vdp1_plan_retain_tile_slots(&retained,old_tiles,valid,3)==WF_OK);
 assert(retained.tile_upload[0].source_tile==10&&retained.tile_upload[1].source_tile==40&&retained.tile_upload[2].source_tile==30);
 assert(retained.cmd[0].stream_tile==2&&retained.cmd[1].stream_tile==0&&retained.cmd[2].stream_tile==1);
 assert(retained.tile_upload[0].stream_slot==0&&retained.tile_upload[1].stream_slot==1&&retained.tile_upload[2].stream_slot==2);
 retained.tile_count=2;retained.cmd_count=2;retained.tile_upload[0].source_tile=30;retained.tile_upload[1].source_tile=10;
 retained.cmd[0].stream_tile=0;retained.cmd[1].stream_tile=1;
 assert(wf_vdp1_plan_retain_tile_slots(&retained,old_tiles,valid,3)==WF_OK);
 assert(retained.tile_upload[0].source_tile==10&&retained.tile_upload[1].source_tile==30);
 assert(retained.cmd[0].stream_tile==1&&retained.cmd[1].stream_tile==0);
 const uint16_t duplicated_resident[3]={10,10,30};
 retained.tile_count=3;retained.cmd_count=3;
 retained.tile_upload[0].source_tile=30;retained.tile_upload[1].source_tile=10;retained.tile_upload[2].source_tile=40;
 for(unsigned i=0;i<3;i++)retained.cmd[i].stream_tile=(uint16_t)i;
 assert(wf_vdp1_plan_retain_tile_slots(&retained,duplicated_resident,valid,3)==WF_OK);
 assert(retained.tile_upload[0].source_tile==10&&retained.tile_upload[1].source_tile==40&&retained.tile_upload[2].source_tile==30);
 for(unsigned i=0;i<3;i++)assert(retained.cmd[i].stream_tile<retained.tile_count);
 wf_saturn_yaul_backend_t backend;wf_saturn_yaul_backend_init(&backend,NULL,0);
 const wf_saturn_render_ops_t *ops=wf_saturn_yaul_render_ops();
 assert(ops->begin_frame(&backend,1,0,1)==WF_OK);
 wf_vdp1_sprite_cmd_t outside={0};outside.stream_tile=1;
 assert(ops->emit_sprite_16x16(&backend,&outside)==WF_BAD_STATE);
 ops->abort_frame(&backend);
 puts("PASS test_saturn_render_bridge");return 0;
}
