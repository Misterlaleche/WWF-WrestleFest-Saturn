#include "wf_music_seq.h"
#include "wf_scsp_ym.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
static uint8_t rom[65536];static wf_music_seq_t vm;static wf_scsp_ym_t ym;
static unsigned pending=0xffffffffu,mask,bank,channels,drums;static uint64_t events;
static bool put(uint32_t a,uint16_t v,void*u){(void)v;(void)u;assert(a>=0x100000&&a<0x100400&&!(a&1));return true;}
static bool event(unsigned k,unsigned r,unsigned x,uint64_t t,void*u){
 (void)t;(void)u;++events;
 if(k==WF_MUSIC_YM){assert(wf_scsp_ym_write(&ym,(uint8_t)r,(uint8_t)x));if(r==8&&x&0x78)channels|=1u<<(x&7);}
 else if(k==WF_MUSIC_OKI){if(pending!=0xffffffffu){mask|=x>>4;assert((x>>4)<=1);pending=0xffffffffu;++drums;}else if(x&128)pending=x&127;}
 else if(k==WF_MUSIC_BANK)bank|=1u<<x;
 return true;
}
int main(int argc,char**argv){
 FILE*f=NULL;assert(argc==2&&fopen_s(&f,argv[1],"rb")==0);assert(fread(rom,1,65536,f)==65536);fclose(f);
 for(unsigned c=1;c<=18;c++){
  pending=0xffffffffu;mask=bank=channels=drums=0;events=0;
  wf_scsp_ym_init(&ym,put,NULL);assert(wf_music_seq_init(&vm,rom,sizeof rom,event,NULL,true));
  assert(!wf_music_seq_run(&vm,2ull*3579545,20000000));assert(wf_music_seq_send(&vm,(uint8_t)c));
  uint64_t origin=vm.cycles;
  for(unsigned seconds=1;seconds<=600;seconds++)assert(!wf_music_seq_run(&vm,origin+(uint64_t)seconds*3579545,20000000));
  assert(wf_music_seq_send(&vm,0x1f));assert(!wf_music_seq_run(&vm,vm.cycles+8ull*3579545,20000000));
  assert(!ym.fault&&!ym.range_clamps);
  printf("%02x: 600s+fade events=%llu channels=%02x oki_mask=%x drums=%u bank=%x steps=%llu skipped=%llu\n",c,(unsigned long long)events,channels,mask,drums,bank,(unsigned long long)vm.steps,(unsigned long long)vm.skipped);fflush(stdout);
 }
 /* Cross the32-bit Z80 cycle counter in bounded calls. */
 for(unsigned i=0;i<1200;i++)assert(!wf_music_seq_run(&vm,vm.cycles+3579545u,20000000));
 assert(vm.cycles>0x100000000ull&&!vm.error);
 puts("PASS: all18 continuous scores, native register conversion, drums, fades,32-bit wrap; no OPM synthesis");return 0;
}
