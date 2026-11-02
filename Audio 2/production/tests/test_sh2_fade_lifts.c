/* Exact original-ROM fade operations, compared with the untouched scalar CPU.
 * This checks semantics and admission boundaries, not Saturn throughput. */
#include "wf_music_seq.h"
#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern unsigned wf_z80_control_fast_step(z80 *,unsigned,uint32_t);
extern bool wf_z80_native_compatible(const uint8_t *,size_t);
extern void wf_ref_z80_init(z80 *);
extern void wf_ref_z80_step(z80 *);
static uint8_t rom[65536];
static wf_music_seq_t reference,candidate;
static uint64_t cases,clamps,unclamped;
static unsigned operation;

static uint8_t read_byte(void *user,uint16_t address)
{
 wf_music_seq_t *v=user;
 if(address<0xc000)return v->rom[address];
 if(address<0xc800)return v->ram[address-0xc000];
 fprintf(stderr,"Unexpected MMIO read %04x\n",address);exit(6);
}
static void write_byte(void *user,uint16_t address,uint8_t value)
{
 wf_music_seq_t *v=user;
 if(address>=0xc000&&address<0xc800){v->ram[address-0xc000]=value;return;}
 fprintf(stderr,"Unexpected non-RAM write %04x\n",address);exit(6);
}
static void normalize(z80 *z)
{z->read_byte=0;z->write_byte=0;z->port_in=0;z->port_out=0;z->userdata=0;}
static void setup(unsigned a,unsigned value,unsigned selector,unsigned channel,unsigned seed)
{
 memset(&reference,0,sizeof reference);reference.rom=rom;
 operation=0x0be7;
 for(unsigned i=0;i<sizeof reference.ram;i++)reference.ram[i]=(uint8_t)(i*37u+seed);
 z80 *z=&reference.cpu;wf_ref_z80_init(z);
 z->read_byte=read_byte;z->write_byte=write_byte;z->userdata=&reference;
 z->pc=0x0be7;z->sp=(uint16_t)(0xc402u+2u*(seed%511u));
 z->cyc=(seed&1)?0xfffffff0ul:0x123456ul;
 z->mem_ptr=(uint16_t)seed;z->ix=(uint16_t)(seed*13u);z->iy=(uint16_t)(seed*17u);
 z->a=(uint8_t)a;z->b=(uint8_t)seed;z->c=(uint8_t)(seed>>8);
 z->d=(uint8_t)(seed>>16);z->e=(uint8_t)(seed>>24);z->h=(uint8_t)(seed*19u);z->l=(uint8_t)(seed*23u);
 z->a_=seed^0x39;z->b_=seed^0x91;z->c_=seed^0x27;z->d_=seed^0xb1;
 z->e_=seed^0x41;z->h_=seed^0x35;z->l_=seed^0x1d;z->f_=seed^0xa5;
 z->r=(uint8_t)seed;z->i=(uint8_t)(seed>>8);z->interrupt_mode=seed%3;
 z->sf=(seed>>7)&1;z->zf=(seed>>6)&1;z->yf=(seed>>5)&1;z->hf=(seed>>4)&1;
 z->xf=(seed>>3)&1;z->pf=(seed>>2)&1;z->nf=(seed>>1)&1;z->cf=seed&1;
 z->iff1=z->iff2=(seed>>9)&1;
 reference.ram[0x65]=(uint8_t)channel;reference.ram[0x07]=(uint8_t)selector;
 reference.ram[0x8e + channel]=(uint8_t)value;
 reference.ram[z->sp-0xc000]=0x00;reference.ram[z->sp-0xbfff]=0x50;
 candidate=reference;candidate.cpu.userdata=&candidate;
}
static unsigned original(void)
{
 unsigned steps=0;
 do{wf_ref_z80_step(&reference.cpu);assert(++steps<=80);}while(reference.cpu.pc!=0x5000);
 if(operation==0x0be7)assert(steps==20||steps==21);
 return steps;
}
static void compare(unsigned expected,unsigned took)
{
 if(took!=expected){fprintf(stderr,"RED fade operation %04X expected %u original instructions, admitted %u\n",operation,expected,took);exit(2);}
 z80 l=reference.cpu,r=candidate.cpu;normalize(&l);normalize(&r);
 if(memcmp(&l,&r,sizeof l)||memcmp(reference.ram,candidate.ram,sizeof reference.ram)){
  fprintf(stderr,"FAIL fade CPU/flags/stack/cycles/R at case %"PRIu64"; PC=%04x/%04x cyc=%lu/%lu\n",cases,l.pc,r.pc,l.cyc,r.cyc);
  const unsigned char *lp=(const unsigned char *)&l,*rp=(const unsigned char *)&r;
  for(unsigned i=0;i<sizeof l;i++)if(lp[i]!=rp[i])fprintf(stderr,"CPU byte %u: %02x/%02x\n",i,lp[i],rp[i]);
  exit(3);
 }
 ++cases;if(operation==0x0be7){if(expected==21)++clamps;else ++unclamped;}
}
static void one(unsigned a,unsigned value,unsigned selector,unsigned channel,unsigned seed)
{
 setup(a,value,selector,channel,seed);unsigned steps=original();
 compare(steps,wf_z80_control_fast_step(&candidate.cpu,steps,(uint32_t)reference.cpu.cyc));
}
static void rejects(void)
{
 /* Every rejected operation must leave all CPU/RAM/adapter bytes untouched. */
 for(unsigned kind=0;kind<13;kind++){
  setup(31,47,11,3,0x6352);unsigned steps=original();
  unsigned budget=steps;uint32_t limit=(uint32_t)reference.cpu.cyc;
  switch(kind){
   case 0:budget=0;break;
   case 1:budget=steps-1;break;
   case 2:--limit;break;
   case 3:limit=(uint32_t)candidate.cpu.cyc;break;
   case 4:candidate.cpu.nmi_pending=1;break;
   case 5:candidate.cpu.int_pending=1;candidate.cpu.iff1=1;break;
   case 6:candidate.cpu.iff_delay=1;break;
   case 7:candidate.cpu.halted=1;break;
   case 8:candidate.cpu.sp=0xc400;break;
   case 9:candidate.cpu.sp=0xc7ff;break;
   case 10:candidate.cpu.sp=0x0100;break;
   case 11:candidate.cpu.pc=0x0be8;break;
   case 12:limit=(uint32_t)candidate.cpu.cyc-1;break;
  }
  wf_music_seq_t before=candidate;
  assert(wf_z80_control_fast_step(&candidate.cpu,budget,limit)==0);
  assert(!memcmp(&before,&candidate,sizeof before));++cases;
 }
 /* A masked interrupt must not suppress a semantically valid operation. */
 setup(244,118,255,255,0xa581);reference.cpu.int_pending=candidate.cpu.int_pending=1;
 reference.cpu.iff1=candidate.cpu.iff1=0;unsigned steps=original();
 compare(steps,wf_z80_control_fast_step(&candidate.cpu,steps,(uint32_t)reference.cpu.cyc));
}
static void resume_setup(unsigned remaining,unsigned bc,unsigned e,unsigned hl,unsigned seed)
{
 setup(remaining,0,0,0,seed);operation=0x0f3a;
 z80 *z=&reference.cpu;z->pc=operation;z->sp=(uint16_t)(0xc402+2*(seed%509));
 z->b=(uint8_t)(bc>>8);z->c=(uint8_t)bc;z->e=(uint8_t)e;
 z->h=(uint8_t)(hl>>8);z->l=(uint8_t)hl;
 unsigned at=z->sp-0xc000;
 reference.ram[at]=(uint8_t)(seed>>8);reference.ram[at+1]=(uint8_t)seed;
 reference.ram[at+2]=(uint8_t)(seed^0xa5);reference.ram[at+3]=(uint8_t)(seed^0x5a);
 reference.ram[at+4]=0;reference.ram[at+5]=0x50;
 candidate=reference;candidate.cpu.userdata=&candidate;
}
static void multiply_resume(void)
{
 for(unsigned n=1;n<=8;n++)for(unsigned bc=0;bc<65536;bc++){
  resume_setup(n,bc,bc^(bc>>8),(bc*49157u)^0x5239,bc);
  unsigned steps=original();compare(steps,wf_z80_control_fast_step(&candidate.cpu,steps,(uint32_t)reference.cpu.cyc));
 }
 for(unsigned n=1;n<=8;n++)for(unsigned e=0;e<256;e++)for(unsigned lo=0;lo<256;lo++){
  resume_setup(n,(lo<<8)|e,e,(e<<8)|lo,0xb7420000u|(e<<8)|lo);
  unsigned steps=original();compare(steps,wf_z80_control_fast_step(&candidate.cpu,steps,(uint32_t)reference.cpu.cyc));
 }
 for(unsigned kind=0;kind<12;kind++){
  resume_setup(8,0xffff,0xff,0xffff,1234);unsigned steps=original();
  unsigned budget=steps;uint32_t limit=(uint32_t)reference.cpu.cyc;
  switch(kind){
   case 0:budget=steps-1;break;
   case 1:--limit;break;
   case 2:candidate.cpu.a=0;break;
   case 3:candidate.cpu.a=9;break;
   case 4:candidate.cpu.sp=0xbfff;break;
   case 5:candidate.cpu.sp=0xc7fb;break;
   case 6:candidate.cpu.nmi_pending=1;break;
   case 7:candidate.cpu.int_pending=1;candidate.cpu.iff1=1;break;
   case 8:candidate.cpu.iff_delay=1;break;
   case 9:candidate.cpu.halted=1;break;
   case 10:budget=0;break;
   case 11:limit=(uint32_t)candidate.cpu.cyc;break;
  }
  wf_music_seq_t before=candidate;
  assert(!wf_z80_control_fast_step(&candidate.cpu,budget,limit));
  assert(!memcmp(&before,&candidate,sizeof before));++cases;
 }
}
static void scale_setup(unsigned a,unsigned factor,unsigned seed)
{
 setup(a,0,0,0,seed);operation=0x0bd7;
 reference.cpu.pc=operation;reference.cpu.sp=(uint16_t)(0xc406+2*(seed%509));
 unsigned at=reference.cpu.sp-0xc000;
 reference.ram[at]=0;reference.ram[at+1]=0x50;reference.ram[0x63]=(uint8_t)factor;
 candidate=reference;candidate.cpu.userdata=&candidate;
}
static void fade_scale(void)
{
 for(unsigned a=0;a<256;a++)for(unsigned f=0;f<256;f++)for(unsigned pattern=0;pattern<2;pattern++){
  scale_setup(a,f,((a<<8)|f)^(pattern?0xffffffffu:0));
  unsigned steps=original();compare(steps,wf_z80_control_fast_step(&candidate.cpu,steps,(uint32_t)reference.cpu.cyc));
 }
 for(unsigned kind=0;kind<12;kind++){
  scale_setup(255,255,0x7451);unsigned steps=original();
  unsigned budget=steps;uint32_t limit=(uint32_t)reference.cpu.cyc;
  switch(kind){
   case 0:budget=steps-1;break;
   case 1:--limit;break;
   case 2:candidate.cpu.sp=0xc405;break;
   case 3:candidate.cpu.sp=0xc7ff;break;
   case 4:candidate.cpu.sp=0;break;
   case 5:candidate.cpu.pc=0x0bd8;break;
   case 6:candidate.cpu.nmi_pending=1;break;
   case 7:candidate.cpu.int_pending=1;candidate.cpu.iff1=1;break;
   case 8:candidate.cpu.iff_delay=1;break;
   case 9:candidate.cpu.halted=1;break;
   case 10:budget=0;break;
   case 11:limit=(uint32_t)candidate.cpu.cyc;break;
  }
  wf_music_seq_t before=candidate;
  assert(!wf_z80_control_fast_step(&candidate.cpu,budget,limit));
  assert(!memcmp(&before,&candidate,sizeof before));++cases;
 }
}
static void prepare_setup(unsigned channel,unsigned instrument,unsigned seed)
{
 setup(seed,0,0,channel,seed);operation=0x0b68;reference.cpu.pc=operation;
 reference.ram[0x82+channel]=(uint8_t)instrument;
 candidate=reference;candidate.cpu.userdata=&candidate;
}
static void level_prepare(void)
{
 for(unsigned channel=0;channel<8;channel++)for(unsigned instrument=0;instrument<256;instrument++){
  unsigned table=0x135f+((instrument*2u)&255u);
  unsigned base=rom[table]|((unsigned)rom[table+1]<<8);
  for(unsigned pattern=0;pattern<32;pattern++){
   prepare_setup(channel,instrument,(instrument<<16)|(channel<<8)|(pattern*7u));
   if(base>0xbfe7){
    wf_music_seq_t before=candidate;
    assert(!wf_z80_control_fast_step(&candidate.cpu,27,(uint32_t)candidate.cpu.cyc+301));
    assert(!memcmp(&before,&candidate,sizeof before));++cases;continue;
   }
   for(unsigned step=0;step<27;step++)wf_ref_z80_step(&reference.cpu);
   assert(reference.cpu.pc==0x0b98);
   compare(27,wf_z80_control_fast_step(&candidate.cpu,27,(uint32_t)reference.cpu.cyc));
  }
 }
 unsigned valid=0;
 while(valid<256){unsigned at=0x135f+((valid*2u)&255u);if((rom[at]|((unsigned)rom[at+1]<<8))<=0xbfe7)break;++valid;}
 assert(valid<256);
 for(unsigned kind=0;kind<10;kind++){
  prepare_setup(0,valid,0x4352);unsigned budget=27;uint32_t limit=(uint32_t)candidate.cpu.cyc+301;
  switch(kind){
   case 0:budget=26;break;
   case 1:--limit;break;
   case 2:candidate.ram[0x65]=8;break;
   case 3:candidate.ram[0x65]=255;break;
   case 4:candidate.cpu.pc=0x0b69;break;
   case 5:candidate.cpu.nmi_pending=1;break;
   case 6:candidate.cpu.int_pending=1;candidate.cpu.iff1=1;break;
   case 7:candidate.cpu.iff_delay=1;break;
   case 8:candidate.cpu.halted=1;break;
   case 9:budget=0;break;
  }
  wf_music_seq_t before=candidate;
  assert(!wf_z80_control_fast_step(&candidate.cpu,budget,limit));
  assert(!memcmp(&before,&candidate,sizeof before));++cases;
 }
}
static void ym_return_setup(unsigned main_flags,unsigned alternate_flags,unsigned back,unsigned seed)
{
 setup(seed,0,0,0,seed);operation=0x0f5f;reference.cpu.pc=operation;
 z80 *z=&reference.cpu;z->sp=(uint16_t)(0xc000u+seed%2047u);
 z->sf=(main_flags>>7)&1;z->zf=(main_flags>>6)&1;z->yf=(main_flags>>5)&1;z->hf=(main_flags>>4)&1;
 z->xf=(main_flags>>3)&1;z->pf=(main_flags>>2)&1;z->nf=(main_flags>>1)&1;z->cf=main_flags&1;
 z->f_=(uint8_t)alternate_flags;
 reference.ram[z->sp-0xc000]=(uint8_t)back;reference.ram[z->sp-0xbfff]=(uint8_t)(back>>8);
 candidate=reference;candidate.cpu.userdata=&candidate;
}
static void ym_return(void)
{
 for(unsigned f=0;f<256;f++)for(unsigned alt=0;alt<256;alt++){
  ym_return_setup(f,alt,(f<<8)|alt,(f<<8)|alt);
  wf_ref_z80_step(&reference.cpu);wf_ref_z80_step(&reference.cpu);
  compare(2,wf_z80_control_fast_step(&candidate.cpu,2,(uint32_t)reference.cpu.cyc));
 }
 /* The instrument continuation may still take its existing five-step lift,
  * but exactly two instructions must be admissible at its shorter boundary. */
 for(unsigned b=0;b<256;b++){
  ym_return_setup(b,b^255u,0x0e71,0xffff0000u|b);
  reference.cpu.b=candidate.cpu.b=(uint8_t)b;
  wf_ref_z80_step(&reference.cpu);wf_ref_z80_step(&reference.cpu);
  compare(2,wf_z80_control_fast_step(&candidate.cpu,5,(uint32_t)reference.cpu.cyc));
 }
 for(unsigned kind=0;kind<11;kind++){
  ym_return_setup(0xa5,0x5a,0x5000,3);unsigned budget=2;uint32_t limit=(uint32_t)candidate.cpu.cyc+14;
  switch(kind){
   case 0:budget=1;break;
   case 1:--limit;break;
   case 2:candidate.cpu.sp=0xbfff;break;
   case 3:candidate.cpu.sp=0xc7ff;break;
   case 4:candidate.cpu.pc=0x0f60;break;
   case 5:candidate.cpu.nmi_pending=1;break;
   case 6:candidate.cpu.int_pending=1;candidate.cpu.iff1=1;break;
   case 7:candidate.cpu.iff_delay=1;break;
   case 8:candidate.cpu.halted=1;break;
   case 9:budget=0;break;
   case 10:limit=(uint32_t)candidate.cpu.cyc;break;
  }
  wf_music_seq_t before=candidate;
  assert(!wf_z80_control_fast_step(&candidate.cpu,budget,limit));
  assert(!memcmp(&before,&candidate,sizeof before));++cases;
 }
}
int main(int argc,char **argv)
{
 if(argc!=2&&argc!=3)return 64;
 unsigned selected=argc==3?(unsigned)strtoul(argv[2],NULL,0):0;
 if(selected&&selected!=0x0be7&&selected!=0x0f3a&&selected!=0x0bd7&&selected!=0x0b68&&selected!=0x0f5f)return 64;
 FILE *f=fopen(argv[1],"rb");assert(f);assert(fread(rom,1,sizeof rom,f)==sizeof rom);assert(fgetc(f)==EOF);fclose(f);
 assert(wf_z80_native_compatible(rom,sizeof rom));
 rom[0x0be7]^=1;assert(!wf_z80_native_compatible(rom,sizeof rom));rom[0x0be7]^=1;
 if(!selected||selected==0x0be7){
  for(unsigned a=0;a<256;a++)for(unsigned v=0;v<256;v++){
   one(a,v,(a^v),v,(a<<8)|v);
   one(a,v,(a+v)&255u,a,~((a<<8)|v));
  }
  /* Cover every original lookup byte with all channel indices. */
  for(unsigned selector=0;selector<256;selector++)for(unsigned ch=0;ch<256;ch++)
   one(ch,selector,selector,ch,0x12340000u|(selector<<8)|ch);
  rejects();assert(clamps&&unclamped);
 }
 if(!selected||selected==0x0f3a)multiply_resume();
 if(!selected||selected==0x0bd7)fade_scale();
 if(!selected||selected==0x0b68)level_prepare();
 if(!selected||selected==0x0f5f)ym_return();
 printf("PASS fade lift exact original CPU/RAM/cycles/R cases=%"PRIu64" clamp=%"PRIu64" unclamped=%"PRIu64"\n",cases,clamps,unclamped);
 return 0;
}
