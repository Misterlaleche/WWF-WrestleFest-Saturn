#include <string.h>
#include "wf_r18_video.h"
#ifdef __APPLE__
#define OPTIONAL __attribute__((weak_import))
#else
#define OPTIONAL __attribute__((weak))
#endif
extern wf_status_t wf_r18_referee_direction_20c8(wf_runtime_t*,wf_actor_raw_t*,void*) OPTIONAL;
extern wf_status_t wf_r18_referee_number_2067c(wf_runtime_t*,uint16_t,void*) OPTIONAL;
extern wf_status_t wf_r18_referee_clear_206fe(wf_runtime_t*,void*) OPTIONAL;
extern wf_status_t wf_r18_referee_display_2067c(wf_runtime_t*,void*) OPTIONAL;
static wf_status_t read_rom(void *user,uint32_t at,void *dst,size_t n){if(at>0x80000u||n>0x80000u-at)return WF_BLOCKED;memcpy(dst,(const uint8_t*)user+at,n);return WF_OK;}
int referee_probe(unsigned op,const uint8_t *rom,uint8_t *actor,uint32_t *regs,uint8_t *fg,uint16_t id,uint8_t *scratch){
 wf_runtime_t rt={0};wf_r18_video_t video={0};wf_status_t status=WF_BLOCKED;
 memcpy(rt.g.direction_scratch_15e4,scratch,8);memcpy(rt.referee.b,actor,WF_ACTOR_BYTES);memcpy(rt.m68k_regs.d,regs,32);rt.m68k_regs.valid=1;
 memcpy(video.fg0,fg,sizeof video.fg0);video.read_rom=read_rom;video.read_user=(void*)rom;
 if(op==0&&wf_r18_referee_direction_20c8)status=wf_r18_referee_direction_20c8(&rt,&rt.referee,&video);
 if(op==1&&wf_r18_referee_number_2067c)status=wf_r18_referee_number_2067c(&rt,id,&video);
 if(op==2&&wf_r18_referee_clear_206fe)status=wf_r18_referee_clear_206fe(&rt,&video);
 if(op==3&&wf_r18_referee_display_2067c)status=wf_r18_referee_display_2067c(&rt,&video);
 memcpy(scratch,rt.g.direction_scratch_15e4,8);memcpy(actor,rt.referee.b,WF_ACTOR_BYTES);memcpy(regs,rt.m68k_regs.d,32);memcpy(fg,video.fg0,sizeof video.fg0);return status;
}
