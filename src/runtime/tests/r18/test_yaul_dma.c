#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <yaul.h>
#include "wf_saturn_yaul_backend.h"
unsigned mock_sync_calls;
unsigned mock_sprite_priorities[8],mock_screen_priorities[2];
uint8_t mock_vram[0x80000];uint32_t mock_cram[1024];
static unsigned transfers,waits,submitted;static uint8_t stage[32768] __aligned(32),pixels[32768];
void scu_dma_transfer(unsigned channel,void*dest,const void*src,size_t n){assert(channel==0);assert((const uint8_t*)src>=stage&&(const uint8_t*)src+n<=stage+sizeof stage);memcpy(dest,src,n);transfers++;}
void scu_dma_transfer_wait(unsigned c){assert(c==0);waits++;}
void vdp1_sync_cmdt_list_put(vdp1_cmdt_list_t*l,unsigned offset){assert(offset==0);for(unsigned i=0;i<l->count;i++)if(l->cmdts[i].kind==3){assert(l->cmdts[i].mode.end_code_disable==1);assert(l->cmdts[i].mode.trans_pixel_disable==0);}submitted++;}
int main(void){wf_saturn_yaul_backend_t b;const wf_saturn_render_ops_t*ops=wf_saturn_yaul_render_ops();
 for(unsigned i=0;i<sizeof pixels;i++)pixels[i]=(uint8_t)(i^(i>>7));
 wf_saturn_yaul_backend_init(&b,stage,sizeof stage);
 assert(ops->begin_frame(&b,256,0,1)==WF_OK);
 for(unsigned i=0;i<256;i++)assert(ops->upload_tile_16x16_4bpp(&b,i,pixels+i*128,128)==WF_OK);
 assert(transfers==0);wf_vdp1_sprite_cmd_t cmd={0};assert(ops->emit_sprite_16x16(&b,&cmd)==WF_OK);
 assert(ops->end_frame(&b)==WF_OK);assert(transfers==1&&waits==1&&submitted==1);
 assert(memcmp(mock_vram+WF_SATURN_VDP1_DYNAMIC_OFFSET,pixels,sizeof pixels)==0);
 assert(ops->begin_frame(&b,2,0,0)==WF_OK);assert(ops->upload_tile_16x16_4bpp(&b,1,pixels,128)==WF_BAD_STATE);ops->abort_frame(&b);assert(transfers==1);
 assert(ops->begin_frame(&b,2,0,0)==WF_OK);assert(ops->upload_tile_16x16_4bpp(&b,0,pixels,128)==WF_OK);assert(ops->end_frame(&b)==WF_BAD_STATE);assert(transfers==1);
 assert(ops->begin_frame(&b,0,0,0)==WF_OK);assert(ops->end_frame(&b)==WF_OK);assert(transfers==1);
 unsigned before=mock_sync_calls;b.defer_sync=1;
 assert(ops->begin_frame(&b,0,0,0)==WF_OK);assert(ops->end_frame(&b)==WF_OK);assert(mock_sync_calls==before);
 wf_saturn_yaul_backend_init(&b,stage,128);assert(ops->begin_frame(&b,2,0,0)==WF_BLOCKED);
 puts("PASS Yaul boundary: one DMA for 256 tiles, exact pixels, abort/incomplete isolation, color 15 preserved");return 0;}
