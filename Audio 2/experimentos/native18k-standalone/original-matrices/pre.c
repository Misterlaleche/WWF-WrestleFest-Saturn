#include "wf_music_seq.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
extern unsigned wf_z80_control_fast_step(z80 *,unsigned,uint32_t);
extern void wf_ref_z80_init(z80 *);
extern void wf_ref_z80_step(z80 *);
typedef struct {unsigned kind,reg,value;uint64_t time,ram_hash;z80 cpu;uint8_t status,tb,ctl,latch,bank,ym;bool error,pending;} event_t;
typedef struct {wf_music_seq_t seq;event_t events[8];unsigned count;bool response;unsigned mutation;} fixture_t;
static fixture_t a,b,before;static uint8_t rom[65536];static unsigned rng=0x7d83c191,case_op,case_sub,case_index;
static uint64_t checks,successes,declines,disabled_irq_successes,guard_declines,cold_checks,io_events;
static unsigned random32(void){rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;return rng;}
static void normalize(z80 *z){z->userdata=NULL;}
static uint64_t ram_hash(const uint8_t*p){uint64_t h=1469598103934665603ull;for(unsigned i=0;i<2048;i++){h^=p[i];h*=1099511628211ull;}return h;}
static void record(wf_music_seq_t *s,unsigned kind,unsigned reg,unsigned value){
 fixture_t *f=(fixture_t*)s;assert(f->count<8);event_t *e=&f->events[f->count++];
 e->kind=kind;e->reg=reg;e->value=value;e->time=s->cycles+(uint32_t)((uint32_t)s->cpu.cyc-s->last_cpu);e->ram_hash=ram_hash(s->ram);
 e->cpu=s->cpu;normalize(&e->cpu);e->status=s->status;e->tb=s->tb;e->ctl=s->ctl;e->latch=s->latch;e->bank=s->bank;e->ym=s->ym_addr;e->error=s->error;e->pending=s->command_pending;
}
static void emit(wf_music_seq_t*s,unsigned kind,unsigned reg,unsigned value){
 record(s,kind,reg,value);fixture_t*f=(fixture_t*)s;z80*z=&s->cpu;
 if(kind==WF_MUSIC_YM){
  if(f->mutation==1)z->nmi_pending=true;
  if(f->mutation==2||f->mutation==6||f->mutation==7){z->int_pending=true;z->iff1=true;z->interrupt_mode=(uint8_t)(f->mutation==6?0:f->mutation==7?2:1);z->int_data=255;z->i=0x20;}
  if(f->mutation==3||f->mutation==4){z->iff_delay=(uint8_t)(f->mutation==3?1:2);z->int_pending=true;z->iff1=false;z->iff2=false;}
  if(f->mutation==5){z->int_pending=true;z->iff1=false;}
 }
 if(!f->response)s->error=true;
}
static uint8_t read_byte(void *u,uint16_t addr){
 wf_music_seq_t*s=u;
 if(addr<0xc000)return s->rom[addr];if(addr<0xc800)return s->ram[addr-0xc000];
 if(addr==0xc800||addr==0xc801){return s->status;}
 if(addr==0xd800){record(s,4,addr,0);return 0;}
 if(addr==0xe000){s->command_pending=false;record(s,4,addr,s->latch);return s->latch;}
 record(s,4,addr,255);return 255;
}
static void write_byte(void*u,uint16_t addr,uint8_t value){
 wf_music_seq_t*s=u;
 if(addr>=0xc000&&addr<0xc800){s->ram[addr-0xc000]=value;return;}
 if(addr==0xc800){s->ym_addr=value;return;}
 if(addr==0xc801){
  emit(s,WF_MUSIC_YM,s->ym_addr,value);
  if(s->ym_addr==0x12)s->tb=value;
  if(s->ym_addr==0x14){uint8_t old=s->ctl;s->ctl=value;if(value&32)s->status&=0xfd;if((value&2)&&!(old&2))s->next_tb=(uint32_t)s->cpu.cyc+(256u-s->tb)*1024u;}
 }else if(addr==0xd800)emit(s,WF_MUSIC_OKI,0,value);
 else if(addr==0xe800){s->bank=value&1;emit(s,WF_MUSIC_BANK,0,value);}
}
static uint8_t port_in(z80*z,uint8_t p){(void)z;(void)p;return 255;}
static void port_out(z80*z,uint8_t p,uint8_t v){(void)z;(void)p;(void)v;}
static const unsigned edges[]={0,0xbfff,0xc000,0xc7fe,0xc7ff,0xc800,0xc801,0xd800,0xe000,0xe800,0xffff};
static const unsigned fetches[]={0x2000,0xbffb,0xbffc,0xbffd,0xbffe,0xbfff,0xc000,0xc7ff,0xc800,0xe000,0xffff};
static void setup(unsigned pc,unsigned iteration){
 memset(&a,0,sizeof a);wf_ref_z80_init(&a.seq.cpu);wf_music_seq_t*s=&a.seq;z80*z=&s->cpu;
 s->rom=rom;for(unsigned i=0;i<2048;i++)s->ram[i]=(uint8_t)random32();
 z->a=random32();z->b=random32();z->c=random32();z->d=random32();z->e=random32();z->h=random32();z->l=random32();
 z->a_=random32();z->b_=random32();z->c_=random32();z->d_=random32();z->e_=random32();z->h_=random32();z->l_=random32();z->f_=random32();
 z->i=random32();z->r=random32();z->ix=random32();z->iy=random32();z->sp=random32();z->mem_ptr=random32();
 z->sf=random32()&1;z->zf=random32()&1;z->yf=random32()&1;z->hf=random32()&1;z->xf=random32()&1;z->pf=random32()&1;z->nf=random32()&1;z->cf=random32()&1;
 z->iff1=random32()&1;z->iff2=random32()&1;z->interrupt_mode=random32()%3;z->int_data=random32();
 z->cyc=iteration%5?random32():0xfffffffcu;z->pc=(uint16_t)pc;
 unsigned addr=edges[iteration%(sizeof edges/sizeof edges[0])];
 if(iteration&1){z->sp=(uint16_t)addr;z->h=(uint8_t)(addr>>8);z->l=(uint8_t)addr;z->ix=(uint16_t)addr;z->iy=(uint16_t)addr;}
 s->cycles=((uint64_t)random32()<<16)|random32();s->last_cpu=(uint32_t)z->cyc-random32();s->tb=random32();s->status=random32()&3;s->ctl=random32();s->latch=random32();s->bank=random32()&1;s->ym_addr=(uint8_t)(iteration%3==0?0x12:iteration%3==1?0x14:random32());s->next_tb=random32();s->command_pending=(iteration&1)!=0;
 z->userdata=s;z->read_byte=read_byte;z->write_byte=write_byte;z->port_in=port_in;z->port_out=port_out;a.response=(iteration&2)!=0;
}
static void inject(unsigned at,unsigned value){at&=65535;if(at<0xc000)rom[at]=(uint8_t)value;else if(at<0xc800)a.seq.ram[at-0xc000]=(uint8_t)value;}
static void clone(void){b=a;b.seq.cpu.userdata=&b.seq;}
static void fail(const char*why){printf("FAIL %s op=%02x sub=%02x case=%u PCa=%04x PCb=%04x CYCa=%lu CYCb=%lu events=%u/%u\n",why,case_op,case_sub,case_index,a.seq.cpu.pc,b.seq.cpu.pc,a.seq.cpu.cyc,b.seq.cpu.cyc,a.count,b.count);fflush(stdout);abort();}
static void equal(void){
 wf_music_seq_t x=a.seq,y=b.seq;normalize(&x.cpu);normalize(&y.cpu);
 if(memcmp(&x,&y,sizeof x))fail("full CPU/sequencer/RAM");
 if(a.count!=b.count||memcmp(a.events,b.events,sizeof a.events))fail("I/O callback state/time/order");
 io_events+=a.count;
}

