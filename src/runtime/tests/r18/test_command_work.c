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
static uint16_t expected[16];
void scu_dma_transfer(unsigned c,void*d,const void*s,size_t n){assert(c==0);memcpy(d,s,n);}
void scu_dma_transfer_wait(unsigned c){assert(c==0);}
void vdp1_sync_cmdt_list_put(vdp1_cmdt_list_t*l,unsigned at){
 assert(at==0&&l->count==4);
 assert(l->cmdts[0].cmd_ctrl==9&&l->cmdts[0].cmd_xc==319&&l->cmdts[0].cmd_yc==239);
 assert(l->cmdts[1].cmd_ctrl==10&&l->cmdts[3].cmd_ctrl==0x8000);
 assert(!memcmp(&l->cmdts[2],expected,sizeof expected));++submitted;
}
int main(void){
 wf_saturn_yaul_backend_t b;uint8_t stage[128];wf_saturn_yaul_backend_init(&b,stage,sizeof stage);
 const wf_saturn_render_ops_t*o=wf_saturn_yaul_render_ops();
 const int16_t xy[4][2]={{-15,-15},{0,0},{319,239},{-32768,32767}};
 for(unsigned i=0;i<256u;i++){
  wf_vdp1_sprite_cmd_t cmd={.x=xy[i&3u][0],.y=xy[i&3u][1],.stream_tile=(uint16_t)i,.cram_slot=(uint8_t)((i>>2)&15u),.flip_x=(uint8_t)(i&1u),.flip_y=(uint8_t)((i>>1)&1u)};
  /* Literal hardware-word contract, independently of the encoder/helper. */
  memset(expected,0,sizeof expected);expected[0]=(uint16_t)((cmd.flip_x?16u:0u)|(cmd.flip_y?32u:0u));
  expected[2]=0x80;expected[3]=(uint16_t)(0x100u+cmd.cram_slot*16u);
  expected[4]=(uint16_t)(0x96b0u+i*16u);expected[5]=0x210;
  expected[6]=(uint16_t)cmd.x;expected[7]=(uint16_t)cmd.y;
  memset(g_cmdts,0x55,sizeof g_cmdts);cleared=0;
  assert(o->begin_frame(&b,0,0,1)==WF_OK);assert(o->emit_sprite_16x16(&b,&cmd)==WF_OK);assert(o->end_frame(&b)==WF_OK);
  assert(cleared<=sizeof b.dirty+3u*sizeof(vdp1_cmdt_t));
  /* A stale command/END tail must never be read, cleared or submitted. */
  for(unsigned k=0;k<sizeof g_cmdts[4];k++)assert(((const uint8_t*)&g_cmdts[4])[k]==0x55);
 }
 assert(submitted==256u);
 printf("PASS256 command encodings: exact32 hardware bytes, palettes/flips/coordinates/slots; cleared%zu bytes per single-sprite frame\n",cleared);
}
