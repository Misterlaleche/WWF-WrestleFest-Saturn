#include "wf_r18_video.h"
#include "wf_r18_ring.h"
#include <string.h>

int background_owner_case(const void *pack,size_t bytes,const uint8_t *ids,
                          size_t count,uint8_t *image,uint16_t *names,
                          uint32_t *registers)
{
    static wf_runtime_t runtime;
    static wf_r18_video_t video;
    wf_r17_post7_ctx_t owner;
    memset(&runtime,0,sizeof runtime);
    memset(&video,0,sizeof video);
    memset(&owner,0,sizeof owner);
    memset(names,0,1024u*sizeof *names);
    if(wf_r18_ring_bind(pack,bytes))return WF_BAD_STATE;
    if(wf_r18_ring_cells_update(1,480,1488,names,NULL))return WF_BAD_STATE;
    wf_r18_video_ring_bind(&video);
    runtime.m68k_regs.valid=1;
    memcpy(runtime.m68k_regs.d,registers,32);
    if(count>sizeof runtime.g.transfer_queue_1808)return WF_BAD_STATE;
    memcpy(runtime.g.transfer_queue_1808,ids,count);
    runtime.g.transfer_count_1828=(uint16_t)count;
    owner.video_batch=wf_r18_video_batch;
    owner.user=&video;
    wf_status_t status=wf_r17_owner_298ec(&runtime,&owner);
    if(wf_r18_ring_cells_update(1,480,1488,names,NULL))return WF_BAD_STATE;
    memcpy(image,video.background+0x2000u,0x800u);
    memcpy(registers,runtime.m68k_regs.d,32);
    return status;
}
