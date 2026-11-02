/* Catches scalar fallback caused solely by an almost consumed service grant.
 * A synthetic NOP/JP prefix reaches unchanged original-ROM lift instructions.
 * The pinned scalar CPU independently verifies every resulting state. */
#include "wf_music_seq.h"
#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
extern bool wf_ref_init(wf_music_seq_t *,const uint8_t *,size_t,wf_music_emit_fn,void *,bool);
extern int wf_ref_run(wf_music_seq_t *,uint64_t,unsigned);
extern void wf_ref_z80_step(z80 *);
extern uint64_t wf_audit_since_service,wf_audit_retired,wf_audit_services,wf_audit_max_service;
extern unsigned wf_audit_max_grant;
extern int wf_audit_service_enabled;
extern void wf_service_audit(void),wf_service_audit_report(void);
#ifdef WF_EARLY_QUOTA_TEST
extern unsigned wf_z80_lift_quota(z80 *);
#endif
static uint8_t original_rom[65536],rom[65536];
static wf_music_seq_t candidate,reference;
static unsigned entry_pc,entry_services,fail_service,prefix_instructions;
static uint64_t prefix_cycles;
static unsigned original_body_steps;
static const unsigned pcs[]={0xb68,0xbe7,0xbd7,0xf3a,0xf2d,0xc09,0xc16,0xc21,0xc2c,0xf5f,0xf5f};
static const unsigned quotas[]={27,21,78,61,68,18,18,18,18,5,2};
static uint64_t exact_cases;
void wf_music_unknown_opcode(void){abort();}

