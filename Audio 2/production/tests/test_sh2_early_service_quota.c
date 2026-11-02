/* Private causal test: unchanged original 08C1 reached with a fragmented
 * 512-instruction service grant. The original scalar CPU verifies outcomes.
 * The quota bound is checked separately against the existing fast operation. */
#include "wf_music_seq.h"
#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern bool wf_ref_init(wf_music_seq_t *,const uint8_t *,size_t,wf_music_emit_fn,void *,bool);
extern int wf_ref_run(wf_music_seq_t *,uint64_t,unsigned);
extern void wf_ref_z80_step(z80 *);
#ifdef WF_EARLY_QUOTA_TEST
extern unsigned wf_z80_lift_quota(z80 *);
extern unsigned wf_z80_control_fast_step(z80 *,unsigned,uint32_t);
#endif
extern uint64_t wf_audit_since_service,wf_audit_retired,wf_audit_services,wf_audit_max_service;
extern unsigned wf_audit_max_grant;
extern int wf_audit_service_enabled;
extern void wf_service_audit(void),wf_service_audit_report(void);
static uint8_t original_rom[65536],rom[65536];
static wf_music_seq_t candidate,reference;
static unsigned prefix_instructions,entry_services,fail_service,original_body_steps;
static uint64_t prefix_cycles,exact_cases;
#ifdef WF_EARLY_QUOTA_TEST
static uint64_t bound_cases;
#endif
typedef struct {
 unsigned a,offset,e,active,control0,control1,delta0,delta1,steps,quota;
} spec_t;
static const spec_t specs[]={
 {0x10,0x00,0,0,0,0,0,0,57,58},
 {0x10,0x81,0,0,0,0,0,0,62,63},
 {0xff,0x00,0,0,0,0,0,0,58,58},
 {0xff,0x81,0,0,0,0,0,0,63,63},
 {0x10,0x00,0,1,1,0,0x10,0,75,79},
 {0x10,0x00,0,1,0,0,1,0,77,79},
 {0x60,0x81,0xff,3,1,0,0x100,0,100,106},
 {0x61,0x81,0,3,1,0,0,1,102,106},
 {0x10,0x81,0,3,0,1,0xffff,0xffff,106,106},
};

