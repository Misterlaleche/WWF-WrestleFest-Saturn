#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "wf_music_seq.h"
extern unsigned wf_z80_control_batch(z80 *,unsigned,uint32_t);
extern uint64_t wf_test_fast_calls,wf_test_fast_misses;
static int admitted(unsigned pc)
{
 switch(pc){case 0x037c:case 0x0f2d:case 0x08c1:case 0x0f4b:
 case 0x0a8f:case 0x0aa0:case 0x0991:case 0x0a0b:
 case 0x080d:case 0x08a7:return 1;default:return 0;}
}
int main(void)
{
 static wf_music_seq_t v,before;
 unsigned expected=0,cases=0;
 for(unsigned pc=0;pc<65536u;pc++)for(unsigned mode=0;mode<7;mode++){
  memset(&v,0,sizeof(v));v.cpu.userdata=&v;v.cpu.pc=(uint16_t)pc;
  unsigned budget=4096;uint32_t limit=100;
  switch(mode){case 0:budget=0;break;case 1:limit=0;break;
   case 2:v.cpu.nmi_pending=1;break;
   case 3:v.cpu.int_pending=1;v.cpu.iff1=1;break;
   case 4:v.cpu.iff_delay=1;break;case 5:v.cpu.halted=1;break;
   case 6:v.cpu.cyc=0xfffffff0u;limit=0xffffffefu;break;}
  memcpy(&before,&v,sizeof(v));assert(!wf_z80_control_batch(&v.cpu,budget,limit));
  assert(!memcmp(&v,&before,sizeof(v)));expected+=admitted(pc);cases++;
 }
 printf("admission cases=%u fast_calls=%llu expected=%u fast_misses=%llu\n",cases,(unsigned long long)wf_test_fast_calls,expected,(unsigned long long)wf_test_fast_misses);
 fflush(stdout);
 assert(wf_test_fast_calls==expected);assert(!wf_test_fast_misses);
 puts("PASS all65536PC x7 control declines: exact functional state and fast never entered for a miss");
 return 0;
}
