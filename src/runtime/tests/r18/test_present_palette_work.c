/* Exercise the real batch owner and Yaul backend. Count the physical
 * palette boundary, including a failed submission followed by a retry. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <yaul.h>
#include "wf_r18_present.h"
static const wf_saturn_render_ops_t *counted_ops(void);
#define wf_saturn_yaul_render_ops counted_ops
#include "../../src/wf_r18_present.c"
#undef wf_saturn_yaul_render_ops
uint8_t mock_vram[0x80000];uint32_t mock_cram[1024];
unsigned mock_sync_calls,mock_sprite_priorities[8],mock_screen_priorities[3];
static unsigned palette_calls,fail_end;
static wf_status_t palette(void *b,uint8_t bank,const uint8_t *p,size_t n){
 ++palette_calls;return wf_saturn_yaul_render_ops()->upload_palette_xbgr444_16(b,bank,p,n);
}
static wf_status_t end(void *b){
 if(fail_end){fail_end=0;wf_saturn_yaul_render_ops()->abort_frame(b);return WF_BLOCKED;}
 return wf_saturn_yaul_render_ops()->end_frame(b);
}
static const wf_saturn_render_ops_t *counted_ops(void){
 static wf_saturn_render_ops_t ops;ops=*wf_saturn_yaul_render_ops();
 ops.upload_palette_xbgr444_16=palette;ops.end_frame=end;return &ops;
}
void scu_dma_transfer(unsigned c,void*d,const void*s,size_t n){assert(c==0);memcpy(d,s,n);}
void scu_dma_transfer_wait(unsigned c){assert(c==0);}
void vdp1_sync_cmdt_list_put(vdp1_cmdt_list_t *l,unsigned at){assert(!at&&l->count==3);}
static int file(const char*n,uint32_t at,void*d,size_t bytes){(void)n;(void)at;(void)d;(void)bytes;return -1;}
int main(void){
 static wf_runtime_t r;static wf_r18_video_t v;uint8_t rom[1024],tile[128];
 wf_r18_present_init(file,rom,sizeof rom,tile,sizeof tile);wf_saturn_cram_reset();
 assert(wf_r18_present_batch(&r,NULL,0,&v)==WF_OK&&palette_calls==16);
 assert(wf_saturn_cram_writes==256);
 assert(wf_r18_present_batch(&r,NULL,0,&v)==WF_OK);
 if(palette_calls!=16){fprintf(stderr,"Unchanged sprite palettes were staged%u times (budget16 initial)\n",palette_calls);return 1;}
 /* A text color does not invalidate the independent sprite palette. */
 wf_r17_fg0_write_t text={0x180003u,15,1};
 assert(wf_r18_video_batch(&r,&text,1,&v)==WF_OK);
 assert(wf_r18_present_batch(&r,NULL,0,&v)==WF_OK&&palette_calls==16);
 wf_r17_fg0_write_t changed={0x182083u,15,1};
 assert(wf_r18_video_batch(&r,&changed,1,&v)==WF_OK);
 fail_end=1;assert(wf_r18_present_batch(&r,NULL,0,&v)==WF_BLOCKED);
 assert(palette_calls==17&&wf_saturn_cram_writes==256);
 assert(wf_r18_present_batch(&r,NULL,0,&v)==WF_OK);
 assert(palette_calls==18&&wf_saturn_cram_writes==257&&mock_cram[273]==255);
 assert(wf_r18_present_batch(&r,NULL,0,&v)==WF_OK&&palette_calls==18);
 puts("PASS real sprite palette submission: initial zero banks, unchanged/text skip, abort/retry and exact CRAM");return 0;
}
