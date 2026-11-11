#include "wf_scene_checkpoint.h"
#include <string.h>
static uint32_t read_long(const uint8_t*p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static void write_long(uint8_t*p,uint32_t v){for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(v>>(24u-i*8u));}
wf_status_t wf_scene_checkpoint_capture(const wf_runtime_t*r,uint8_t*raw,size_t bytes,
 wf_r18_video_t*v,const wf_r18_world_ctx_t*w,const wf_r18_sprite_record_t*records,size_t count,const uint16_t queues[6]){
 if(!r||!r->m68k_regs.valid||!raw||bytes<0x4000u||!v||!w||(!records&&count)||count>512u||!queues)return WF_BAD_STATE;
 for(unsigned i=0;i<6;i++)if(queues[i])return WF_BLOCKED;
 if(w->animation_count[0]||w->animation_count[1])return WF_BLOCKED;
 /* These cache bytes are owned by the original world transport. Retain
  * them across the image swap, before runtime_export overlays its aliases. */
 for(unsigned side=0;side<2;side++)for(unsigned i=0;i<16;i++){
  uint8_t*p=raw+0x16d6u+side*128u+i*8u;
  write_long(p,w->cache[side][i].source);write_long(p+4,w->cache[side][i].destination);
 }
 raw[0x19b2]=raw[0x19b4]=(uint8_t)(count>>8);raw[0x19b3]=raw[0x19b5]=(uint8_t)count;
 if(count)memmove(v->fg0+0x2000,records,count*sizeof *records);
 return WF_OK;
}
wf_status_t wf_scene_checkpoint_restore(const wf_runtime_t*r,uint8_t*raw,size_t bytes,wf_r18_video_t*v){
 if(!r||!r->m68k_regs.valid||!raw||bytes<0x4000u||!v)return WF_BAD_STATE;
 unsigned count=(unsigned)raw[0x19b2]<<8|raw[0x19b3];if(count>512u)return WF_BAD_STATE;
 wf_r18_world_ctx_t world={0};const uint16_t queues[6]={0};
 for(unsigned side=0;side<2;side++)for(unsigned i=0;i<16;i++){
  const uint8_t*p=raw+0x16d6u+side*128u+i*8u;
  world.cache[side][i].source=read_long(p);world.cache[side][i].destination=read_long(p+4);
 }
 /* The producer queues were checked empty before capture. The original
  * implementation supplies palette/runtime/cache ordering without a clone. */
 return wf_r18_scene_resume(r,raw,bytes,raw,bytes,v,&world,
  (const wf_r18_sprite_record_t*)(const void*)(v->fg0+0x2000),count,queues);
}
