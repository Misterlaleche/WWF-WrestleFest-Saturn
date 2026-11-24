/* Real VBlank handler + SMPC parser + input publication. Only hardware/time
 * and unrelated VBlank work are substituted. No joystick algorithm stub. */
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "jo/types.h"
#include "jo/sega_saturn.h"
#include "jo/smpc.h"
#include "jo/input.h"
#define JO_PRINTF_BUF_SIZE 1024
#include "jo/tools.h"
#undef JO_ZERO
#define JO_ZERO(x) ((x)=0)
static struct {int width;} __jo_screen={320};
static unsigned char regs[128];
static unsigned mask,commands,pad_commands,rtc_commands,height,pal,cks,clock_hz;
static unsigned frcbase,latched_frc,latched_v,tvmd,exten,reads;
static uint64_t ns,line_ns,issue_ns,status_ready,callback_ns,usercallback_ns,music_ns,palette_done;
static int hardware; /* 0 idle, 1 status ready pending, 2 needs CONT, 3 pads */
static bool irq_pending,servicing,auto_vblank;
static unsigned pad_value,rtc_second,gun_order,erase_order,order;
static void host_advance(void);
static void hardware_step(void);
static void deliver_irq(void);
static void one_vblank(void);
static unsigned lines(void){return pal?313u:263u;}
static unsigned actual_line(void){return (unsigned)((ns/line_ns)%lines());}
static unsigned raw_line(void){unsigned l=actual_line(),v=pal?(height==240?267u:259u):(height==240?245u:237u);return l+(l>=v?512u-lines():0u);}
static unsigned frc(void){return (unsigned)((ns*clock_hz/(1000000000ull*(8u<<(2u*cks)))+frcbase)&65535u);}
static unsigned host_mmio(unsigned a){
 host_advance();switch(a){
 case 0xfffffe12u:latched_frc=frc();return latched_frc>>8;
 case 0xfffffe13u:return latched_frc&255u;
 case 0xfffffe16u:return cks;
 case 0x25f80000u:return tvmd;
 case 0x25f80002u:latched_v=raw_line();return exten;
 case 0x25f8000au:return latched_v;
 default:assert(0);return 0;}
}
static unsigned char host_read(jo_smpc_memory_address a){assert(a!=InputRegister0);host_advance();return regs[(unsigned)a&127];}
static void host_write(jo_smpc_memory_address a,unsigned char v){
 host_advance();unsigned old=regs[(unsigned)a&127];regs[(unsigned)a&127]=v;
 if(a==InputRegister0 && hardware==2 && ((v^old)&128)){
  /* A status continuation after OUT cannot start this field's acquisition. */
  if(actual_line()>=height && actual_line()<lines()-1)hardware=3;
 }
 if(a==CommandRegister){
  uint64_t phase=ns%(line_ns*lines()),since=phase>=height*line_ns?phase-height*line_ns:phase+(lines()-height)*line_ns;
  assert(since>=300000u && mask==15);assert(hardware==0);commands++;issue_ns=ns;
  if(v==FetchStatusAndPeripheralData){
   if(regs[(unsigned)InputRegister1&127]&8){
    assert(gun_order && erase_order>gun_order);pad_commands++;
    assert(phase>=height*line_ns && phase<(lines()-1)*line_ns);
   }else rtc_commands++;
   if(regs[(unsigned)InputRegister0&127]&1){hardware=1;status_ready=ns+300000u;}
   else hardware=3;
  }else regs[(unsigned)StatusFlag&127]=0;
 }
}
static void host_poll(void){host_advance();}
static void host_begin(void){while(host_read(StatusFlag)&1)host_poll();host_write(StatusFlag,1);}
static void host_end(jo_smpc_command c){host_write(CommandRegister,c);while(host_read(StatusFlag)&1)host_poll();}
static void __jo_core_restore_scu_mask(void){}
static unsigned __jo_core_disable_interrupts(void){unsigned old=mask;mask=15;return old;}
static void __jo_core_restore_interrupts(unsigned sr){mask=sr;deliver_irq();}
static void __jo_core_set_interrupt_handler(unsigned v,void(*h)(void)){(void)v;(void)h;}
void __jo_vdp2_set_external_latch(bool e){(void)e;}
#define jo_smpc_read_byte host_read
#define jo_smpc_write_byte host_write
#define jo_smpc_begin_command host_begin
#define jo_smpc_end_command host_end
#include "smpc.inc"

