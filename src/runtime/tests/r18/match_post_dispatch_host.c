#define main frontend_only_main
#include "frontend_driver_host.c"
#undef main
#include "wf_r18_match_driver.h"
#include "wf_r18_match_support.h"
#include "wf_r18_frontend_world.h"
static wf_r18_match_driver_t match;
static wf_status_t scene_rom(void*u,uint32_t at,uint8_t*p,size_t n){return rom(u,at,p,n);}
static wf_status_t scene_palette(void*u,uint32_t at,uint16_t value){
 (void)u;wf_r17_fg0_write_t w={at,value,2};return wf_r18_video_batch(&h.runtime,&w,1,&h.video);
}
static wf_status_t paid_extension(void*u,uint32_t owner,wf_r18_prematch_t*m){
 (void)u;
 if(owner==0x285da){
  wf_r18_frontend_world_t w={.ram=m->ram,.ram_bytes=sizeof h.ram,.regs=m->regs,.a=m->a,.read_rom=rom,.user=&h};
  wf_status_t st=wf_r18_frontend_world_call(&w,owner);if(st!=WF_OK)m->dependency=w.blocked_dependency;return st;
 }
 wf_r18_match_support_t c={.machine=m,.fg_0c0000=h.video.fg0,.fg_bytes=sizeof h.video.fg0,.read_rom=scene_rom,.user=&h,.write_palette=scene_palette};
 return wf_r18_match_support_call(&c,owner);
}

int main(int argc,char**argv){
 assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f);
 assert(fread(h.rom,1,sizeof h.rom,f)==sizeof h.rom);fclose(f);
 start_case(0,1);
 assert(wf_r18_match_driver_init(&match,&h.driver,paid_extension,NULL)==WF_OK);
 h.driver.regs.d[0]=3u;
 uint32_t saved_d[8],saved_a[4];memcpy(saved_d,h.driver.regs.d,sizeof saved_d);memcpy(saved_a,match.machine.a,sizeof saved_a);
 int st=wf_r18_match_driver_owner(&match,0xae20u);
 printf("AE20 dispatch status=%d dependency=%06x pc=%06x world=%u\n",st,match.blocked_dependency,match.blocked_pc,read_be(h.ram+0x7e,2));fflush(stdout);
 assert(st==WF_OK);
 assert(read_be(h.ram+0x7e,2)==4u&&read_be(h.ram+0x1286,2)==0xffffu);
 assert(!memcmp(saved_d,h.driver.regs.d,sizeof saved_d)&&!memcmp(saved_a,match.machine.a,sizeof saved_a));
 puts("PASS shared match driver executes AE20 program3 and retains registers");
 return 0;
}
