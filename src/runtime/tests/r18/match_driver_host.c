#define main frontend_only_main
#include "frontend_driver_host.c"
#undef main
#include "wf_r18_match_driver.h"
#include "wf_r18_match_support.h"
#include "wf_r18_runtime_handoff.h"
#include "wf_r18_frontend_world.h"
static wf_r18_match_driver_t match;
static unsigned join_injected;
static wf_status_t paid_rom(void*u,uint32_t at,uint8_t*p,size_t n){return rom(u,at,p,n);}
static wf_status_t paid_palette(void*u,uint32_t at,uint16_t value){(void)u;wf_r17_fg0_write_t w={at,value,2};return wf_r18_video_batch(&h.runtime,&w,1,&h.video);}
static wf_status_t paid_extension(void*u,uint32_t owner,wf_r18_prematch_t*m){
 (void)u;
 if(owner==0x285da){
  wf_r18_frontend_world_t w={.ram=m->ram,.ram_bytes=sizeof h.ram,.regs=m->regs,.a=m->a,.read_rom=rom,.user=&h};
  wf_status_t st=wf_r18_frontend_world_call(&w,owner);if(st!=WF_OK)m->dependency=w.blocked_dependency;return st;
 }
 if(owner==0x98ba||owner==0x8eb6){
  wf_r18_match_support_t c={.machine=m,.fg_0c0000=h.video.fg0,.fg_bytes=sizeof h.video.fg0,.read_rom=paid_rom,.user=&h,.write_palette=paid_palette};
  return wf_r18_match_support_call(&c,owner);
 }
 m->dependency=owner;return WF_BLOCKED;
}
static wf_status_t joined_vblank(void*u,wf_r18_frontend_driver_t*d){
 if(!join_injected){tick(0,12);tick(1,3);tick(0,1);assert(h.ram[0x4f]==1);h.start=1;join_injected=1;}
 return vblank(u,d);
}
static wf_status_t joined_level(void*u,wf_r18_frontend_driver_t*d,unsigned high){
 h.level=high;return high?joined_vblank(u,d):WF_OK;
}
int main(int argc,char**argv){
 assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f);assert(fread(h.rom,1,sizeof h.rom,f)==sizeof h.rom);fclose(f);
 for(unsigned test=0;test<4;test++){
 unsigned royal=test/2u,manual=test&1u;
 start_case(royal,manual);assert(wf_r18_match_driver_init(&match,&h.driver,paid_extension,NULL)==WF_OK);
 int st=wf_r18_match_driver_run(&match);
 printf("paid state status=%d pc=%06x owner=%06x flow=%06x frames=%u mode=%u actors=%02x/%u,%02x/%u,%02x/%u,%02x/%u\n",st,match.blocked_pc,match.blocked_dependency,match.flow.pc,h.frames,h.ram[0x161],h.ram[0x5b0],h.ram[0x607],h.ram[0x6bc],h.ram[0x713],h.ram[0x7c8],h.ram[0x81f],h.ram[0x8d4],h.ram[0x92b]);
 fflush(stdout);assert(st==WF_OK);assert(match.flow.pc==0xf8c);assert(wf_r18_runtime_handoff_f8c(&h.runtime,h.ram,sizeof h.ram,&h.driver.regs)==WF_OK);
 assert(h.runtime.frame_stage==3&&h.runtime.g.frame_counter==0);
 assert((h.runtime.g.mode_0161&1u)==royal);puts("PASS paid full frontend/card/prematch/gameplay handoff");
 }
 cold(0);assert(wf_r18_frontend_driver_run(&h.driver)==WF_OK);
 assert(!h.driver.credited_start&&!read_be(h.ram+0x166,2));
 assert(wf_r18_match_driver_init(&match,&h.driver,paid_extension,NULL)==WF_OK);
 assert(wf_r18_match_driver_run(&match)==WF_OK&&match.flow.pc==0xf8c);
 assert(!h.ram[0x7c]&&(h.ram[0x606]&0x80u));
 puts("PASS natural uncredited title timeout selects original B1C0 demo setup and reaches F8C");
 cold(0);assert(wf_r18_frontend_driver_run(&h.driver)==WF_OK);
 h.driver.platform.wait_vblank=joined_vblank;h.driver.platform.wait_level=joined_level;join_injected=0;
 assert(wf_r18_match_driver_init(&match,&h.driver,paid_extension,NULL)==WF_OK);
 unsigned generation=h.driver.generation;
 assert(wf_r18_match_driver_run(&match)==WF_BLOCKED&&h.driver.generation!=generation);
 assert(!match.blocked_dependency&&h.driver.phase==WF_R18_DRIVER_MODE_PLATE&&h.driver.credited_start);
 h.start=0;h.manual_roster=1;h.driver.platform.wait_vblank=vblank;h.driver.platform.wait_level=level;
 assert(wf_r18_frontend_driver_run(&h.driver)==WF_OK);
 assert(wf_r18_match_driver_init(&match,&h.driver,paid_extension,NULL)==WF_OK&&wf_r18_match_driver_run(&match)==WF_OK);
 assert(h.ram[0x7c]&0x80u);puts("PASS credited START discards B1C call stack, resumes52BE, selects and reaches paid F8C");
 return 0;
}