jo_peripheral jo_inputs[JO_INPUT_MAX_DEVICE];
static unsigned short jo_input_push[JO_INPUT_MAX_DEVICE],jo_input_pull[JO_INPUT_MAX_DEVICE];
static unsigned short jo_input_reported_push[JO_INPUT_MAX_DEVICE],jo_input_reported_pull[JO_INPUT_MAX_DEVICE];
typedef struct {short x,y;bool light;} jo_input_gun_shot;
static jo_input_gun_shot jo_input_gun_pending[2],jo_input_gun_frame[2];
#define __JO_SIZEOF_6_PerDigital_SIZE_DIV_SIZEOF_INT 36
#define __JO_SECOND_MULTITAP_OFFSET 54
#include "input.inc"

static unsigned __jo_core_vblank_count;
static struct {int count;} __vblank_callbacks={1};
static int divisor_register,high_register,quotient_register;
#undef JO_SH2_DVSR
#undef JO_SH2_DVDNTH
#undef JO_SH2_DVDNTL
#define JO_SH2_DVSR divisor_register
#define JO_SH2_DVDNTH high_register
#define JO_SH2_DVDNTL quotient_register
static void (*__jo_core_music_vblank)(void);
static void host_gun_vblank(void){gun_order=++order;ns+=1000;}
static void __jo_vdp1_vblank(void){erase_order=++order;ns+=1000;}
static void __jo_time_vblank(void){ns+=1000;}
static void __jo_vdp2_vblank(void){ns+=callback_ns;hardware_step();}
static void costly_music(void){ns+=music_ns;hardware_step();}
static void __vblank_callbacks_run(void){ns+=usercallback_ns;palette_done=ns;jo_get_inputs_vblank();}
#define __jo_vblank_callbacks __vblank_callbacks_run
#include "core.inc"
#include "rtc.inc"

