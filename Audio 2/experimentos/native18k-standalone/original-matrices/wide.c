#include "wf_music_seq.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
extern bool wf_ref_init(wf_music_seq_t*,const uint8_t*,size_t,wf_music_emit_fn,void*,bool);
extern int wf_ref_run(wf_music_seq_t*,uint64_t,unsigned);
static wf_music_seq_t a,b;static uint8_t rom[65536];
static void normalize(z80*z){z->userdata=NULL;z->read_byte=NULL;z->write_byte=NULL;z->port_in=NULL;z->port_out=NULL;}
int main(int argc,char**argv){
 FILE*f=NULL;assert(argc==2&&!fopen_s(&f,argv[1],"rb"));assert(fread(rom,1,sizeof rom,f)==sizeof rom);fclose(f);
 rom[0x4000]=0xc3;rom[0x4001]=0;rom[0x4002]=0x40;
 unsigned budgets[]={1u<<26,(1u<<26)-1u,(1u<<26)+1u};
 for(unsigned i=0;i<3;i++){
  assert(wf_ref_init(&a,rom,sizeof rom,NULL,NULL,false));assert(wf_music_seq_init(&b,rom,sizeof rom,NULL,NULL,false));b.optimized=true;a.cpu.pc=b.cpu.pc=0x4000;
  unsigned budget=budgets[i];uint64_t target=(uint64_t)budget*10u;
  int x=wf_ref_run(&a,target,budget),y=wf_music_seq_run(&b,target,budget);
  assert(x==0&&!a.error&&y==0&&!b.error);
  z80 ca=a.cpu,cb=b.cpu;normalize(&ca);normalize(&cb);assert(!memcmp(&ca,&cb,sizeof ca));assert(!memcmp(a.ram,b.ram,sizeof a.ram));
  assert(a.cycles==b.cycles&&a.last_cpu==b.last_cpu&&a.steps==b.steps&&a.next_tb==b.next_tb&&a.status==b.status&&a.tb==b.tb&&a.ctl==b.ctl);
  printf("GREEN elapsed overflow: budget=%u elapsed=%lu taken=%llu old_product=%u reference_rc=%d fixed_rc=%d fullCPU/RAM/cycles/timers/steps exact\n",budget,b.cpu.cyc,b.steps,64u*(unsigned)b.steps,x,y);
 }
 return 0;
}
