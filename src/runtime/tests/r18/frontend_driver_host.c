/* Explicit host level/control tape. No SDK, synthesized driver IRQ or seed. */
#include "wf_r18_frontend_driver.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
typedef struct host {
 uint8_t rom[0x80000],ram[0x4000],board[0x40],display[0x20];
 wf_runtime_t runtime;wf_r18_video_t video;wf_r18_frontend_driver_t driver;
 unsigned level,frames,rasters,binds,submits,stores,commits,coin,start,royal,save_loads;
 unsigned clear_begins,clear_ends;
 unsigned title_seen,old_generation,manual_mode,manual_roster,busy_reads,roster_max_timer;
 uint32_t phase_mask,iohash;unsigned reject_store,reject_bind,reject_submit;
} host_t;
static host_t h;
static unsigned read_be(const uint8_t*p,unsigned n){unsigned v=0;while(n--)v=(v<<8)|*p++;return v;}
static void write_be(uint8_t*p,unsigned v,unsigned n){for(unsigned i=0;i<n;i++)p[i]=(uint8_t)(v>>(8*(n-1-i)));}
static wf_status_t rom(void*u,uint32_t at,void*p,size_t n){host_t*x=u;if(at>sizeof x->rom||n>sizeof x->rom-at)return WF_BLOCKED;memcpy(p,x->rom+at,n);return WF_OK;}
static wf_status_t read_bus(void*u,uint32_t at,uint32_t*v,unsigned n){
 host_t*x=u;if(at>=0x140020&&at<0x140028&&n==2){unsigned port=(at-0x140020)/2;*v=0xffff;
  if(port==0){if(x->coin)*v&=~0x100u;if(x->start)*v&=~0x80u;if(x->busy_reads){x->busy_reads--;*v&=~0x200u;}
   if(x->driver.phase==WF_R18_DRIVER_MODE&&x->manual_mode){
    if(x->royal&&!(x->ram[0x161]&1))*v&=~2u;
    else *v&=~0x10u;
   }
   if(x->driver.phase==WF_R18_DRIVER_ROSTER&&x->manual_roster){
    if((x->frames&3u)==1u)*v&=~0x10u;
    if(!x->royal&&(x->ram[0x5e3]&1u)&&!(x->ram[0x6ef]&1u)&&(x->frames&3u)==2u)*v&=~1u;
   }}
  if(port==3){*v=(*v&~0x400u)|(x->level?0x400u:0);}
  return WF_OK;}
 if(at>=0x100000&&at+n<=0x100020){*v=read_be(x->display+at-0x100000,n);return WF_OK;}
 if(at>=0x140000&&at+n<=0x140040){*v=read_be(x->board+at-0x140000,n);return WF_OK;}
 return WF_BLOCKED;
}
static wf_status_t store_bus(void*u,uint32_t at,uint32_t v,unsigned n){
 host_t*x=u;x->stores++;
 x->iohash=(x->iohash^at)*16777619u;x->iohash=(x->iohash^v)*16777619u;x->iohash=(x->iohash^n)*16777619u;
 if(x->reject_store)return WF_BLOCKED;
 if(at>=0x100000&&at+n<=0x100020){write_be(x->display+at-0x100000,v,n);return WF_OK;}
 if(at>=0x140000&&at+n<=0x140040){write_be(x->board+at-0x140000,v,n);return WF_OK;}
 if(at==0x184000&&n==2)return WF_OK;
 return WF_BLOCKED;
}
static wf_status_t bind(void*u,unsigned mode){host_t*x=u;if(mode>6||x->reject_bind)return WF_BLOCKED;x->binds++;return WF_OK;}
static wf_status_t submit(void*u,const wf_r18_sprite_record_t*r,size_t n){host_t*x=u;assert(r==(const void*)(x->video.fg0+0x2000));assert(n<=512);x->submits++;return x->reject_submit?WF_BLOCKED:WF_OK;}
static wf_status_t commit(void*u,wf_r18_frontend_driver_t*d){host_t*x=u;assert(d==&x->driver);x->commits++;return WF_OK;}
static wf_status_t reset_ready(void*u,uint8_t*ram,size_t bytes){host_t*x=u;assert(ram==x->ram&&bytes==sizeof x->ram);assert(ram[0xc2]==8u);x->save_loads++;return WF_OK;}
static wf_status_t scene_clear(void*u,uint32_t owner,unsigned complete){
 host_t*x=u;
 /* Clearing the old plane is not a reveal point. The copyright plate
  * becomes visible only after its replacement text has been written. */
 if(complete){assert(owner==0x2503cu);assert(x->driver.phase==WF_R18_DRIVER_BOOT);x->clear_ends++;}
 else{assert(owner==0x1fc0u||owner==0x1f6cu||owner==0x1fdeu);x->clear_begins++;}
 return WF_OK;
}
static wf_status_t vblank(void*u,wf_r18_frontend_driver_t*d){host_t*x=u;x->level=1;x->frames++;return wf_r18_frontend_driver_on_vblank(d);}
static wf_status_t level(void*u,wf_r18_frontend_driver_t*d,unsigned high){host_t*x=u;x->level=high;if(high)return vblank(u,d);return WF_OK;}
static void tick(unsigned coin,unsigned n){h.coin=coin;for(unsigned i=0;i<n;i++){h.rasters++;assert(wf_r18_frontend_driver_on_raster(&h.driver)==WF_OK);}}
static int step(void){unsigned phase=h.driver.phase;h.phase_mask|=1u<<phase;if(phase==WF_R18_DRIVER_ROSTER){unsigned timer=read_be(h.ram+0x15c,2);if(timer>h.roster_max_timer)h.roster_max_timer=timer;}int s=wf_r18_frontend_driver_step(&h.driver);if(s<0)fprintf(stderr,"blocked phase%u pc%06x dep%06x addr%06x\n",h.driver.phase,h.driver.pc,h.driver.blocked_dependency,h.driver.blocked_address);assert(s>=0);if(phase!=h.driver.phase)printf("TRACE phase%u->%u PC%06x frame%u flags=%02x/%02x credits%u mode%u io=%08x\n",phase,h.driver.phase,h.driver.pc,h.frames,h.ram[0x76],h.ram[0x6f],h.ram[0x4f],h.ram[0x161],h.iohash);return s;}
static void cold(unsigned royal){
 uint8_t preserved[0x60];memset(h.ram,0xa5,sizeof h.ram);memset(h.ram,0,0x60);write_be(h.ram+0x5c,0x12345678,4);memcpy(preserved,h.ram,0x60);
 memset(&h.video,0,sizeof h.video);memset(&h.runtime,0,sizeof h.runtime);
 h.level=h.frames=h.rasters=h.binds=h.submits=h.stores=h.commits=h.coin=h.start=h.phase_mask=h.busy_reads=h.roster_max_timer=h.save_loads=h.clear_begins=h.clear_ends=0;h.royal=royal;h.manual_mode=1;h.manual_roster=0;
 h.iohash=2166136261u;h.reject_store=h.reject_bind=h.reject_submit=0;
 wf_r18_frontend_driver_config_t c={.ram=h.ram,.ram_bytes=sizeof h.ram,.runtime=&h.runtime,.video=&h.video,.platform={rom,read_bus,store_bus,level,vblank,commit,bind,submit,&h,reset_ready,scene_clear},.ssp=0x1c3bd4,.usp=0x1c3fd4,.sr=0x2700};
 assert(wf_r18_frontend_driver_init(&h.driver,&c)==WF_OK);assert(!memcmp(h.ram,preserved,0x60));
}
static void start_case(unsigned royal,unsigned manual){
 cold(royal);h.manual_roster=manual;
 for(unsigned i=0;h.driver.phase!=WF_R18_DRIVER_TITLE;i++){assert(i<100000);step();}
 assert(read_be(h.ram+0x5c,4)==0x12345678);assert(h.frames>=384);assert(h.ram[0x4f]==0);
 /* First enter full animated title before accepting START. */
 for(unsigned i=0;h.driver.scene.title.pc<0x837e;i++){assert(i<100000);step();}
 tick(0,12);tick(1,3);tick(0,1);assert(h.ram[0x4f]==1);
 h.start=1;h.old_generation=h.driver.generation;
 for(unsigned i=0;h.driver.phase!=WF_R18_DRIVER_MODE_PLATE;i++){assert(i<100000);step();}
 assert(h.driver.credited_start&&h.driver.generation!=h.old_generation);assert(h.ram[0x4f]==0);assert(h.ram[0x7c]&128);assert(h.ram[0x5b0]&128);
 h.start=0;
 for(unsigned i=0;h.driver.phase!=WF_R18_DRIVER_AC0;i++){assert(i<200000);step();}
 assert(h.driver.pc==0xac0&&((h.ram[0x161]&1)!=0)==royal);assert(read_be(h.ram+0x598,2)!=0xffff);
 assert(read_be(h.ram+0x15c,2)==0&&read_be(h.ram+0x15e,2)==0);
 assert((h.phase_mask&0x3fu)==0x3fu);assert(h.submits&&h.commits==h.frames);assert(h.save_loads==1u);
 assert(h.clear_begins>0u&&h.clear_ends>0u);
 assert(manual?h.roster_max_timer<1024u:h.roster_max_timer>=1024u);
 printf("PASS host tape %s %s frames=%u rasterevents=%u stores=%u submits=%u binds=%u phaseMask=%02x history=%04x AC0\n",royal?"Royal":"TAG",manual?"manual":"timeout-auto",h.frames,h.rasters,h.stores,h.submits,h.binds,h.phase_mask,read_be(h.ram+0x598,2));
 fflush(stdout);
 /* Whole IRQ preserves declared interrupted D/A/SR/SSP/USP, including
  * repeated real834 busy reads before the external tape releases it. */
 wf_r17_m68k_shadow_t regs=h.driver.regs;uint32_t ar[7];memcpy(ar,h.driver.a,sizeof ar);
 uint16_t sr=h.driver.control.sr;uint32_t ssp=h.driver.control.ssp,usp=h.driver.control.usp;
 h.busy_reads=3;assert(wf_r18_frontend_driver_on_vblank(&h.driver)==WF_OK&&!h.busy_reads);
 assert(!memcmp(&regs,&h.driver.regs,sizeof regs)&&!memcmp(ar,h.driver.a,sizeof ar));
 assert(h.driver.control.sr==sr&&h.driver.control.ssp==ssp&&h.driver.control.usp==usp);
 tick(0,1);assert(!memcmp(&regs,&h.driver.regs,sizeof regs)&&!memcmp(ar,h.driver.a,sizeof ar));
 assert(h.driver.control.sr==sr&&h.driver.control.ssp==ssp&&h.driver.control.usp==usp);
}
int main(int argc,char**argv){assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f);assert(fread(h.rom,1,sizeof h.rom,f)==sizeof h.rom);fclose(f);
 start_case(0,0);start_case(1,0);start_case(0,1);start_case(1,1);
 cold(0);h.manual_roster=1;
 for(unsigned i=0;h.driver.phase!=WF_R18_DRIVER_BOOT||h.driver.scene.boot.pc!=0x1e7c;i++){assert(i<10000);step();}
 tick(0,12);tick(1,3);tick(0,1);assert(h.ram[0x4f]==1);h.start=1;h.old_generation=h.driver.generation;
 step();assert(h.driver.phase==WF_R18_DRIVER_MODE_PLATE&&h.driver.scene.plate.pc==0x52be&&h.driver.generation==h.old_generation+1);
 assert(!h.driver.world_visible&&h.driver.credited_start&&!h.ram[0x4f]);h.start=0;
 for(unsigned i=0;h.driver.phase!=WF_R18_DRIVER_AC0;i++){assert(i<100000);step();}
 assert(!(h.phase_mask&(1u<<WF_R18_DRIVER_TITLE)));
 puts("PASS credited START abandons original FBI hold cursor and reaches manualTAG AC0 without title replay");
 cold(0);assert(wf_r18_frontend_driver_run(&h.driver)==WF_OK);
 assert(h.driver.phase==WF_R18_DRIVER_AC0&&!h.driver.credited_start&&!h.ram[0x4f]&&!(h.ram[0x6f]&16u));
 puts("PASS uncredited title timeout reaches original attract AC0 boundary without synthetic loop");
 /* Reusable post-AC0 helpers expose visibility from original clear/map. */
 assert(wf_r18_frontend_driver_call(&h.driver,0x1f6c)==WF_OK&&!h.driver.world_visible);
 write_be(h.ram+0x1804,0x140,2);write_be(h.ram+0x1806,0x200,2);
 assert(wf_r18_frontend_driver_call(&h.driver,0x26e66)==WF_OK&&h.driver.world_visible);
 h.reject_submit=1;assert(wf_r18_frontend_driver_call(&h.driver,0x1fde)==WF_BLOCKED&&h.driver.blocked_dependency==0x1fde);
 cold(0);h.reject_store=1;assert(wf_r18_frontend_driver_call(&h.driver,0x2006)==WF_BLOCKED&&h.driver.blocked_dependency==0x2006&&h.driver.blocked_address==0x100008);
 cold(0);write_be(h.ram+0x7e,3,2);h.reject_bind=1;assert(wf_r18_frontend_driver_call(&h.driver,0x26e66)==WF_BLOCKED&&h.driver.blocked_dependency==0x26e66&&!h.driver.world_visible);
 cold(0);
 assert(wf_r18_frontend_driver_call(&h.driver,0x123456)==WF_BLOCKED&&h.driver.phase==WF_R18_DRIVER_FATAL&&h.driver.blocked_dependency==0x123456);
 puts("PASS store/bind/submit/unknown failures stop; shared sprite shadow; hardware waits delivered externally");return 0;}