static void deliver_irq(void){
 if(irq_pending && mask<8 && !servicing){unsigned old=mask;mask=8;irq_pending=false;servicing=true;jo_smpc_handler();servicing=false;mask=old;}
}
static void hardware_step(void){
 if(hardware==1 && ns>=status_ready){
  for(unsigned i=0;i<32;i++)regs[((unsigned)OutputRegister0+2*i)&127]=0;
  regs[((unsigned)OutputRegister0+2)&127]=0x20;regs[((unsigned)OutputRegister0+4)&127]=0x26;
  regs[((unsigned)OutputRegister0+6)&127]=0x0b;regs[((unsigned)OutputRegister0+8)&127]=0x13;
  regs[((unsigned)OutputRegister0+14)&127]=(rtc_second/10)*16+rtc_second%10;
  if(regs[(unsigned)InputRegister1&127]&8){regs[(unsigned)StatusRegister&127]=0x6f;hardware=2;}
  else {regs[(unsigned)StatusRegister&127]=0x4f;regs[(unsigned)StatusFlag&127]=0;hardware=0;}
  irq_pending=true;
 }
 if(hardware==3 && ns>=((issue_ns/(line_ns*lines()))*lines()+lines())*line_ns+100000u){
  unsigned char data[32]={0xf1,2,pad_value>>8,pad_value&255,0xf1,2,0xff,0xff};
  for(unsigned i=0;i<32;i++)regs[((unsigned)OutputRegister0+2*i)&127]=data[i];
  regs[(unsigned)StatusRegister&127]=0xc0;regs[(unsigned)StatusFlag&127]=0;hardware=0;irq_pending=true;
 }
 deliver_irq();
}
static void host_advance(void){
 assert(++reads<5000000);uint64_t old=ns;ns+=100;hardware_step();
 if(auto_vblank && mask==0 && old/line_ns!=ns/line_ns && actual_line()==height)one_vblank();
}
static void one_vblank(void){
 unsigned old=mask;mask=15;order=gun_order=erase_order=0;jo_core_vblank_in_handler();mask=old;hardware_step();
}
static void reset(unsigned p,unsigned c){
 memset(regs,0,sizeof regs);memset(__jo_smpc_peripherals,0,sizeof __jo_smpc_peripherals);memset(jo_inputs,0,sizeof jo_inputs);
 memset(__jo_smpc_previous_pressed,0,sizeof __jo_smpc_previous_pressed);
 memset(jo_input_push,0,sizeof jo_input_push);memset(jo_input_pull,0,sizeof jo_input_pull);
 memset(jo_input_reported_push,0,sizeof jo_input_reported_push);memset(jo_input_reported_pull,0,sizeof jo_input_reported_pull);
 pal=p;height=240;cks=c;clock_hz=pal?26687500:26875000;line_ns=pal?64000:63556;frcbase=65520;tvmd=0x8010;exten=0;
 ns=0;mask=commands=pad_commands=rtc_commands=reads=0;hardware=0;irq_pending=servicing=auto_vblank=false;
 __jo_smpc_intback_busy=__jo_smpc_command_pending=false;__jo_smpc_intback_length=0;
 __jo_smpc_gun_ports=__jo_smpc_gun_found=0;__jo_smpc_port_end[0]=15;__jo_smpc_port_end[1]=30;__jo_smpc_timing_fault=0;
 for(unsigned i=0;i<30;i++)jo_smpc_clear_peripheral(&__jo_smpc_peripherals[i]);
 callback_ns=pal?4000000:1200000;pad_value=0xffff;rtc_second=1;
 usercallback_ns=music_ns=palette_done=0;__jo_core_music_vblank=NULL;
}
static void test_cadence(void){
 for(unsigned p=0;p<2;p++)for(unsigned c=0;c<3;c++){
  reset(p,c);
  for(unsigned f=0;f<36;f++){
   unsigned previous=pad_value;pad_value=f>=5&&f<26?0xfdff:0xffff;
   ns=(f*lines()+height)*line_ns;one_vblank();
   assert(pad_commands==f+1); /* old end-of-handler guard starves here */
   __jo_input_frame_start();if(f)assert(jo_inputs[0].data==previous);
   ns=((f+1)*lines()+10)*line_ns;hardware_step();
   assert(!__jo_smpc_intback_busy && __jo_smpc_peripherals[0].data==pad_value);
   assert(__jo_smpc_peripherals[0].size==2 && !__jo_smpc_timing_fault);
  }
 }
 puts("PASS actual core ISR: 216 consecutive requests, final packets, press/hold/release and jo_inputs publication");
}
static void test_rtc(void){
 for(unsigned p=0;p<2;p++)for(unsigned sr=0;sr<=15;sr+=15){
  reset(p,1);ns=100*line_ns;mask=sr;auto_vblank=sr==0;
  __jo_smpc_peripherals[0].data=0xfdff;jo_peripheral old[30];memcpy(old,__jo_smpc_peripherals,sizeof old);
  jo_datetime now;jo_getdate(&now);assert(now.year==2026 && now.month==11 && now.day==13 && now.second==1);
  assert(rtc_commands==1 && mask==sr && !__jo_smpc_command_pending);
  rtc_second=47;jo_getdate(&now);assert(now.second==47 && rtc_commands==2 && mask==sr);
  assert(!memcmp(old,__jo_smpc_peripherals,sizeof old));
  if(sr==15){
   /* SF completion left the status IRQ pending. VBlank15 wins first. */
   assert(irq_pending);ns=height*line_ns;one_vblank();
   assert(pad_commands==0 && !memcmp(old,__jo_smpc_peripherals,sizeof old));
   mask=0;deliver_irq();assert(!irq_pending);
   ns=(lines()+height)*line_ns;one_vblank();assert(pad_commands==1);
   ns=(2*lines()+10)*line_ns;hardware_step();assert(!__jo_smpc_intback_busy);
  }
 }
 for(unsigned p=0;p<2;p++){
  reset(p,1);pad_value=0xfdff;ns=height*line_ns;one_vblank();
  assert(__jo_smpc_intback_busy);auto_vblank=true;
  jo_datetime now;jo_getdate(&now);
  assert(now.second==1 && !__jo_smpc_intback_busy && !__jo_smpc_command_pending);
  assert(__jo_smpc_peripherals[0].data==0xfdff && mask==0 && rtc_commands==1);
 }
 puts("PASS actual jo_getdate: RTC advances between reads, caller mask and published pads retained");
}
static void test_publication(void){
 for(unsigned p=0;p<2;p++){
  reset(p,1);callback_ns=12000;usercallback_ns=672000;music_ns=1434000;
  __jo_core_music_vblank=costly_music;ns=height*line_ns;one_vblank();
  assert(palette_done<(lines()-1)*line_ns);
  assert(pad_commands==1);
  ns=(lines()+10)*line_ns;hardware_step();assert(!__jo_smpc_intback_busy);
 }
 puts("PASS video publication before OUT with measured title callback costs; full peripheral packet survives delayed IRQ");
}
int main(int argc,char **argv){assert(argc==2);if(!strcmp(argv[1],"all")||!strcmp(argv[1],"cadence"))test_cadence();if(!strcmp(argv[1],"all")||!strcmp(argv[1],"rtc"))test_rtc();if(!strcmp(argv[1],"all")||!strcmp(argv[1],"publication"))test_publication();}
