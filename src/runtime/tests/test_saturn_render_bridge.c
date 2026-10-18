#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "wf_saturn_render_bridge.h"
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
 puts("PASS test_saturn_render_bridge");return 0;
}