static bool service(void *unused)
{
 (void)unused;
 assert(candidate.cycles==candidate.cpu.cyc);
 if(candidate.cpu.pc==entry_pc&&wf_audit_since_service==prefix_instructions)++entry_services;
 wf_service_audit();return !fail_service;
}
static void normalize(z80 *z)
{z->userdata=0;z->read_byte=0;z->write_byte=0;z->port_in=0;z->port_out=0;}
static void equal_state(void)
{
 z80 a=reference.cpu,b=candidate.cpu;normalize(&a);normalize(&b);
 assert(!memcmp(&a,&b,sizeof a));assert(!memcmp(reference.ram,candidate.ram,sizeof reference.ram));
 assert(reference.cycles==candidate.cycles&&reference.last_cpu==candidate.last_cpu);
 assert(reference.next_tb==candidate.next_tb&&reference.status==candidate.status&&reference.ctl==candidate.ctl);
 ++exact_cases;
}
static uint64_t setup(unsigned spec,unsigned seed)
{
 unsigned pc=pcs[spec];
 unsigned nops=spec==9?508u:(spec==10?510u:500u);
 prefix_instructions=nops+1u;prefix_cycles=nops*4u+10u;
 memcpy(rom,original_rom,sizeof rom);
 assert(wf_music_seq_init(&candidate,rom,sizeof rom,NULL,NULL,true));
 assert(wf_ref_init(&reference,rom,sizeof rom,NULL,NULL,false));
 for(unsigned i=0;i<nops;i++)rom[0x2000+i]=0;
 rom[0x2000+nops]=0xc3;rom[0x2000+nops+1]=(uint8_t)pc;rom[0x2000+nops+2]=(uint8_t)(pc>>8);
 unsigned instrument=0;
 while(instrument<256){unsigned at=0x135f+((instrument*2)&255u);if((rom[at]|((unsigned)rom[at+1]<<8))<=0xbfe7)break;++instrument;}
 assert(instrument<256);
 wf_music_seq_t *both[]={&candidate,&reference};
 for(unsigned which=0;which<2;which++){
  wf_music_seq_t *v=both[which];z80 *z=&v->cpu;
  z->pc=0x2000;z->sp=0xc7f0;z->a=pc==0xf3a?8:31;z->b=255;z->c=63;z->d=0x60;z->e=255;z->h=0x35;z->l=0x91;z->r=(uint8_t)seed;
  z->cyc=0;v->cycles=0;v->last_cpu=0;
  v->ram[0x65]=0;v->ram[7]=0;v->ram[0x63]=255;v->ram[0x82]=(uint8_t)instrument;v->ram[0x8e]=47;
  v->ram[0x7f0]=0;v->ram[0x7f1]=0x50;
  if(spec==9){v->ram[0x7f0]=0x71;v->ram[0x7f1]=0x0e;}
  if(pc==0xf3a){v->ram[0x7f0]=0x55;v->ram[0x7f1]=0x99;v->ram[0x7f2]=0xa5;v->ram[0x7f3]=0x47;v->ram[0x7f4]=0;v->ram[0x7f5]=0x50;}
 }
 wf_music_seq_t body=reference;body.cpu.userdata=&body;body.cpu.pc=(uint16_t)pc;
 unsigned end=pc==0xb68?0xb98:(spec>=5&&spec<=8?0xf5f:(spec==9?0xe6a:0x5000)),steps=0;
 do{wf_ref_z80_step(&body.cpu);assert(++steps<100);}while(body.cpu.pc!=end);
 original_body_steps=steps;
 if(spec>=5)assert(steps==quotas[spec]);
#ifdef WF_EARLY_QUOTA_TEST
 z80 quota_probe=candidate.cpu;quota_probe.pc=(uint16_t)pc;z80 quota_before=quota_probe;
 assert(wf_z80_lift_quota(&quota_probe)==quotas[spec]);assert(!memcmp(&quota_probe,&quota_before,sizeof quota_probe));
 if(pc==0xf5f){quota_probe.sp=0xbfff;assert(!wf_z80_lift_quota(&quota_probe));quota_probe.sp=0xc7ff;assert(!wf_z80_lift_quota(&quota_probe));}
#endif
 entry_pc=pc;entry_services=fail_service=0;wf_audit_since_service=0;wf_audit_service_enabled=1;
 return prefix_cycles+body.cpu.cyc;
}
int main(int argc,char **argv)
{
 assert(argc==2);FILE *f=fopen(argv[1],"rb");assert(f&&fread(original_rom,1,sizeof original_rom,f)==sizeof original_rom);assert(!fclose(f));
 unsigned missing=0;
 for(unsigned i=0;i<11;i++){
  uint64_t target=setup(i,i*47);int rc=wf_music_seq_run_serviced(&candidate,target,4096,service,NULL);assert(rc==0);
  assert(wf_ref_run(&reference,target,20000000)==0);equal_state();
  if(entry_services!=1){fprintf(stderr,"RED fragmented grant: PC%04X should receive service after%u original instructions before lift; observed=%u\n",pcs[i],prefix_instructions,entry_services);++missing;}
 }
 if(missing)return 2;
 for(unsigned i=0;i<11;i++){
  uint64_t target=setup(i,i*13);uint64_t before=wf_audit_retired;
  unsigned global_budget=prefix_instructions+original_body_steps-1u;
  int rc=wf_music_seq_run_serviced(&candidate,target,global_budget,service,NULL);assert(rc==1);
  assert(wf_audit_retired-before<=global_budget&&wf_audit_retired-before>prefix_instructions);assert(entry_services==0);
  do{rc=wf_music_seq_run_serviced(&candidate,target,4096,service,NULL);assert(rc>=0);}while(rc);
  assert(wf_ref_run(&reference,target,20000000)==0);equal_state();
  target=setup(i,i*17);wf_audit_service_enabled=0;
  assert(wf_music_seq_run(&candidate,target,4096)==0);
  assert(wf_ref_run(&reference,target,20000000)==0);equal_state();
  target=setup(i,i*19);fail_service=1;
  assert(wf_music_seq_run_serviced(&candidate,target,4096,service,NULL)==-1);
  assert(candidate.cpu.pc==pcs[i]&&candidate.cycles==prefix_cycles&&candidate.steps==prefix_instructions&&candidate.error);
  assert(wf_ref_run(&reference,prefix_cycles,20000000)==0);equal_state();
 }
 printf("PASS fragmented-grant causal cases=%"PRIu64" lift_entries=11; global-budget fallback, unserviced path and callback failure preserved\n",exact_cases);
 wf_service_audit_report();return 0;
}
