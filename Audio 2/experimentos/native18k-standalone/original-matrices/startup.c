/* Detects partial copies, incorrect CPU side effects, unsafe acceptance and
 * any RAM/timer mutation outside the exact pinned LDIR331 operation. */
#include "wf_music_seq.h"
#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
extern unsigned wf_z80_startup_copy(z80*,unsigned,uint32_t);
extern void wf_ref_z80_step(z80*);
static wf_music_seq_t a,b,saved;
static uint8_t rom[65536];
static uint32_t rng=0x5a17c09du;
static uint32_t random32(void){rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;return rng;}
static void normalize(z80*z){z->userdata=NULL;}
static void seed(unsigned n){
 assert(wf_music_seq_init(&a,rom,sizeof rom,NULL,NULL,true));
 for(unsigned i=0;i<sizeof a.ram;i++)a.ram[i]=(uint8_t)random32();
 a.cpu.cyc=n%3==0?0xfffffff0u:random32();a.cpu.pc=0x037c;
 a.cpu.sp=(uint16_t)random32();a.cpu.ix=(uint16_t)random32();a.cpu.iy=(uint16_t)random32();
 a.cpu.mem_ptr=(uint16_t)random32();a.cpu.a=(uint8_t)random32();
 a.cpu.b=1;a.cpu.c=0x4b;a.cpu.d=0xc0;a.cpu.e=0x57;a.cpu.h=0x10;a.cpu.l=0xf0;
 a.cpu.a_=(uint8_t)random32();a.cpu.b_=(uint8_t)random32();a.cpu.c_=(uint8_t)random32();
 a.cpu.d_=(uint8_t)random32();a.cpu.e_=(uint8_t)random32();a.cpu.h_=(uint8_t)random32();
 a.cpu.l_=(uint8_t)random32();a.cpu.f_=(uint8_t)random32();
 a.cpu.i=(uint8_t)random32();a.cpu.r=(uint8_t)random32();
 a.cpu.sf=random32()&1;a.cpu.zf=random32()&1;a.cpu.yf=random32()&1;a.cpu.hf=random32()&1;
 a.cpu.xf=random32()&1;a.cpu.pf=random32()&1;a.cpu.nf=random32()&1;a.cpu.cf=random32()&1;
 a.cpu.iff1=random32()&1;a.cpu.iff2=random32()&1;a.cpu.int_pending=!a.cpu.iff1&&(random32()&1);
 a.cpu.interrupt_mode=(uint8_t)(random32()%3);a.cpu.int_data=(uint8_t)random32();
 a.next_tb=random32();a.last_cpu=random32();a.cycles=((uint64_t)random32()<<32)|random32();
 a.steps=random32();a.skipped=random32();a.ym_addr=(uint8_t)random32();a.status=(uint8_t)random32();
 a.tb=(uint8_t)random32();a.ctl=(uint8_t)random32();a.latch=(uint8_t)random32();a.bank=(uint8_t)random32();
 a.error=random32()&1;a.command_pending=random32()&1;
 b=a;b.cpu.userdata=&b;
}
static void equal(unsigned n){
 wf_music_seq_t x=a,y=b;normalize(&x.cpu);normalize(&y.cpu);
 if(memcmp(&x,&y,sizeof x)){fprintf(stderr,"FAIL differential case%u PC=%04x/%04x cyc=%lu/%lu WZ=%04x/%04x R=%02x/%02x\n",n,x.cpu.pc,y.cpu.pc,x.cpu.cyc,y.cpu.cyc,x.cpu.mem_ptr,y.cpu.mem_ptr,x.cpu.r,y.cpu.r);assert(0);}
}
static unsigned declines;
static void declined(unsigned budget,uint32_t limit){
 saved=b;unsigned taken=wf_z80_startup_copy(&b.cpu,budget,limit);
 assert(taken==0);assert(!memcmp(&saved,&b,sizeof b));++declines;
}
int main(int argc,char**argv){
 FILE*f=NULL;assert(argc==2&&!fopen_s(&f,argv[1],"rb"));assert(fread(rom,1,sizeof rom,f)==sizeof rom);fclose(f);
 assert(rom[0x037c]==0xed&&rom[0x037d]==0xb0);
 seed(0);unsigned taken=wf_z80_startup_copy(&b.cpu,331,(uint32_t)b.cpu.cyc+6946u);
 if(taken!=331){fprintf(stderr,"FAIL missing startup lift: expected331 original steps, got%u\n",taken);return 1;}
 for(unsigned i=0;i<331;i++)wf_ref_z80_step(&a.cpu);equal(0);
 for(unsigned n=1;n<10000;n++){
  seed(n);unsigned count=wf_z80_startup_copy(&b.cpu,331+(random32()&1023u),(uint32_t)b.cpu.cyc+6946u+(random32()&65535u));
  assert(count==331);for(unsigned i=0;i<331;i++)wf_ref_z80_step(&a.cpu);equal(n);
 }
 seed(0);for(unsigned budget=0;budget<331;budget++)declined(budget,(uint32_t)b.cpu.cyc+6946u);
 for(unsigned span=0;span<6946;span++)declined(331,(uint32_t)b.cpu.cyc+span);
 declined(331,(uint32_t)b.cpu.cyc-1u);declined(331,(uint32_t)b.cpu.cyc+0x80000000u);
 b.cpu.nmi_pending=1;declined(331,(uint32_t)b.cpu.cyc+6946u);b.cpu.nmi_pending=0;
 b.cpu.iff1=1;b.cpu.int_pending=1;declined(331,(uint32_t)b.cpu.cyc+6946u);b.cpu.int_pending=0;
 for(unsigned i=1;i<=255;i++){b.cpu.iff_delay=(uint8_t)i;declined(331,(uint32_t)b.cpu.cyc+6946u);}b.cpu.iff_delay=0;
 b.cpu.halted=1;declined(331,(uint32_t)b.cpu.cyc+6946u);b.cpu.halted=0;
 for(unsigned field=0;field<7;field++){
  seed(field);uint8_t*parts[]={&b.cpu.b,&b.cpu.c,&b.cpu.d,&b.cpu.e,&b.cpu.h,&b.cpu.l};
  if(field<6)*parts[field]^=1;else b.cpu.pc^=1;
  declined(331,(uint32_t)b.cpu.cyc+6946u);
 }
 printf("PASS startup copy:10000 full CPU/sequence/RAM differentials vs331 pinned z80_step each; %u immutable declines; 32-bit cycle wrap, budget0..330, span0..6945, NMI/IRQ/delay/halt/range guards\n",declines);return 0;
}
