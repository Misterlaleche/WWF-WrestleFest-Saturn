#include "wf_r18_frontend_driver.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint8_t ram[0x4000],saved_ram[0x4000];
static wf_runtime_t runtime,saved_runtime;
static wf_r18_video_t video,saved_video;
static wf_status_t rr(void*u,uint32_t a,void*p,size_t n){(void)u;(void)a;memset(p,0,n);return WF_OK;}
static wf_status_t rb(void*u,uint32_t a,uint32_t*v,unsigned n){(void)u;(void)a;(void)n;*v=0;return WF_OK;}
static wf_status_t wb(void*u,uint32_t a,uint32_t v,unsigned n){(void)u;(void)a;(void)v;(void)n;return WF_OK;}
static wf_status_t level(void*u,wf_r18_frontend_driver_t*d,unsigned h){(void)u;(void)d;(void)h;return WF_OK;}
static wf_status_t event(void*u,wf_r18_frontend_driver_t*d){(void)u;(void)d;return WF_OK;}
static wf_status_t world(void*u,unsigned m){(void)u;(void)m;return WF_OK;}
static wf_status_t submit(void*u,const wf_r18_sprite_record_t*r,size_t n){(void)u;(void)r;(void)n;return WF_OK;}
int main(void){
 wf_r18_frontend_driver_t d,before;
 wf_r18_frontend_driver_config_t c={.ram=ram,.ram_bytes=sizeof ram,.runtime=&runtime,.video=&video,.ssp=0x1c4000,.usp=0x1c3000,.sr=0x2300,
  .platform={rr,rb,wb,level,event,event,world,submit,0,0,0}};
 assert(wf_r18_frontend_driver_init(&d,&c)==WF_OK);
 for(unsigned i=0;i<sizeof ram;i++)ram[i]=(uint8_t)(i*71u+19u);
 for(unsigned i=0;i<8;i++)d.regs.d[i]=0x12345678u+i*0x10001u;
 for(unsigned i=0;i<7;i++)d.a[i]=0xff1c0000u+i*0x134u;
 d.phase=WF_R18_DRIVER_AC0;d.generation=17;d.world_visible=1;d.credited_start=1;
 before=d;memcpy(saved_ram,ram,sizeof ram);saved_runtime=runtime;saved_video=video;
 assert(wf_r18_frontend_driver_resume_boot(&d,0x6fc)==WF_OK);
 assert(d.phase==WF_R18_DRIVER_BOOT&&d.pc==0x6fc&&d.scene.boot.pc==0x6fc&&d.generation==18&&!d.credited_start);
 assert(!memcmp(ram,saved_ram,sizeof ram)&&!memcmp(&d.regs,&before.regs,sizeof d.regs)&&!memcmp(d.a,before.a,sizeof d.a));
 assert(!memcmp(&d.control,&before.control,sizeof d.control)&&!memcmp(&d.platform,&before.platform,sizeof d.platform));
 assert(!memcmp(&runtime,&saved_runtime,sizeof runtime)&&!memcmp(&video,&saved_video,sizeof video)&&d.world_visible==1);
 for(unsigned which=0;which<7;which++){
  d=before;uint32_t entry=0x6fc;
  if(which==0)entry=0x654;
  if(which==1)d.phase=WF_R18_DRIVER_FATAL;
  if(which==2)d.irq_active=1;
  if(which==3)d.coin_active=1;
  if(which==4)d.call_depth=1;
  if(which==5)d.active_owner=0x65c0;
  if(which==6)d.regs.valid=0;
  wf_r18_frontend_driver_t unchanged=d;
  assert(wf_r18_frontend_driver_resume_boot(&d,entry)==WF_BAD_STATE&& !memcmp(&d,&unchanged,sizeof d));
 }
 assert(wf_r18_frontend_driver_resume_boot(NULL,0x6fc)==WF_BAD_STATE);
 puts("PASS BOOT6FC metadata-only resume, raw/D/A/control/runtime/video preserved;8 guards");return 0;
}
