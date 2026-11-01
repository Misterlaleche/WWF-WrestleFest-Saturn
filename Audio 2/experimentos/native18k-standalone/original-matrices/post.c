#include "wf_music_seq.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
extern unsigned wf_test_post(z80*,unsigned,uint32_t);
void reference_z80_init(z80*);
void reference_z80_step(z80*);
static uint8_t rom[65536];static wf_music_seq_t a,b;static unsigned rng=0x5197acbd,checks,declines,partial;
static unsigned random32(void){rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;return rng;}
static unsigned ones(unsigned v){unsigned n=0;for(unsigned i=0;i<8;i++)n+=(v>>i)&1;return n;}
static uint8_t readbyte(void*u,uint16_t p){wf_music_seq_t*v=u;if(p<0xc000)return rom[p];if(p<0xc800)return v->ram[p-0xc000];++v->io_epoch;return 255;}
static void writebyte(void*u,uint16_t p,uint8_t x){wf_music_seq_t*v=u;if(p>=0xc000&&p<0xc800)v->ram[p-0xc000]=x;else if(p>=0xc800)++v->io_epoch;}
static void clone(void){b=a;b.cpu.userdata=&b;}
static void equal(void){z80 x=a.cpu,y=b.cpu;x.userdata=y.userdata=NULL;assert(!memcmp(&x,&y,sizeof x));assert(!memcmp(a.ram,b.ram,sizeof a.ram));assert(a.io_epoch==b.io_epoch);}
static void immutable(wf_music_seq_t*old){assert(!memcmp(old,&b,sizeof b));++declines;}
static void setup(unsigned v,unsigned sp){memset(&a,0,sizeof a);reference_z80_init(&a.cpu);a.rom=rom;a.cpu.userdata=&a;a.cpu.read_byte=readbyte;a.cpu.write_byte=writebyte;z80*z=&a.cpu;
 for(unsigned i=0;i<2048;i++)a.ram[i]=(uint8_t)random32();
 z->a=v>>8;z->b=random32();z->c=random32();z->d=random32();z->e=v;z->h=random32();z->l=random32();z->a_=v;z->b_=random32();z->c_=random32();z->d_=random32();z->e_=random32();z->h_=random32();z->l_=random32();z->f_=random32();z->i=random32();z->r=random32();z->ix=random32();z->iy=random32();z->mem_ptr=random32();z->sf=random32()&1;z->zf=random32()&1;z->yf=random32()&1;z->hf=random32()&1;z->xf=random32()&1;z->pf=random32()&1;z->nf=random32()&1;z->cf=random32()&1;z->iff1=random32()&1;z->iff2=random32()&1;z->int_pending=!z->iff1&&(random32()&1);z->interrupt_mode=random32()%3;z->int_data=random32();z->cyc=random32();z->pc=0xf5f;z->sp=(uint16_t)sp;if(sp>=0xc000&&sp<=0xc7fe){a.ram[sp-0xc000]=0x71;a.ram[sp-0xbfff]=0x0e;}clone();}
static void exact(void){unsigned n=5,t=a.cpu.b==1?38:43;uint32_t before=a.cpu.cyc;unsigned refresh=a.cpu.r;
 for(unsigned j=0;j<n;j++)reference_z80_step(&a.cpu);
 unsigned got=wf_test_post(&b.cpu,n,before+t);
 if(got!=n){printf("ymtail expected%u got%u\n",n,got);fflush(stdout);}assert(got==n);equal();assert((uint32_t)(a.cpu.cyc-before)==t);assert(a.cpu.r==((refresh&128)|((refresh+6)&127)));++checks;}
static void partial_compare(unsigned budget,unsigned ticks){wf_music_seq_t saved=b;uint32_t start=a.cpu.cyc;unsigned donea=0,doneb=0;
 while(donea<budget&&(uint32_t)(a.cpu.cyc-start)<ticks){reference_z80_step(&a.cpu);++donea;}
 while(doneb<budget&&(uint32_t)(b.cpu.cyc-start)<ticks){unsigned n=wf_test_post(&b.cpu,budget-doneb,start+ticks);if(!n){reference_z80_step(&b.cpu);n=1;}doneb+=n;}
 equal();assert(donea==doneb);if(!budget||!ticks)assert(!memcmp(&saved,&b,sizeof b));++partial;}
