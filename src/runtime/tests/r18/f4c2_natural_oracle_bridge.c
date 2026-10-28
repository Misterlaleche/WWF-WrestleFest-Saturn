#include <string.h>
#include "wf_gameplay_bindings.h"
#include "wf_r18_runtime_handoff.h"

static wf_status_t video_batch(wf_runtime_t *rt,const wf_r17_fg0_write_t *writes,
                               size_t count,void *user){
 uint8_t *mem=user;(void)rt;
 for(size_t i=0;i<count;i++){
  if(!((writes[i].addr>=0xc0000u&&writes[i].addr+writes[i].width<=0xc4000u)||
       (writes[i].addr>=0x180000u&&writes[i].addr+writes[i].width<=0x184000u)))return WF_BLOCKED;
  for(unsigned j=0;j<writes[i].width;j++)
   mem[writes[i].addr+j]=(uint8_t)(writes[i].value>>(8u*(writes[i].width-1u-j)));
 }
 return WF_OK;
}
static wf_status_t audio(wf_runtime_t *rt,uint16_t command,void *user){
 (void)rt;(void)command;(void)user;return WF_OK;
}
static unsigned read16(const uint8_t*p){return ((unsigned)p[0]<<8)|p[1];}
static unsigned read32(const uint8_t*p){return (read16(p)<<16)|read16(p+2);}
static void write16(uint8_t*p,unsigned v){p[0]=(uint8_t)(v>>8);p[1]=(uint8_t)v;}
static void write32(uint8_t*p,unsigned v){write16(p,v>>16);write16(p+2,v);}
static wf_actor_raw_t *queue_actor(wf_runtime_t *r,unsigned pointer){
 if(pointer==0x1c1134u)return &r->fixed_four_1134;
 for(unsigned i=0;i<WF_ACTOR_SLOTS;i++)
  if(pointer==0x1c05b0u+i*WF_ACTOR_BYTES)return &r->actors[i];
 return 0;
}
static unsigned queue_pointer(wf_runtime_t *r,wf_actor_raw_t *a){
 if(a==&r->fixed_four_1134)return 0x1c1134u;
 for(unsigned i=0;i<WF_ACTOR_SLOTS;i++)
  if(a==&r->actors[i])return 0x1c05b0u+i*WF_ACTOR_BYTES;
 return 0;
}
int f4c2_natural_oracle(uint8_t *mem,uint32_t *d,uint32_t *blocked){
 wf_runtime_t r={0};wf_gameplay_binding_context_t c={0};
 wf_r17_m68k_shadow_t regs={0};
 memcpy(regs.d,d,sizeof regs.d);regs.valid=1;
 if(wf_r18_runtime_import(&r,mem+0x1c0000u,0x4000u,&regs)!=WF_OK)return -1;
 for(unsigned k=0;k<WF_R17_RENDER_LISTS;k++){
  unsigned count=read16(mem+0x1c19b8u+4u*k);
  if(count>WF_R17_RENDER_LIST_CAP)return -3;
  c.render_queue.count[k]=(uint16_t)count;
  for(unsigned i=0;i<count;i++){
   unsigned pointer=read32(mem+0x1c19d0u+0x80u*k+4u*i);
   c.render_queue.list[k][i]=queue_actor(&r,pointer);
   if(!c.render_queue.list[k][i])return -4;
  }
 }
 c.camera_valid=1;c.camera.scroll0_x=r.g.global_17e6;
 c.camera.scroll0_y=r.g.global_17ee;
 c.post7_video_batch=video_batch;c.r17_owner_user=mem;
 c.audio_command=audio;
 wf_status_t st=wf_phase_wrestlers_f4c2(&r,0x00f4c2u,&c);
 if(blocked)*blocked=c.last_blocked_dependency;
 if(wf_r18_runtime_export(&r,mem+0x1c0000u,0x4000u)!=WF_OK)return -2;
 for(unsigned k=0;k<WF_R17_RENDER_LISTS;k++){
  write16(mem+0x1c19b8u+4u*k,c.render_queue.count[k]);
  for(unsigned i=0;i<c.render_queue.count[k];i++){
   unsigned pointer=queue_pointer(&r,c.render_queue.list[k][i]);
   if(!pointer)return -5;
   write32(mem+0x1c19d0u+0x80u*k+4u*i,pointer);
  }
 }
 memcpy(d,r.m68k_regs.d,sizeof r.m68k_regs.d);
 return (int)st;
}
