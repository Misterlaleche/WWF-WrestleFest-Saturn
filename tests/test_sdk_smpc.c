/* The complete SDK smpc.c, with only MMIO, CPU mask operations and empty
 * waits substituted. No scheduler/polling predicate is reimplemented. */
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
static unsigned scu_ist,mask,restores,commands,last_command,continues,polls;
static unsigned char continuations[32];
static unsigned poll_mode,inject_race;
static void host_poll(void);
static unsigned char host_read(jo_smpc_memory_address a){assert(a!=InputRegister0);return regs[(unsigned)a&127];}
static void host_write(jo_smpc_memory_address a,unsigned char v){
    if(a==InputRegister0 && (v^regs[(unsigned)a&127])&128){
        assert(continues<32);continuations[continues++]=v;
    }
    regs[(unsigned)a&127]=v;
    if(a==CommandRegister){commands++;last_command=v;
        /* Commands complete asynchronously in hardware. INTBACK batches are
         * delivered explicitly below; the command's SF has already cleared. */
        regs[(unsigned)StatusFlag&127]=0;
    }
}
static void host_begin(void){while(host_read(StatusFlag)&1)host_poll();host_write(StatusFlag,1);}
static void host_end(jo_smpc_command command){host_write(CommandRegister,command);while(host_read(StatusFlag)&1)host_poll();}
static void __jo_core_restore_scu_mask(void){restores++;}
static unsigned __jo_core_disable_interrupts(void);
static void __jo_core_restore_interrupts(unsigned sr){mask=sr;}
static void __jo_core_set_interrupt_handler(unsigned vector,void(*handler)(void)){(void)vector;(void)handler;}
void __jo_vdp2_set_external_latch(bool enabled){(void)enabled;}
#undef JO_SCU_IST
#define JO_SCU_IST scu_ist
#define jo_smpc_read_byte host_read
#define jo_smpc_write_byte host_write
#define jo_smpc_begin_command host_begin
#define jo_smpc_end_command host_end
#include "sdk_smpc.inc"