int main(int argc,char**argv){FILE*f=NULL;assert(argc==2&&!fopen_s(&f,argv[1],"rb"));assert(fread(rom,1,65536,f)==65536);fclose(f);
 /* Red/green anchor: result, timing, flags, refresh, stack and WZ, not result alone. */
 setup(0x7fff,0xc400);exact();
 for(unsigned v=0;v<65536;v++){setup(v,0xc000+(random32()%2047));exact();}
 for(unsigned sp=0;sp<65536;sp++){setup(random32(),sp);if(sp>=0xc000&&sp<=0xc7fe)exact();else{wf_music_seq_t old=b;assert(!wf_test_post(&b.cpu,100, (uint32_t)b.cpu.cyc+1000));immutable(&old);}}
 for(unsigned r=0;r<256;r++){setup(random32(),0xc7fe);a.cpu.r=(uint8_t)r;a.cpu.cyc=0xffffffffu-r;clone();exact();}
 for(unsigned af=0;af<65536;af++){setup(random32(),0xc400);z80*z=&a.cpu;z->a=af>>8;z->sf=(af>>7)&1;z->zf=(af>>6)&1;z->yf=(af>>5)&1;z->hf=(af>>4)&1;z->xf=(af>>3)&1;z->pf=(af>>2)&1;z->nf=(af>>1)&1;z->cf=af&1;clone();exact();}
 for(unsigned flags=0;flags<65536;flags++){setup(random32(),0xc400);z80*z=&a.cpu;z->f_=flags>>8;z->sf=(flags>>7)&1;z->zf=(flags>>6)&1;z->yf=(flags>>5)&1;z->hf=(flags>>4)&1;z->xf=(flags>>3)&1;z->pf=(flags>>2)&1;z->nf=(flags>>1)&1;z->cf=flags&1;clone();exact();}
 for(unsigned bval=0;bval<256;bval++)for(unsigned i=0;i<16;i++){setup(random32(),0xc400);a.cpu.b=(uint8_t)bval;clone();exact();}
 for(unsigned i=0;i<1000;i++){setup(random32(),0xc400);a.cpu.b=1;clone();unsigned n=wf_test_post(&b.cpu,5,(uint32_t)b.cpu.cyc+38);for(unsigned j=0;j<5;j++)reference_z80_step(&a.cpu);assert(n==5);equal();++checks;}
 /* Exhaustive lower budget/limit combinations for every E and wrap origins. */
 for(unsigned e=0;e<256;e++){
  unsigned n=5,t=43;
  for(unsigned budget=0;budget<=n+1;budget++){
   setup((random32()&0xff00)|e,0xc400);a.cpu.b=2;a.cpu.cyc=0xfffffff0u;clone();wf_music_seq_t old=b;unsigned got=wf_test_post(&b.cpu,budget,(uint32_t)b.cpu.cyc+t);if(budget<n){assert(!got);immutable(&old);}else{for(unsigned j=0;j<n;j++)reference_z80_step(&a.cpu);assert(got==n);equal();++checks;}
  }
  for(unsigned ticks=0;ticks<=t+1;ticks++){
   setup((random32()&0xff00)|e,0xc400);a.cpu.b=2;a.cpu.cyc=0xfffffff0u;clone();wf_music_seq_t old=b;unsigned got=wf_test_post(&b.cpu,n,(uint32_t)b.cpu.cyc+ticks);if(ticks<t){assert(!got);immutable(&old);}else{for(unsigned j=0;j<n;j++)reference_z80_step(&a.cpu);assert(got==n);equal();++checks;}
  }
  for(unsigned j=0;j<8;j++){setup((random32()&0xff00)|e,0xc400);a.cpu.b=2;a.cpu.cyc=0xfffffff0u;clone();partial_compare(j==0?0:j==1?n-1:j==2?n:n+1,j<4?t:j==4?0:j==5?t-1:j==6?t:t+1);}
 }
 for(unsigned j=0;j<10000;j++){
  setup(random32(),0xc400);unsigned mode=j%7;if(mode==0)b.cpu.nmi_pending=1;else if(mode==1){b.cpu.int_pending=1;b.cpu.iff1=1;}else if(mode==2)b.cpu.iff_delay=1;else if(mode==3)b.cpu.iff_delay=2;else if(mode==4)b.cpu.halted=1;else if(mode==5)b.cpu.pc=0xf60;else b.cpu.pc=random32();wf_music_seq_t old=b;assert(!wf_test_post(&b.cpu,100,(uint32_t)b.cpu.cyc+1000));immutable(&old);
 }
 for(unsigned ret=0;ret<65536;ret++){if(ret==0xe71)continue;setup(random32(),0xc400);a.ram[0x400]=(uint8_t)ret;a.ram[0x401]=(uint8_t)(ret>>8);clone();wf_music_seq_t old=b;assert(!wf_test_post(&b.cpu,5,(uint32_t)b.cpu.cyc+1000));immutable(&old);}
 const uint32_t spans[]={0,0xffffffffu,0xfffffe00u,0x80000000u,0x7fffffffu,0xfffffff0u};
 for(unsigned j=0;j<1000;j++)for(unsigned k=0;k<sizeof spans/sizeof spans[0];k++){
  setup(random32(),0xc400);a.cpu.cyc=j&1?0xfffffff0u:0;clone();wf_music_seq_t old=b;unsigned n=wf_test_post(&b.cpu,0xffffffffu,(uint32_t)b.cpu.cyc+spans[k]);if((int32_t)spans[k]<0||!spans[k]){assert(!n);immutable(&old);}else{unsigned steps=5;for(unsigned q=0;q<steps;q++)reference_z80_step(&a.cpu);assert(n==steps);equal();++checks;}
 }
 printf("PASS POST EX_AF+RET(E71)+INCIX+INCHL+DJNZ: %u exact full CPU/RAM/R/WZ/AF/timing cases, %u immutable guard declines, %u split budget/target comparisons; original pinned oracle\n",checks,declines,partial);return 0;}
