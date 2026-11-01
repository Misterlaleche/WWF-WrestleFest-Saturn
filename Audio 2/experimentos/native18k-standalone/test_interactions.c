#include "wf_music_seq.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
extern unsigned wf_z80_control_batch(z80*,unsigned,uint32_t);
extern unsigned wf_z80_control_fast_step(z80*,unsigned,uint32_t);
extern unsigned wf_z80_scalar_step(z80*,unsigned,uint32_t);
extern void wf_ref_z80_step(z80*);
typedef struct{wf_music_seq_t *seq;unsigned mode,count;uint64_t time;z80 cpu;uint8_t ram[2048];} trace_t;
static wf_music_seq_t seed,a,b;static trace_t ta,tb;static uint8_t rom[65536];
static unsigned rng=0x917cc451u;
static unsigned case_mode,case_register,case_iteration,case_phase;
static unsigned random32(void){rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;return rng;}
static bool event(unsigned kind,unsigned reg,unsigned value,uint64_t cycle,void *user){
 trace_t*t=user;wf_music_seq_t*v=t->seq;(void)reg;(void)value;
 ++t->count;t->time=cycle;t->cpu=v->cpu;t->cpu.userdata=NULL;memcpy(t->ram,v->ram,sizeof t->ram);
 if(kind==WF_MUSIC_YM){
  if(t->mode==2)v->cpu.nmi_pending=true;
  if(t->mode==3){v->cpu.int_pending=true;v->cpu.iff1=true;v->cpu.interrupt_mode=2;v->cpu.i=0x20;v->cpu.int_data=255;}
  if(t->mode==4){v->cpu.iff_delay=2;v->cpu.int_pending=true;v->cpu.iff1=false;}
  if(t->mode==5)v->cpu.pc=0x003a;
  if(t->mode==6){unsigned at=v->cpu.sp-0xc000u;assert(at<2047);v->ram[at]=0x99;v->ram[at+1]=0;}
  if(t->mode==7){v->cpu.int_pending=true;v->cpu.iff1=false;}
 }
 return t->mode!=1;
}
static void equal(void){
 wf_music_seq_t x=a,y=b;x.cpu.userdata=y.cpu.userdata=NULL;x.user=y.user=NULL;
 /* Activity metadata counts executed adapter calls: the proven YM lift
  * removes a pure status poll. Compare its required barrier separately. */
 x.io_epoch=y.io_epoch=0;
 if(memcmp(&x,&y,sizeof x)){
  printf("DIFF mode=%u reg=%u i=%u phase=%u pc=%x/%x cyc=%lu/%lu io=%u/%u steps=%llu/%llu\n",case_mode,case_register,case_iteration,case_phase,x.cpu.pc,y.cpu.pc,x.cpu.cyc,y.cpu.cyc,x.io_epoch,y.io_epoch,(unsigned long long)x.steps,(unsigned long long)y.steps);
  for(unsigned j=0;j<sizeof x;j++)if(((unsigned char*)&x)[j]!=((unsigned char*)&y)[j])printf("byte%u %02x/%02x\n",j,((unsigned char*)&x)[j],((unsigned char*)&y)[j]);
  fflush(stdout);assert(0);
 }
 assert(ta.count==tb.count&&ta.time==tb.time);
 assert(!memcmp(&ta.cpu,&tb.cpu,sizeof ta.cpu)&&!memcmp(ta.ram,tb.ram,sizeof ta.ram));
}
int main(int argc,char**argv){
 FILE*f=NULL;assert(argc==2&&!fopen_s(&f,argv[1],"rb"));assert(fread(rom,1,sizeof rom,f)==sizeof rom);fclose(f);
 assert(wf_music_seq_init(&seed,rom,sizeof rom,event,NULL,true));
 unsigned checks=0,post5=0,barriers=0;
 const unsigned registers[]={0x20,0x12,0x14};
 for(unsigned mode=0;mode<8;mode++)for(unsigned ri=0;ri<3;ri++)for(unsigned i=0;i<1000;i++){
  case_mode=mode;case_register=ri;case_iteration=i;case_phase=0;
  seed.cpu.pc=0xe6a;seed.cpu.sp=0xc7f0;seed.cpu.ix=0xc010;seed.cpu.h=0xc1;seed.cpu.l=0;
  seed.cpu.b=(uint8_t)(1u+(i&1u));seed.cpu.a=(uint8_t)random32();seed.cpu.f_=(uint8_t)random32();seed.cpu.a_=(uint8_t)random32();
  seed.cpu.cyc=i%3==0?0xfffffff0u:random32();seed.last_cpu=(uint32_t)seed.cpu.cyc;seed.cycles=0x1234567800ull;
  seed.cpu.iff_delay=0;seed.cpu.nmi_pending=seed.cpu.int_pending=seed.cpu.halted=false;seed.cpu.iff1=seed.cpu.iff2=false;
  seed.status=0;seed.ctl=0;seed.tb=0xff;seed.error=false;seed.io_epoch=i&1?0xffffffffu:0;
  for(unsigned j=0;j<sizeof seed.ram;j++)seed.ram[j]=(uint8_t)random32();
  seed.ram[0x10]=(uint8_t)registers[ri];seed.ram[0x100]=(uint8_t)random32();
  a=seed;b=seed;a.cpu.userdata=&a;b.cpu.userdata=&b;
  memset(&ta,0,sizeof ta);memset(&tb,0,sizeof tb);ta.seq=&a;tb.seq=&b;ta.mode=tb.mode=mode;a.user=&ta;b.user=&tb;
  uint32_t begin=(uint32_t)b.cpu.cyc;
  for(unsigned j=0;j<15;j++)wf_ref_z80_step(&a.cpu);
  unsigned n=wf_z80_control_batch(&b.cpu,100,begin+1000u);
  assert(n==15&&tb.count==1&&a.io_epoch!=seed.io_epoch&&b.io_epoch!=seed.io_epoch);equal();++checks;++barriers;
  if(mode==0){
   case_phase=1;
   assert(b.cpu.pc==0xf5f);for(unsigned j=0;j<5;j++)wf_ref_z80_step(&a.cpu);
   assert(wf_z80_control_batch(&b.cpu,5,begin+1000u)==5);equal();++checks;++post5;
  }else if(mode==1){
   case_phase=2;
   wf_music_seq_t saved=b;assert(b.error&&wf_z80_control_batch(&b.cpu,100,begin+1000u)==0);
   assert(!memcmp(&saved,&b,sizeof b));++checks;
  }else{
   case_phase=3;
   wf_music_seq_t saved=b;assert(wf_z80_control_fast_step(&b.cpu,1,begin+1000u)==0);
   assert(!memcmp(&saved,&b,sizeof b));
   wf_ref_z80_step(&a.cpu);
   n=wf_z80_scalar_step(&b.cpu,1,begin+1000u);if(!n)z80_step(&b.cpu);else assert(n==1);
   equal();++checks;
  }
 }
 printf("PASS combined PRE/POST: %u full CPU/RAM/timer/callback-state+time checks; %u mandatory post-C801 I/O barriers; %u exact POST5 continuations; callbackfalse,NMI,IM2,delay,PC/return mutations,epoch/cycle wrap\n",checks,barriers,post5);
 return 0;
}