static unsigned __jo_core_disable_interrupts(void){
    unsigned old=mask;
    if(inject_race){inject_race=0;__jo_smpc_vblank_in();}
    mask=15;return old;
}
static void host_poll(void){
    assert(++polls<10);assert(mask==0);
    if(poll_mode==1){
        /* The actual boot failure: no pending SMPC IRQ, SF=0, busy=1.
         * Only the next VBlank can release the synchronous command. */
        __jo_smpc_vblank_in();
    }else if(poll_mode==2){
        regs[(unsigned)StatusRegister&127]=0x80;jo_smpc_handler();
    }else assert(!"Unexpected busy wait");
}
static void reset(void){
    memset(regs,0,sizeof(regs));memset(__jo_smpc_peripherals,0,sizeof(__jo_smpc_peripherals));
    memset(__jo_smpc_previous_pressed,0,sizeof(__jo_smpc_previous_pressed));
    memset(__jo_smpc_intback_buffer,0,sizeof(__jo_smpc_intback_buffer));
    __jo_smpc_intback_length=0;__jo_smpc_intback_busy=false;
    __jo_smpc_reset_disable_depth=0;__jo_smpc_gun_ports=0;__jo_smpc_gun_found=0;
    __jo_smpc_intback_gun_ports=0;__jo_smpc_port_end[0]=15;__jo_smpc_port_end[1]=30;
    mask=restores=commands=last_command=continues=polls=poll_mode=inject_race=0;
    scu_ist=0x2884;
}
static void batch(unsigned char sr,const unsigned char *data){
    for(unsigned i=0;i<32;i++)regs[((unsigned)OutputRegister0+2*i)&127]=data?data[i]:0;
    regs[(unsigned)StatusRegister&127]=sr;jo_smpc_handler();
}
static void published_pad(void){
    __jo_smpc_peripherals[0].id=JO_PERIPHERAL_ID_STANDARD_PAD;
    __jo_smpc_peripherals[0].data=0xffef;__jo_smpc_previous_pressed[0]=0x10;
}
static void preserved(const jo_peripheral *old){assert(!memcmp(old,__jo_smpc_peripherals,sizeof(__jo_smpc_peripherals)));}
static void test_timeout(void){
    reset();published_pad();jo_peripheral old[30];memcpy(old,__jo_smpc_peripherals,sizeof old);
    __jo_smpc_intback_busy=true;__jo_smpc_intback_length=64;
    regs[(unsigned)InputRegister0&127]=0x80;regs[(unsigned)StatusRegister&127]=0x6f;
    __jo_smpc_vblank_in();
    assert(commands==0&&!__jo_smpc_intback_busy&&__jo_smpc_intback_length==0);preserved(old);
    assert(scu_ist==0x2884); /* SCU errata forbids writing IST. */
    batch(0x80,0);preserved(old);assert(!__jo_smpc_intback_busy);
    __jo_smpc_vblank_in();
    assert(commands==1&&last_command==FetchStatusAndPeripheralData&&__jo_smpc_intback_busy);
}
static void test_hardware_busy(void){
    reset();__jo_smpc_intback_busy=true;__jo_smpc_intback_length=32;
    regs[(unsigned)StatusFlag&127]=1;__jo_smpc_vblank_in();
    assert(commands==0&&__jo_smpc_intback_busy&&__jo_smpc_intback_length==32&&scu_ist==0x2884);
}
static void test_batches(void){
    reset();published_pad();jo_peripheral old[30];memcpy(old,__jo_smpc_peripherals,sizeof old);
    __jo_smpc_vblank_in();continues=0;
    batch(0x60,0);assert(continuations[0]==0x80);preserved(old);
    unsigned char data[32]={0xf1,2,0xff,0xef,0xf1,2,0xfb,0xff};
    batch(0xa0,data);assert(continuations[1]==0x00);preserved(old);
    batch(0xa0,0);assert(continuations[2]==0x80);preserved(old);
    batch(0x80,0);assert(!__jo_smpc_intback_busy&&__jo_smpc_intback_length==96);
    assert(__jo_smpc_peripherals[0].data==0xffef&&__jo_smpc_peripherals[15].data==0xfbff);
    __jo_smpc_vblank_in();continues=0;batch(0x60,0);
    assert(continues==1&&continuations[0]==0x80);
}
static void test_command_timeout(void){
    reset();published_pad();jo_peripheral old[30];memcpy(old,__jo_smpc_peripherals,sizeof old);
    __jo_smpc_intback_busy=true;__jo_smpc_intback_length=32;poll_mode=1;
    __jo_smpc_set_reset_button(false);
    assert(commands==1&&last_command==DisableReset&&polls==1&&mask==0);
    assert(!__jo_smpc_intback_busy&&__jo_smpc_intback_length==0);preserved(old);
    /* A delayed IRQ after cancellation cannot publish stale OREG data. */
    unsigned before=restores;batch(0x80,0);preserved(old);assert(restores==before+1);
    __jo_smpc_set_reset_button(false);assert(commands==1);
    __jo_smpc_set_reset_button(true);assert(commands==1);
    __jo_smpc_set_reset_button(true);assert(commands==2&&last_command==EnableReset);
    __jo_smpc_vblank_in();assert(commands==3&&__jo_smpc_intback_busy);
}
static void test_command_completion_and_race(void){
    reset();__jo_smpc_vblank_in();poll_mode=2;
    __jo_smpc_command(DisableReset);assert(commands==2&&last_command==DisableReset&&polls==1&&mask==0);
    reset();inject_race=1;__jo_smpc_command(DisableReset);
    assert(commands==1&&last_command==DisableReset&&!__jo_smpc_intback_busy&&mask==0);
}
int main(void){
    test_timeout();test_hardware_busy();test_batches();test_command_timeout();test_command_completion_and_race();
    puts("PASS SDK SMPC: timeout, SF-busy exclusion, 4 batches, retained pads, reset nesting, idle IRQ, command/IRQ race");
}
