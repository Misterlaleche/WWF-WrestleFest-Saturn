/* Measure actual memory work through the production Yaul boundary. A tiny
 * draw list must not clear storage for the unused512-command tail. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <yaul.h>
#include "wf_saturn_yaul_backend.h"
static size_t cleared;
static void *counted_clear(void*d,int v,size_t n);
#undef memset
#define memset counted_clear
#include "../../src/wf_saturn_yaul_backend.c"
#undef memset
static void *counted_clear(void*d,int v,size_t n){cleared+=n;return memset(d,v,n);}
unsigned mock_sync_calls,mock_sprite_priorities[8],mock_screen_priorities[3];
uint8_t mock_vram[0x80000];uint32_t mock_cram[1024];
static unsigned submitted;
void scu_dma_transfer(unsigned c,void*d,const void*s,size_t n){assert(c==0);memcpy(d,s,n);}
void scu_dma_transfer_wait(unsigned c){assert(c==0);}
void vdp1_sync_cmdt_list_put(vdp1_cmdt_list_t*l,unsigned at){
 assert(at==0&&l->count==4);assert(l->cmdts[0].kind==1&&l->cmdts[1].kind==2&&l->cmdts[2].kind==3&&l->cmdts[3].kind==4);
 assert(l->cmdts[2].mode.end_code_disable==1&&l->cmdts[2].mode.trans_pixel_disable==0);++submitted;
}
int main(void){
 wf_saturn_yaul_backend_t b;uint8_t stage[128];wf_saturn_yaul_backend_init(&b,stage,sizeof stage);
 memset(g_cmdts,0x55,sizeof g_cmdts);cleared=0;
 const wf_saturn_render_ops_t*o=wf_saturn_yaul_render_ops();wf_vdp1_sprite_cmd_t cmd={0};
 assert(o->begin_frame(&b,0,0,1)==WF_OK);assert(o->emit_sprite_16x16(&b,&cmd)==WF_OK);assert(o->end_frame(&b)==WF_OK);
 assert(submitted==1);
 assert(cleared<=sizeof b.dirty+4u*sizeof(vdp1_cmdt_t));
 printf("PASS command preparation: tiny list cleared%zu bytes, bounded by submitted prefix; exact clip/local/sprite/END modes\n",cleared);
}
