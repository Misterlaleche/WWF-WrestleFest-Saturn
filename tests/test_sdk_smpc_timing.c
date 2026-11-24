/* Complete SDK source, with MMIO and CPU mask operations substituted.
 * The independent clock/beam model advances at each hardware observation. */
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "jo/types.h"
#include "jo/sega_saturn.h"
#include "jo/smpc.h"
#include "jo/input.h"
#define JO_ZERO(x) ((x)=0)
static struct {int width;} __jo_screen={320};
static unsigned char regs[128];
static unsigned mask,commands,polls,height,pal,cks,frcbase,latched_frc,latched_v;
static unsigned tvmd,exten,clock_hz,reads;
static unsigned test_fast_clock,test_display_on;
static uint64_t ns,line_ns,issue_ns,irq_delay_ns;
static unsigned irq_enabled,irq_running;
static void host_advance(void);
static unsigned actual_line(void){return (unsigned)((ns/line_ns)%(pal?313u:263u));}
static unsigned raw_line(void){
 unsigned line=actual_line();unsigned vsync=pal?(height==240?267u:259u):(height==240?245u:237u);
 return line+(line>=vsync?512u-(pal?313u:263u):0u);
}
static unsigned frc(void){return (unsigned)((ns*clock_hz/(1000000000ull*(8u<<(2u*cks)))+frcbase)&65535u);}
static unsigned host_mmio(unsigned a){
 host_advance();
 switch(a){
 case 0xfffffe12u:latched_frc=frc();return latched_frc>>8;
 case 0xfffffe13u:return latched_frc&255u;
 case 0xfffffe16u:return cks;
 case 0x25f80000u:return tvmd;
 case 0x25f80002u:if(!(exten&512u)){latched_v=raw_line();if((tvmd&0xc0u)==0xc0u)latched_v=(latched_v<<1)|1u;}return exten;
 case 0x25f8000au:return latched_v;
 default:assert(!"Unexpected MMIO");return 0;
 }
}
static unsigned char host_read(jo_smpc_memory_address a){
 assert(a!=InputRegister0);host_advance();return regs[(unsigned)a&127];
}
static void host_write(jo_smpc_memory_address a,unsigned char v){
 host_advance();regs[(unsigned)a&127]=v;
 if(a==CommandRegister){
  unsigned lines=pal?313u:263u;uint64_t phase=ns%(line_ns*lines);
  uint64_t since=phase>=height*line_ns?phase-height*line_ns:phase+(lines-height)*line_ns;
  assert(since>=300000u);assert(mask==15);commands++;issue_ns=ns;
  if(v==FetchStatusAndPeripheralData && (regs[(unsigned)InputRegister1&127]&8)){
   assert(phase>=height*line_ns && phase<(lines-1)*line_ns);
   assert((lines-1)*line_ns-phase>=100000u); // peripheral-only: final sample-to-COMREG reserve
  }
  regs[(unsigned)StatusFlag&127]=0;
 }
}
static void host_poll(void){assert(++polls<1000000);host_advance();}
static void host_begin(void){while(host_read(StatusFlag)&1)host_poll();host_write(StatusFlag,1);}
static void host_end(jo_smpc_command c){host_write(CommandRegister,c);while(host_read(StatusFlag)&1)host_poll();}
static void __jo_core_restore_scu_mask(void){}
static unsigned __jo_core_disable_interrupts(void){unsigned old=mask;ns+=irq_delay_ns;irq_delay_ns=0;mask=15;return old;}
static void __jo_core_restore_interrupts(unsigned sr){mask=sr;host_advance();}
static void __jo_core_set_interrupt_handler(unsigned v,void(*h)(void)){(void)v;(void)h;}
void __jo_vdp2_set_external_latch(bool e){(void)e;}
#define jo_smpc_read_byte host_read
#define jo_smpc_write_byte host_write
#define jo_smpc_begin_command host_begin
#define jo_smpc_end_command host_end
#include "sdk_smpc_timing.inc"
static void host_advance(void){
 assert(++reads<3000000);uint64_t old=ns;ns+=100;
 unsigned n=pal?313u:263u;
 if(irq_enabled&&!irq_running&&mask==0 && (old/line_ns!=ns/line_ns) && actual_line()==height){
  (void)n;irq_running=1;mask=15;__jo_smpc_vblank_mark();__jo_smpc_vblank_in();mask=0;irq_running=0;
 }
}
static void reset(unsigned p,unsigned h,unsigned c){
 memset(regs,0,sizeof regs);memset(__jo_smpc_peripherals,0,sizeof __jo_smpc_peripherals);
 pal=p;height=h;cks=c;frcbase=65520;
 clock_hz=test_fast_clock?(pal?28437500u:28636364u):(pal?26687500u:26875000u);
 line_ns=pal?64000u:63556u;tvmd=(h==240?0x10u:0u)|(test_display_on?0x8000u:0u);exten=0;ns=0;mask=0;
 reads=commands=polls=irq_enabled=irq_running=0;irq_delay_ns=0;__jo_smpc_timing_fault=0;
 __jo_smpc_intback_busy=__jo_smpc_command_pending=false;__jo_smpc_intback_length=0;
 __jo_smpc_gun_ports=__jo_smpc_gun_found=0;
 __jo_smpc_peripherals[0].data=0xffef;
}
static unsigned run_intback(unsigned p,unsigned h,unsigned c,uint64_t phase){
 reset(p,h,c);ns=phase;mask=15;__jo_smpc_vblank_mark();__jo_smpc_vblank_in();
 assert(__jo_smpc_peripherals[0].data==0xffef);assert(!__jo_smpc_timing_fault);
 assert(commands<=1);return commands;
}
int main(void){
 unsigned cases=0;
 for(test_fast_clock=0;test_fast_clock<2;test_fast_clock++)
 for(test_display_on=0;test_display_on<2;test_display_on++)
 for(unsigned p=0;p<2;p++)for(unsigned h=224;h<=240;h+=16)for(unsigned c=0;c<3;c++){
  reset(p,h,c);uint64_t line=line_ns;
  assert(run_intback(p,h,c,h*line)==1);cases++;
  for(unsigned delay=0;delay<900;delay+=7){run_intback(p,h,c,h*line+delay*1000u);cases++;}
  assert(run_intback(p,h,c,100*line)==0);cases++;
  assert(run_intback(p,h,c,((p?313u:263u)-1)*line-100000u)==0);cases++;
  for(unsigned scan=0;scan<(p?313u:263u);scan+=3){
   reset(p,h,c);ns=scan*line+line/2;irq_enabled=1;
   __jo_smpc_command(DisableReset);assert(commands==1&&mask==0&&!__jo_smpc_command_pending);cases++;
  }
  reset(p,h,c);ns=192*line;irq_delay_ns=100000000u;irq_enabled=1;
  __jo_smpc_command(EnableReset);assert(commands==1&&mask==0);cases++;
  reset(p,h,c);ns=(h-1)*line;irq_enabled=1;__jo_smpc_intback_busy=true;
  __jo_smpc_intback_length=64;__jo_smpc_command(DisableReset);
  assert(commands==1&&mask==0&&!__jo_smpc_intback_busy&&!__jo_smpc_intback_length);cases++;
  reset(p,h,c);ns=h*line;mask=15;__jo_smpc_vblank_mark();
  __jo_smpc_intback_busy=true;__jo_smpc_intback_length=64;__jo_smpc_vblank_in();
  assert(!commands&&!__jo_smpc_intback_busy&&!__jo_smpc_intback_length);cases++;
  reset(p,h,c);ns=h*line;mask=15;__jo_smpc_vblank_mark();regs[(unsigned)StatusFlag&127]=1;
  __jo_smpc_vblank_in();assert(!commands&&__jo_smpc_peripherals[0].data==0xffef);cases++;
  reset(p,h,c);ns=10*line;mask=15;__jo_smpc_init();assert(commands==1&&mask==15);cases++;
  reset(p,h,c);ns=h*line;mask=15;__jo_smpc_vblank_mark();cks=(c+1)%3;
  __jo_smpc_vblank_in();assert(!commands&&__jo_smpc_timing_fault==8);cases++;
 }
 unsigned inherited[]={1u,2u,3u,0x20u,0x80u,0xc0u,0xe3u};
 for(unsigned i=0;i<sizeof inherited/sizeof inherited[0];i++){
  reset(1,240,1);ns=100*line_ns;tvmd=inherited[i];__jo_smpc_command(DisableReset);
  assert(commands==1&&!__jo_smpc_timing_fault&&mask==0);cases++;
 }
 unsigned invalid[]={1u,2u,0x20u,0x30u,0x40u,0xc0u};
 for(unsigned i=0;i<sizeof invalid/sizeof invalid[0];i++){
  reset(0,240,1);ns=240*line_ns;tvmd=invalid[i];mask=15;__jo_smpc_vblank_mark();__jo_smpc_vblank_in();
  assert(!commands&&__jo_smpc_timing_fault==1&&mask==15);cases++;
 }
 reset(0,240,1);ns=100*line_ns;tvmd=4;__jo_smpc_command(DisableReset);
 assert(!commands&&__jo_smpc_timing_fault==1&&mask==0);cases++;
 reset(0,240,1);ns=100*line_ns;exten=512;__jo_smpc_command(DisableReset);
 assert(!commands&&__jo_smpc_timing_fault==2&&mask==0);cases++;
 reset(0,240,3);ns=240*line_ns;mask=15;__jo_smpc_vblank_mark();__jo_smpc_vblank_in();
 assert(!commands&&__jo_smpc_timing_fault==4);cases++;
 reset(0,240,1);ns=240*line_ns;mask=15;__jo_smpc_vblank_mark();
 ns+=700000u; /* callbacks already consumed the wait budget, but beam is safe */
 __jo_smpc_vblank_in();assert(commands==1&&!__jo_smpc_timing_fault);cases++;
 reset(0,240,1);ns=240*line_ns;mask=15;__jo_smpc_vblank_mark();
 ns=262*line_ns-30000u; /* last blank line: no room for the final MMIO path */
 __jo_smpc_vblank_in();assert(!commands);cases++;
 printf("PASS %u complete-source timing cases; PAL/NTSC,224/240,/8/32/128,wrap,late IRQ,bootstrap,pad retention,invalid modes\n",cases);
}