static uint64_t exacts,guards;
static void run_exact(void){clone();for(unsigned i=0;i<15;i++)wf_ref_z80_step(&a.seq.cpu);unsigned n=wf_z80_control_fast_step(&b.seq.cpu,15,(uint32_t)before.seq.cpu.cyc+124);if(n!=15)fail("PRE expected15");equal();++exacts;}
static void good(unsigned i){setup(0xe6a,i);a.seq.cpu.sp=(uint16_t)(0xc002+random32()%2047);a.seq.cpu.ix=(uint16_t)(random32()%0xc800);a.seq.cpu.h=(uint8_t)((random32()%0xc800)>>8);a.seq.cpu.l=(uint8_t)random32();a.seq.status&=127;a.seq.cpu.nmi_pending=false;a.seq.cpu.int_pending=false;a.seq.cpu.iff_delay=0;a.seq.cpu.halted=false;before=a;}
int main(int argc,char**argv){FILE*f=NULL;assert(argc==2&&!fopen_s(&f,argv[1],"rb"));assert(fread(rom,1,65536,f)==65536);fclose(f);
 good(0);run_exact();
 for(unsigned i=0;i<50000;i++){good(i);a.mutation=i%8;before=a;run_exact();}
 const unsigned addr[]={0,1,0xbffe,0xbfff,0xc000,0xc001,0xc7fe,0xc7ff};
 for(unsigned k=0;k<8;k++)for(unsigned l=0;l<8;l++)for(unsigned i=0;i<128;i++){
  good(i);a.seq.cpu.ix=(uint16_t)addr[k];a.seq.cpu.h=(uint8_t)(addr[l]>>8);a.seq.cpu.l=(uint8_t)addr[l];a.seq.cpu.sp=i&1?0xc002:0xc800;a.mutation=i%8;before=a;run_exact();
 }
 /* Reads must precede CALL stack stores, including source aliases. */
 for(unsigned i=0;i<4096;i++){good(i);unsigned sp=a.seq.cpu.sp;a.seq.cpu.ix=(uint16_t)(sp-2);unsigned hl=sp-1;a.seq.cpu.h=(uint8_t)(hl>>8);a.seq.cpu.l=(uint8_t)hl;a.mutation=i%8;before=a;run_exact();}
 for(unsigned sp=0;sp<65536;sp++){
  good(sp);a.seq.cpu.sp=(uint16_t)sp;before=a;clone();before=b;unsigned n=wf_z80_control_fast_step(&b.seq.cpu,15,(uint32_t)b.seq.cpu.cyc+124);
  if(sp>=0xc002&&sp<=0xc800){for(unsigned i=0;i<15;i++)wf_ref_z80_step(&a.seq.cpu);if(n!=15)fail("SP admission");equal();++exacts;}
  else{if(n||memcmp(&before,&b,sizeof b))fail("SP decline mutated");++guards;}
 }
 for(unsigned i=0;i<2000;i++)for(unsigned mode=0;mode<12;mode++){
  good(i);unsigned budget=15,span=124;
  if(mode==0)a.seq.cpu.nmi_pending=true;if(mode==1){a.seq.cpu.int_pending=true;a.seq.cpu.iff1=true;}if(mode==2)a.seq.cpu.iff_delay=1;if(mode==3)a.seq.cpu.halted=true;
  if(mode==4)a.seq.cpu.ix=0xc800;if(mode==5){a.seq.cpu.h=0xc8;a.seq.cpu.l=0;}if(mode==6)a.seq.status|=128;
  if(mode==7)budget=i%15;if(mode==8)span=i%124;if(mode==9)span=0xffffffffu;if(mode==10)span=0x80000000u;if(mode==11){a.seq.cpu.pc=0xe6b;}
  clone();before=b;unsigned n=wf_z80_control_fast_step(&b.seq.cpu,budget,(uint32_t)b.seq.cpu.cyc+span);if(n||memcmp(&before,&b,sizeof b))fail("guard decline mutated");++guards;
 }
 printf("PASS PRE0E6A: %llu full15-step original-R01 CPU/RAM/timer/emit-state+timestamp comparisons, %llu immutable guard declines, memory/stack aliases, callbackfalse,NMI/IRQ/delay mutations andwrap32\n",exacts,guards);return 0;
}
