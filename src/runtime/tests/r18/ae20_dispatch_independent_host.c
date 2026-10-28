/* Independent transport/failure contracts around the real AE20 cursor.
 * The included fixture supplies legitimate manual frontend input; subsequent
 * calls deliberately exercise the public owner API, not a campaign result. */
#include "match_post_dispatch_host.c"
#include "wf_r18_post_tag.h"
_Static_assert(sizeof(wf_r18_post_tag_t)==100u,"post cursor ABI includes callback A0");
static host_t baseline;
static unsigned fault,hit,syncs,clear_observations;
static uint32_t failed_pc;
static wf_status_t checked_rom(void*u,uint32_t at,void*out,size_t n){
 if(fault==2u&&at==0xb0c5u){hit++;failed_pc=h.driver.pc;return WF_BLOCKED;}
 return rom(u,at,out,n);
}
static wf_status_t checked_store(void*u,uint32_t at,uint32_t value,unsigned n){
 if(fault==1u&&at==0x140010u&&h.driver.pc==0xae64u){hit++;failed_pc=h.driver.pc;return WF_BLOCKED;}
 return store_bus(u,at,value,n);
}
static wf_status_t checked_submit(void*u,const wf_r18_sprite_record_t*r,size_t n){
 if(fault==5u){hit++;failed_pc=h.driver.pc;return WF_BLOCKED;}
 return submit(u,r,n);
}
static wf_status_t checked_vblank(void*u,wf_r18_frontend_driver_t*d){
 if(d->pc==0xaf40u||d->pc==0xb068u){
  syncs++;
  if(d->pc==0xb068u&&read_be(h.ram+0x12a6,2)==0x1bu){
   for(unsigned row=0;row<5;row++)for(unsigned i=0;i<144;i++)assert(!h.video.fg0[0x1908+row*0x100+i]);
   clear_observations++;
  }
  if(fault==3u){hit++;failed_pc=d->pc;return WF_BLOCKED;}
  if(fault==4u){
   hit++;d->generation++;d->phase=WF_R18_DRIVER_MODE_PLATE;d->pc=0x52beu;
   for(unsigned i=0;i<8;i++)d->regs.d[i]=0xface1000u+i;
   for(unsigned i=0;i<7;i++)d->a[i]=0xface2000u+i;
   return WF_OK;
  }
  /* Explicit transport-contract fixture: successful callbacks without an ACK
   * must reissue the original poll, never consume its return PC. */
  if(fault==6u&&hit<3u){assert(d->pc==0xaf40u&&!(h.ram[0x6f]&2u));hit++;return WF_OK;}
 }
 return vblank(u,d);
}
static void prepare(unsigned which){
 h=baseline;fault=which;hit=syncs=clear_observations=failed_pc=0;
 h.driver.platform.read_rom=checked_rom;h.driver.platform.store_bus=checked_store;
 h.driver.platform.wait_vblank=checked_vblank;h.driver.platform.submit_records=checked_submit;
 assert(wf_r18_match_driver_init(&match,&h.driver,paid_extension,NULL)==WF_OK);
 for(unsigned i=0;i<8;i++)h.driver.regs.d[i]=0xbacd0000u+i;
 for(unsigned i=0;i<7;i++)match.machine.a[i]=0xcafe0000u+i;
 h.driver.regs.d[0]&=0xffff0000u;
}
int main(int argc,char**argv){
 assert(argc==2);setvbuf(stdout,NULL,_IOLBF,0);FILE*f=fopen(argv[1],"rb");assert(f);
 assert(fread(h.rom,1,sizeof h.rom,f)==sizeof h.rom);fclose(f);
 start_case(0,1);baseline=h;
 for(unsigned which=0;which<7;which++){
  prepare(which);uint32_t saved_d[8],saved_a[4];memcpy(saved_d,h.driver.regs.d,sizeof saved_d);memcpy(saved_a,match.machine.a,sizeof saved_a);
  wf_status_t st=wf_r18_match_driver_owner(&match,0xae20u);
  if(which==0u||which==6u){
   assert(st==WF_OK&&!match.blocked_dependency&&syncs&&clear_observations);
   assert(!memcmp(saved_d,h.driver.regs.d,sizeof saved_d)&&!memcmp(saved_a,match.machine.a,sizeof saved_a));
   assert(!read_be(h.ram+0x1278,2));
   for(unsigned i=0;i<3;i++){assert(!read_be(h.ram+0x1258+i*0x2a,2));assert(!read_be(h.ram+0x1274+i*0x2a,2));}
   if(which==6u)assert(hit==3u);
  }else{
   assert(st==WF_BLOCKED&&hit==1u);
   if(which==4u){
    assert(!match.blocked_dependency&&!match.blocked_pc&&h.driver.phase==WF_R18_DRIVER_MODE_PLATE&&h.driver.pc==0x52beu);
    for(unsigned i=0;i<8;i++)assert(h.driver.regs.d[i]==0xface1000u+i);
    for(unsigned i=0;i<7;i++)assert(h.driver.a[i]==0xface2000u+i&&match.machine.a[i]==h.driver.a[i]);
   }else{
    assert(match.blocked_dependency&&match.blocked_pc);
    if(which==1u)assert(match.blocked_dependency==0xae64u&&failed_pc==0xae64u);
    if(which==2u)assert(match.blocked_dependency==0xae8eu&&failed_pc==0xae8eu);
    if(which==3u)assert(match.blocked_dependency==0xaf40u&&failed_pc==0xaf40u);
   }
   /* Poisoned and abandoned owners must not retry the original prefix. */
   unsigned saved_hit=hit,saved_frames=h.frames,saved_submits=h.submits,saved_stores=h.stores;
   uint8_t saved_ram[sizeof h.ram];memcpy(saved_ram,h.ram,sizeof saved_ram);
   assert(wf_r18_match_driver_owner(&match,0xae20u)==WF_BLOCKED);
   assert(hit==saved_hit&&h.frames==saved_frames&&h.submits==saved_submits&&h.stores==saved_stores&&!memcmp(saved_ram,h.ram,sizeof saved_ram));
  }
  printf("PASS AE20 independent contract=%u status=%u dependency=%06x blocked_pc=%06x failed_pc=%06x syncs=%u B2E4-observations=%u\n",which,st,match.blocked_dependency,match.blocked_pc,failed_pc,syncs,clear_observations);
 }
 const unsigned skipped[]={0u,1u,2u,0xffffu};
 for(unsigned i=0;i<sizeof skipped/sizeof skipped[0];i++){
  prepare(0);write_be(h.ram+0x8c,0x8000u,2);write_be(h.ram+0x1278,0x5a5au,2);
  h.driver.regs.d[0]=0xfedc0000u|skipped[i];uint32_t rd[8],ra[7];uint8_t raw[sizeof h.ram];
  memcpy(rd,h.driver.regs.d,sizeof rd);memcpy(ra,match.machine.a,sizeof ra);memcpy(raw,h.ram,sizeof raw);
  unsigned frames=h.frames,submits=h.submits,stores=h.stores;
  assert(wf_r18_match_driver_owner(&match,0xae20u)==WF_OK&&!match.blocked_dependency);
  assert(!memcmp(rd,h.driver.regs.d,sizeof rd)&&!memcmp(ra,match.machine.a,sizeof ra));
  assert(!memcmp(raw,h.ram,sizeof raw)&&!memcmp(&h.video,&baseline.video,sizeof h.video));
  assert(h.frames==frames&&h.submits==submits&&h.stores==stores&&!syncs&&!hit);
  printf("PASS AE20 gate008C shared dispatch returns arg=%04x before any call/write/MOVEM\n",skipped[i]);
 }
 /* B2E4's shared dispatch preserves all registers and every byte outside the
  * five original spans, including the padded rows between them. */
 prepare(0);memset(h.video.fg0,0xa5,sizeof h.video.fg0);memset(h.video.hud_dirty,0,sizeof h.video.hud_dirty);
 uint32_t saved_d[8],saved_a[7];memcpy(saved_d,h.driver.regs.d,sizeof saved_d);memcpy(saved_a,match.machine.a,sizeof saved_a);
 assert(wf_r18_match_driver_owner(&match,0xb2e4u)==WF_OK);
 for(unsigned at=0;at<sizeof h.video.fg0;at++){
  unsigned inside=at>=0x1908u&&at<0x1d98u&&(at-0x1908u)%0x100u<144u;
  assert(h.video.fg0[at]==(inside?0u:0xa5u));
 }
 for(unsigned i=0;i<sizeof h.video.hud_dirty;i++)assert(h.video.hud_dirty[i]==0xffu);
 assert(!memcmp(saved_d,h.driver.regs.d,sizeof saved_d)&&!memcmp(saved_a,match.machine.a,sizeof saved_a));
 puts("PASS B2E4 shared dispatch exact five spans/padding/full D/A/dirty transport; no scene or timing claim");
 return 0;
}
