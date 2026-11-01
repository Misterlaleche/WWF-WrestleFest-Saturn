#include "wf_music_seq.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
extern unsigned wf_z80_control_batch(z80*,unsigned,uint32_t);
extern void wf_ref_z80_step(z80*);
extern bool wf_ref_init(wf_music_seq_t*,const uint8_t*,size_t,wf_music_emit_fn,void*,bool);
extern int wf_ref_run(wf_music_seq_t*,uint64_t,unsigned);
typedef struct {wf_music_seq_t *v;uint64_t hash,count;bool allow;unsigned mutation;} digest_t;
static wf_music_seq_t a,b;static uint8_t rom[65536];static digest_t da,db;
static unsigned rng=0x783298ad,checks,io_checks,declines;
static unsigned rand32(void){rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;return rng;}
static bool event(unsigned k,unsigned r,unsigned x,uint64_t t,void *u){
 digest_t*d=u;d->hash^=t^((uint64_t)k<<56)^((uint64_t)r<<48)^((uint64_t)x<<40);d->hash*=1099511628211ull;++d->count;
 if(k==WF_MUSIC_YM){z80*z=&d->v->cpu;unsigned m=d->mutation;
  if(m==1)z->nmi_pending=true;
  if(m==2||m==6||m==7){z->int_pending=true;z->iff1=true;z->interrupt_mode=(uint8_t)(m==6?0:m==7?2:1);z->int_data=255;z->i=0x20;}
  if(m==3||m==4){z->iff_delay=(uint8_t)(m==3?1:2);z->int_pending=true;z->iff1=false;z->iff2=false;}
  if(m==5){z->int_pending=true;z->iff1=false;}
 }
 return d->allow;
}
static void normal(z80*z){z->userdata=NULL;z->read_byte=NULL;z->write_byte=NULL;z->port_in=NULL;z->port_out=NULL;}
static void equal(int whole){
 z80 x=a.cpu,y=b.cpu;normal(&x);normal(&y);
 assert(!memcmp(&x,&y,sizeof x));assert(!memcmp(a.ram,b.ram,sizeof a.ram));
 assert(a.next_tb==b.next_tb&&a.status==b.status&&a.tb==b.tb&&a.ctl==b.ctl&&a.ym_addr==b.ym_addr&&a.latch==b.latch&&a.bank==b.bank&&a.command_pending==b.command_pending&&a.error==b.error);
 assert(da.count==db.count&&da.hash==db.hash);
 if(whole)assert(a.cycles==b.cycles&&a.last_cpu==b.last_cpu&&a.steps==b.steps);
 ++checks;
}
static void init(unsigned pc,uint32_t cycle){
 da=(digest_t){&a,1469598103934665603ull,0,true};db=(digest_t){&b,1469598103934665603ull,0,true};
 assert(wf_ref_init(&a,rom,sizeof rom,event,&da,false));assert(wf_music_seq_init(&b,rom,sizeof rom,event,&db,false));b.optimized=true;
 a.cpu.pc=b.cpu.pc=(uint16_t)pc;a.cpu.sp=b.cpu.sp=0xc7fc;a.cpu.cyc=b.cpu.cyc=cycle;
 a.cycles=b.cycles=cycle;a.last_cpu=b.last_cpu=cycle;
}
static void clone_random(void){
 b=a;b.cpu.userdata=&b;
 /* Keep candidate I/O callbacks; the reference callbacks intentionally do not
  * update the new metadata. Functional state is compared field by field. */
 wf_music_seq_t adapter;assert(wf_music_seq_init(&adapter,rom,sizeof rom,NULL,NULL,false));
 b.cpu.read_byte=adapter.cpu.read_byte;b.cpu.write_byte=adapter.cpu.write_byte;b.cpu.port_in=adapter.cpu.port_in;b.cpu.port_out=adapter.cpu.port_out;
 b.user=&db;db=da;db.v=&b;
}
int main(int argc,char**argv){
 FILE*f=NULL;assert(argc==2&&!fopen_s(&f,argv[1],"rb"));assert(fread(rom,1,sizeof rom,f)==sizeof rom);fclose(f);
 /* Real frozen-ROM instruction paths, randomized state, budgets and signed
  * limits around 32-bit wrap. Execute exactly the count accepted by the batch. */
 for(unsigned i=0;i<100000;i++){
  init(rand32()%0xbffcu,(i&1)?0xfffffffdu:rand32());
  a.cpu.a=rand32();a.cpu.b=rand32();a.cpu.c=rand32();a.cpu.d=rand32();a.cpu.e=rand32();a.cpu.h=rand32();a.cpu.l=rand32();
  a.cpu.sp=0xc000+(rand32()&2047);a.cpu.iff1=rand32()&1;a.cpu.iff2=rand32()&1;a.cpu.interrupt_mode=rand32()%3;
  a.cpu.sf=rand32()&1;a.cpu.zf=rand32()&1;a.cpu.yf=rand32()&1;a.cpu.hf=rand32()&1;a.cpu.xf=rand32()&1;a.cpu.pf=rand32()&1;a.cpu.nf=rand32()&1;a.cpu.cf=rand32()&1;
  for(unsigned j=0;j<sizeof a.ram;j++)a.ram[j]=(uint8_t)rand32();
  a.tb=rand32();a.ctl=rand32();a.ym_addr=rand32();a.next_tb=rand32();a.status=rand32()&3;da.allow=(rand32()&1)!=0;clone_random();
  unsigned budget=rand32()%65;uint32_t limit=(uint32_t)b.cpu.cyc+rand32()%129;
  unsigned n=wf_z80_control_batch(&b.cpu,budget,limit);assert(n<=budget);
  for(unsigned j=0;j<n;j++){assert((int32_t)(limit-(uint32_t)a.cpu.cyc)>0);wf_ref_z80_step(&a.cpu);}
  equal(0);if(!n)++declines;if(b.io_epoch)++io_checks;
 }
 /* Synthetic controller-only test programs exercise real memory adapters.
  * This mutable ROM exists only in HOST tests and cannot pass native CRC. */
 static const uint8_t programs[][24]={
  {0x00,0x3c,0x32,0x00,0xc0,0x3a,0x00,0xc0,0xc3,0x00,0x40},
  {0x3e,0x12,0x32,0x00,0xc8,0x3e,0xff,0x32,0x01,0xc8,0x3e,0x14,0x32,0x00,0xc8,0x3e,0x0a,0x32,0x01,0xc8,0xc3,0x00,0x40},
  {0x3e,0x14,0x32,0x00,0xc8,0x3e,0x00,0x32,0x01,0xc8,0x3a,0x00,0xc8,0x3a,0x00,0xe0,0xc3,0x00,0x40},
  {0x3e,0x14,0x32,0x00,0xc8,0x3e,0x2a,0x32,0x01,0xc8,0x3e,0x55,0x32,0x00,0xd8,0xc3,0x00,0x40},
  {0x3e,0x01,0x32,0x00,0xe8,0x3a,0x01,0xc8,0x3a,0x00,0xd8,0x32,0xff,0xff,0xc3,0x00,0x40}
 };
 for(unsigned p=0;p<sizeof programs/sizeof*programs;p++){
  memcpy(rom+0x4000,programs[p],sizeof programs[p]);
  for(unsigned wrap=0;wrap<2;wrap++)for(unsigned ctl=0;ctl<4;ctl++)for(unsigned fail=0;fail<2;fail++)
  for(unsigned budget=0;budget<=32;budget++)for(unsigned span=1;span<=160;span++){
   uint32_t cycle=wrap?0xfffffff0u:100000u;init(0x4000,cycle);
   a.tb=b.tb=255;a.ctl=b.ctl=(uint8_t)(ctl==0?0:ctl==1?2:ctl==2?10:42);a.next_tb=b.next_tb=cycle+((span&3)?13:0);
   a.status=b.status=2;a.command_pending=b.command_pending=true;a.latch=b.latch=7;da.allow=db.allow=!fail;
   int x=wf_ref_run(&a,a.cycles+span,budget),y=wf_music_seq_run(&b,b.cycles+span,budget);assert(x==y);equal(1);
  }
 }
 /* Direct admission guards and actual I/O barriers, including return to003A. */
 static const uint8_t guards[]={0x00,0x00,0xc3,0x3a,0x00};memcpy(rom+0x4000,guards,sizeof guards);
 for(unsigned g=0;g<9;g++){
  init(0x4000,0xfffffff0u);a.cpu.iff1=b.cpu.iff1=true;
  if(g==0)a.cpu.nmi_pending=b.cpu.nmi_pending=true;
  if(g==1)a.cpu.int_pending=b.cpu.int_pending=true;
  if(g==2)a.cpu.iff_delay=b.cpu.iff_delay=1;
  if(g==3)a.cpu.halted=b.cpu.halted=true;
  if(g==4)a.error=b.error=true;
  unsigned n=wf_z80_control_batch(&b.cpu,g==5?0:32,(uint32_t)b.cpu.cyc+(g==6?0:100));
  if(g<=6)assert(n==0);else assert(n==3&&b.cpu.pc==0x003a);
  for(unsigned j=0;j<n;j++)wf_ref_z80_step(&a.cpu);equal(0);
 }
 for(unsigned address=0;address<6;address++)for(unsigned write=0;write<2;write++)for(unsigned allow=0;allow<2;allow++){
  static const uint16_t io[]={0xc800,0xc801,0xd800,0xe000,0xe800,0xffff};uint16_t at=io[address];
  uint8_t code[]={write?0x32:0x3a,(uint8_t)at,(uint8_t)(at>>8),0x00,0x00};memcpy(rom+0x4000,code,sizeof code);init(0x4000,100);
  a.cpu.a=b.cpu.a=0x0a;a.ym_addr=b.ym_addr=0x14;da.allow=db.allow=allow!=0;
  unsigned n=wf_z80_control_batch(&b.cpu,32,200);assert(n==1&&b.io_epoch==1);wf_ref_z80_step(&a.cpu);equal(0);++io_checks;
 }
 /* A callback may create CPU control conditions after the entry predicates.
  * Repeat the reviewed scalar forms through the batch and REAL seq adapters.
  * irq-tail still runs in scalar_step; the epoch must prevent a second step. */
 static const unsigned stores[]={0x70,0x71,0x72,0x73,0x74,0x75,0x77,0x36};
 unsigned mutations=0;
 for(unsigned form=0;form<20;form++)for(unsigned mutation=0;mutation<8;mutation++)for(unsigned allow=0;allow<2;allow++)for(unsigned i=0;i<200;i++){
  unsigned op=form==0?0x32:form==17?0x77:form==18?0xf5:form==19?0x35:form<=8?0xdd:0xfd;
  unsigned sub=form>0&&form<17?stores[(form-1)%8]:0;
  uint8_t code[]={(uint8_t)op,(uint8_t)(form?sub:1),(uint8_t)(form?0:0xc8),0x5a,0};memcpy(rom+0x4000,code,sizeof code);
  init(0x4000,(i&1)?0xfffffffcu:rand32());a.cpu.sp=b.cpu.sp=form==18?0xc802:0xc7fe;
  a.cpu.ix=b.cpu.ix=0xc801;a.cpu.iy=b.cpu.iy=0xc801;
  if(form==17||form==19){a.cpu.h=b.cpu.h=0xc8;a.cpu.l=b.cpu.l=1;}
  a.cpu.a=b.cpu.a=(uint8_t)rand32();a.cpu.iff1=b.cpu.iff1=(i&2)!=0;a.cpu.iff2=b.cpu.iff2=(i&4)!=0;
  a.ym_addr=b.ym_addr=(uint8_t)(i%3==0?0x12:i%3==1?0x14:8);a.tb=b.tb=255;
  da.allow=db.allow=allow!=0;da.mutation=db.mutation=mutation;
  unsigned n=wf_z80_control_batch(&b.cpu,32,(uint32_t)b.cpu.cyc+1000);assert(n==1&&b.io_epoch>0);
  wf_ref_z80_step(&a.cpu);equal(0);++mutations;
 }
 printf("PASS post-callback control through real adapters: %u exact cases,20 scalar forms, NMI/IRQ modes0/1/2/iff_delay1/2/maskedIRQ/callbacktruefalse/32wrap\n",mutations);
 printf("PASS control-batch: %u functional-state checks, %u I/O barriers, %u immutable randomized declines; target/budget/timer32wrap/actualTBCTL/callbackfalse/IRQ/NMI/delay/halt/003A\n",checks,io_checks,declines);return 0;
}
