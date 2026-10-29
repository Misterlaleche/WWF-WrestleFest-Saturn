#include "wp01_audio.h"
#include "wp01_audio_queue.h"
#include <yaul.h>
#include <smpc/smc.h>

#define REG(off) (*(volatile uint16_t *)(uintptr_t)(0x25B00000U+(off)))
#define SOUND_WORD(i) (*(volatile uint16_t *)(uintptr_t)(0x25A00000U+2U*(i)))
/* SH-2 NTSC 320 clock=3843000000/143 Hz; FRT divider32.
 * Two 44.1kHz samples need39 FRT ticks. Preserve every KEYOFF/KEYON edge. */
enum { KEY_EXEC=0x1000,KEY_ON=0x0800,KEY_GUARD_TICKS=39,HEARTBEAT_TICKS=0x4000 };
static wp01_audio_queue_t queue;
static volatile bool playing;
static bool bound,key_valid;
static uint16_t last_counter,last_key_counter;
static uint32_t elapsed;
volatile wp01_audio_debug_t wp01_audio_debug={.magic=0x57413132U};

static uint32_t get32(const uint8_t *p){return ((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|((uint32_t)p[2]<<8)|p[3];}
static uint16_t counter(void){
    volatile cpu_ioregs_t *const io=(volatile cpu_ioregs_t *)CPU_IOREG_BASE;
    /* A low-byte carry between separate byte reads must not move time back. */
    uint8_t high,before,low;
    do{
        before=io->frch;low=io->frcl;
        high=io->frch;(void)io->frcl; /* Complete both documented byte pairs. */
    }while(before!=high);
    return (uint16_t)(((uint16_t)high<<8)|low);
}
static void interrupt(void);
/* Own only compare A. The SDK dispatcher also calls the BIOS no-op entry for
 * an inactive compare B; some HLE BIOS implementations do not populate it. */
static void __interrupt_handler compare_interrupt(void){
    volatile cpu_ioregs_t *const io=(volatile cpu_ioregs_t *)CPU_IOREG_BASE;
    uint8_t status=io->ftcsr;
    if(status&0x08U){
        io->tier&=~0x08U;io->ftcsr=status&~0x08U;
        interrupt();
    }
}
static void clock_update(void){
    uint16_t now=counter();
    elapsed+=(uint16_t)(now-last_counter);last_counter=now;
}
static void write_register(void *context __unused,uint16_t offset,uint16_t value){
    if((offset&31U)==0U){
        /* KYONEX samples all slots' KYONB bits. Preserve the preceding group
         * before changing any slot's control, not only before its next EXEC. */
        if(key_valid&&(uint16_t)(counter()-last_key_counter)<KEY_GUARD_TICKS){
            ++wp01_audio_debug.key_guards;
            while((uint16_t)(counter()-last_key_counter)<KEY_GUARD_TICKS){}
        }
        REG(offset)=value;
        if(value&KEY_EXEC){last_key_counter=counter();key_valid=true;}
    }else REG(offset)=value;
}
static void arm_next(void){
    clock_update();
    uint32_t delay=HEARTBEAT_TICKS;
    if(queue.next<queue.event_count){
        uint32_t due=get32(queue.events+queue.next*8U);
        delay=due>elapsed?due-elapsed:16U;
        if(delay<16U)delay=16U;
        if(delay>HEARTBEAT_TICKS)delay=HEARTBEAT_TICKS;
    }
    uint16_t target=(uint16_t)(last_counter+delay);if(!target)++target;
    cpu_frt_oca_set(target,interrupt);
    /* If a higher-priority interrupt delayed programming, avoid a full wrap. */
    if((int16_t)(target-counter())<=0){
        target=(uint16_t)(counter()+64U);if(!target)++target;
        cpu_frt_oca_set(target,interrupt);++wp01_audio_debug.timer_rearms;
    }
}
static void interrupt(void){
    uint16_t start=counter();
    if(!playing)return;
    clock_update();wp01_audio_queue_advance(&queue,elapsed);
    ++wp01_audio_debug.interrupts;
    wp01_audio_debug.elapsed_ticks=elapsed;wp01_audio_debug.event_index=queue.next;
    wp01_audio_debug.max_lateness_ticks=queue.max_lateness;wp01_audio_debug.max_burst=queue.max_burst;
    arm_next();
    unsigned ticks=(uint16_t)(counter()-start);
    if(ticks>wp01_audio_debug.max_handler_ticks)wp01_audio_debug.max_handler_ticks=ticks;
}
void wp01_audio_stop(void){
    uint8_t mask=cpu_intc_mask_get();cpu_intc_mask_set(15);
    playing=false;wp01_audio_debug.playing=0;cpu_frt_oca_clear();
    for(unsigned i=0;i<32;i++){
        REG(i*32U+0x16)=0;
        REG(i*32U)=REG(i*32U)&~(KEY_ON|KEY_EXEC);
    }
    REG(0)=REG(0)|KEY_EXEC;
    cpu_intc_mask_set(mask);
}
void wp01_audio_init(void){
    playing=false;bound=false;key_valid=false;
    smpc_smc_call(SMPC_SMC_SNDOFF);smpc_smc_wait(0);
    REG(0x400)=0x020f;REG(0x41e)=0;REG(0x42a)=0;
    REG(0x422)=0x7ff;REG(0x42e)=0x7ff;
    for(unsigned i=0;i<32;i++)for(unsigned r=0;r<0x18;r+=2)REG(i*32U+r)=0;
    REG(0)=KEY_EXEC;
    for(unsigned r=0x800;r<0xc00;r+=2)REG(r)=0;
    cpu_frt_interrupt_priority_set(9);
    cpu_intc_ihr_set(CPU_INTC_INTERRUPT_FRT_OCI,compare_interrupt);
}
bool wp01_audio_bank_load(const void *data,uint32_t bytes){
    if(!data||bytes>0x80000U||(bytes&1U))return false;
    const uint16_t *p=data;
    for(unsigned i=0;i<bytes/2U;i++)SOUND_WORD(i)=p[i];
    return true;
}
bool wp01_audio_program_load(const void *data,uint32_t bytes){
    wp01_audio_stop();
    bound=wp01_audio_queue_bind(&queue,data,bytes,write_register,0);
    return bound;
}
void wp01_audio_prepare(void){
    wp01_audio_stop();
    if(!bound)return;
    key_valid=false;wp01_audio_queue_restart(&queue);elapsed=0;
    wp01_audio_debug.interrupts=0;wp01_audio_debug.elapsed_ticks=0;wp01_audio_debug.event_index=0;
    wp01_audio_debug.max_lateness_ticks=0;wp01_audio_debug.max_handler_ticks=0;
    wp01_audio_debug.max_burst=0;wp01_audio_debug.key_guards=0;wp01_audio_debug.timer_rearms=0;
}
void wp01_audio_start(void){
    if(!bound)return;
    elapsed=0;last_counter=counter();playing=true;wp01_audio_debug.playing=1;
    wp01_audio_queue_advance(&queue,0);arm_next();
}
void wp01_audio_service(void){}
uint32_t wp01_audio_written(void){return wp01_audio_debug.event_index;}