void wf_music_unknown_opcode(void){abort();}
static bool service(void *unused)
{
 (void)unused;
 assert(candidate.cycles==candidate.cpu.cyc);
 if(candidate.cpu.pc==0x08c1&&wf_audit_since_service==prefix_instructions)++entry_services;
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
static void configure(wf_music_seq_t *v,const spec_t *s,unsigned ch)
{
 z80 *z=&v->cpu;z->a=(uint8_t)s->a;v->ram[0x65]=(uint8_t)ch;
 v->ram[0xaa+ch]=(uint8_t)s->offset;v->ram[0xb2+ch]=(uint8_t)s->e;
 for(unsigned mod=0;mod<2;mod++){
  unsigned at=(mod?0x112:0xd2)+8*ch,delta=mod?s->delta1:s->delta0;
  v->ram[at]=(uint8_t)((s->active>>mod)&1u);
  v->ram[at+1]=(uint8_t)(mod?s->control1:s->control0);
  v->ram[at+2]=(uint8_t)(delta>>8);v->ram[at+3]=(uint8_t)delta;
 }
}
static uint64_t setup(unsigned index,unsigned remaining)
{
 const spec_t *s=&specs[index];unsigned nops=511u-remaining;
 prefix_instructions=nops+1u;prefix_cycles=nops*4u+10u;
 memcpy(rom,original_rom,sizeof rom);
 assert(wf_music_seq_init(&candidate,rom,sizeof rom,NULL,NULL,true));
 assert(wf_ref_init(&reference,rom,sizeof rom,NULL,NULL,false));
 memset(rom+0x2000,0,nops);
 rom[0x2000+nops]=0xc3;rom[0x2000+nops+1]=0xc1;rom[0x2000+nops+2]=8;
 wf_music_seq_t *both[]={&candidate,&reference};
 for(unsigned which=0;which<2;which++){
  wf_music_seq_t *v=both[which];z80 *z=&v->cpu;
  z->pc=0x2000;z->sp=0xc7f0;z->b=0xff;z->c=0x63;z->d=0x37;z->e=0xa9;
  z->h=0x35;z->l=0x91;z->r=(uint8_t)(remaining*13u);z->a_=0x51;z->f_=0x83;
  z->cyc=0;v->cycles=0;v->last_cpu=0;
  v->ram[0x7f0]=0;v->ram[0x7f1]=0x50;
  configure(v,s,index&7u);
 }
 wf_music_seq_t body=reference;body.cpu.userdata=&body;body.cpu.pc=0x08c1;
 unsigned steps=0;
 do{wf_ref_z80_step(&body.cpu);assert(++steps<130);}while(body.cpu.pc!=0x5000);
 if(steps!=s->steps){fprintf(stderr,"spec%u expected%u actual%u\n",index,s->steps,steps);abort();}
 original_body_steps=steps;entry_services=fail_service=0;
 wf_audit_since_service=0;wf_audit_service_enabled=1;
 return prefix_cycles+body.cpu.cyc;
}
static unsigned causal(unsigned index,unsigned remaining)
{
 uint64_t target=setup(index,remaining);
 assert(wf_music_seq_run_serviced(&candidate,target,4096,service,NULL)==0);
 assert(wf_ref_run(&reference,target,20000000)==0);equal_state();
 if(entry_services!=1){
  fprintf(stderr,"RED 08C1 grant: spec%u body%u remaining%u; expected early service at%u instructions, observed%u\n",
   index,original_body_steps,remaining,prefix_instructions,entry_services);
  return 1;
 }
 return 0;
}
#ifdef WF_EARLY_QUOTA_TEST
static void quota_bound(void)
{
 static const unsigned av[]={0,1,0x5f,0x60,0x61,0x7f,0x80,0xff};
 static const unsigned off[]={0,1,0x7f,0x80,0x81,0xff};
 static const unsigned ev[]={0,1,0x7f,0xff};
 static const unsigned dv[]={0,1,0x5fff,0x6000,0x7fff,0x8000,0xffff};
 setup(0,1);wf_music_seq_t base=candidate,trial,before;
 base.cpu.pc=0x08c1;unsigned max_actual=0,max_quota=0;
 for(unsigned a=0;a<8;a++)for(unsigned o=0;o<6;o++)for(unsigned e=0;e<4;e++)
 for(unsigned mask=0;mask<4;mask++)for(unsigned controls=0;controls<16;controls++)
 for(unsigned d0=0;d0<7;d0++)for(unsigned d1=0;d1<7;d1++){
  trial=base;trial.cpu.userdata=&trial;
  spec_t s={av[a],off[o],ev[e],mask,controls&3u,controls>>2,dv[d0],dv[d1],0,0};
  configure(&trial,&s,(unsigned)bound_cases&7u);
  before=trial;unsigned quota=wf_z80_lift_quota(&trial.cpu);
  assert(!memcmp(&trial,&before,sizeof trial));assert(quota>=57u&&quota<=106u);
  unsigned actual=wf_z80_control_fast_step(&trial.cpu,106,10000);
  assert(actual>0&&actual<=quota);
  if(actual>max_actual)max_actual=actual;
  if(quota>max_quota)max_quota=quota;
  ++bound_cases;
 }
 assert(max_actual==106&&max_quota==106);
 static const unsigned invalid_sp[]={0,0xbfff,0xc000,0xc407,0xc7ff,0xc800,0xffff};
 for(unsigned i=0;i<sizeof invalid_sp/sizeof invalid_sp[0];i++){
  trial=base;trial.cpu.userdata=&trial;trial.cpu.sp=(uint16_t)invalid_sp[i];before=trial;
  assert(wf_z80_lift_quota(&trial.cpu)==0);assert(!memcmp(&trial,&before,sizeof trial));++bound_cases;
 }
 for(unsigned ch=8;ch<256;ch++){
  trial=base;trial.cpu.userdata=&trial;trial.ram[0x65]=(uint8_t)ch;before=trial;
  assert(wf_z80_lift_quota(&trial.cpu)==0);assert(!memcmp(&trial,&before,sizeof trial));++bound_cases;
 }
 for(unsigned sp=0xc408;sp<=0xc7fe;sp+=0x3f6){
  trial=base;trial.cpu.userdata=&trial;trial.cpu.sp=(uint16_t)sp;before=trial;
  assert(wf_z80_lift_quota(&trial.cpu)==58);assert(!memcmp(&trial,&before,sizeof trial));++bound_cases;
 }
 printf("PASS 08C1 quota bound cases=%"PRIu64" max_actual=%u max_quota=%u; read-only guard rejection\n",bound_cases,max_actual,max_quota);
}
#endif
int main(int argc,char **argv)
{
 assert(argc==2);FILE *f=fopen(argv[1],"rb");assert(f&&fread(original_rom,1,sizeof original_rom,f)==sizeof original_rom);assert(!fclose(f));
 unsigned missing=0;
 for(unsigned i=0;i<sizeof specs/sizeof specs[0];i++)missing+=causal(i,specs[i].steps-1u);
 for(unsigned remaining=78;remaining<=105;remaining++)missing+=causal(8,remaining);
 missing+=causal(8,1);
 if(missing)return 2;
 for(unsigned i=0;i<sizeof specs/sizeof specs[0];i++){
  uint64_t target=setup(i,specs[i].steps-1u),before=wf_audit_retired;
  unsigned budget=prefix_instructions+original_body_steps-1u;
  int rc=wf_music_seq_run_serviced(&candidate,target,budget,service,NULL);
  assert(rc==1&&wf_audit_retired-before<=budget&&wf_audit_retired-before>prefix_instructions&&entry_services==0);
  do{rc=wf_music_seq_run_serviced(&candidate,target,4096,service,NULL);assert(rc>=0);}while(rc);
  assert(wf_ref_run(&reference,target,20000000)==0);equal_state();
  target=setup(i,specs[i].steps-1u);wf_audit_service_enabled=0;
  assert(wf_music_seq_run(&candidate,target,4096)==0);assert(wf_ref_run(&reference,target,20000000)==0);equal_state();
  target=setup(i,specs[i].steps-1u);fail_service=1;
  assert(wf_music_seq_run_serviced(&candidate,target,4096,service,NULL)==-1);
  assert(candidate.cpu.pc==0x08c1&&candidate.cycles==prefix_cycles&&candidate.steps==prefix_instructions&&candidate.error);
  assert(wf_ref_run(&reference,prefix_cycles,20000000)==0);equal_state();
#ifdef WF_EARLY_QUOTA_TEST
  z80 probe=candidate.cpu;probe.userdata=&candidate;
  wf_music_seq_t state=candidate;unsigned quota=wf_z80_lift_quota(&probe);
  assert(quota==specs[i].quota&&probe.pc==candidate.cpu.pc);assert(!memcmp(&state,&candidate,sizeof state));
#endif
 }
 printf("PASS 08C1 fragmented-grant causal cases=%"PRIu64"; global budget, no-service and callback-failure paths exact\n",exact_cases);
 wf_service_audit_report();
#ifdef WF_EARLY_QUOTA_TEST
 quota_bound();
#endif
 return 0;
}
