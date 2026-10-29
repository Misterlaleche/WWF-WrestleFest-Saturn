/* Hardware boundary fake; real adapter and original SFX sequencer run here.
 * Catches wrong sample/bank/volume, late CD reads, lost concurrent cues,
 * F1 restarts and premature truncation after the scene stops advancing. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wp02_audio.h"
#include "ponesound_min.h"
#include "wp02_pcm16.h"
#include "wf_audio_sfx.h"

static unsigned cd_reads, loads, starts, ceases, blanks;
static uint8_t scsp[0x80000];
static uint8_t *pack;
static size_t pack_bytes;
static void (*frt_handler)(void);
static uint8_t mask;
static uint16_t frt_count;
static uint64_t physical_frt_ticks;
static unsigned frc_write_latency, irq_count, zero_compares;
static unsigned read_latency_once;
cpu_ioregs_t test_cpu_ioregs;
static struct { unsigned bytes, offset, rate, volume; } cfg[3];

/* Model the real peripheral boundary, not the software clock. The hardware
 * counter continues while an IRQ is masked. Compare registers can wrap to 0;
 * Yaul's initial-registration API deliberately disables an OCRA value of 0. */
static void advance_ticks(uint64_t ticks);
static void service_frt(void) {
    if (!frt_handler || mask >= 8u || !(test_cpu_ioregs.tier & 8u) ||
        !(test_cpu_ioregs.ftcsr & 8u)) return;
    uint8_t old=mask;mask=8u;
    test_cpu_ioregs.ftcsr &= (uint8_t)~8u; /* Yaul clears before callback. */
    ++irq_count;
    frt_handler();
    mask=old;
}
static void advance_ticks(uint64_t ticks) {
    const uint64_t target=physical_frt_ticks+ticks;
    while (physical_frt_ticks<target) {
        uint16_t compare=(uint16_t)((test_cpu_ioregs.ocrah<<8)|test_cpu_ioregs.ocral);
        unsigned distance=(uint16_t)(compare-frt_count);
        if (!distance) distance=65536u;
        uint64_t step=target-physical_frt_ticks;
        if (step>distance) step=distance;
        frt_count=(uint16_t)(frt_count+step);physical_frt_ticks+=step;
        if (step==distance) {
            test_cpu_ioregs.ftcsr|=8u;
            if (test_cpu_ioregs.ftcsr&1u) frt_count=0;
            if (compare==0) ++zero_compares;
        }
        service_frt();
    }
}

int cd_block_sectors_read(uint32_t fad,void *dst,size_t bytes) {
    size_t offset=(fad-100u)*2048u;
    assert(fad>=100u && offset+bytes<=pack_bytes);
    assert(bytes%2048u==0u);
    memcpy(dst,pack+offset,bytes); ++cd_reads; return 0;
}
uint32_t cpu_frt_count_get(void) {
    uint16_t snapshot=frt_count;
    unsigned latency=read_latency_once;read_latency_once=0;
    if (latency) advance_ticks(latency);
    return snapshot;
}
void cpu_frt_count_set(uint32_t x) {
    if (frc_write_latency) advance_ticks(frc_write_latency);
    frt_count=(uint16_t)x;
}
void cpu_frt_init(unsigned x) {
    (void)x;memset(&test_cpu_ioregs,0,sizeof(test_cpu_ioregs));frt_count=0;
}
void cpu_frt_oca_set(unsigned x,void (*f)(void)) {
    test_cpu_ioregs.ocrah=(uint8_t)(x>>8);test_cpu_ioregs.ocral=(uint8_t)x;
    test_cpu_ioregs.ftcsr&=(uint8_t)~8u;
    if (x && f) { test_cpu_ioregs.tier|=8u;frt_handler=f; }
    else test_cpu_ioregs.tier&=(uint8_t)~8u;
}
void cpu_frt_interrupt_priority_set(unsigned x) {(void)x;}
uint8_t cpu_intc_mask_get(void) {return mask;}
void cpu_intc_mask_set(uint8_t x) {mask=x;service_frt();}
void wf_ponesound_load_driver(const uint8_t *d,int n) {
    assert(d&&n==4);memset(scsp,0,sizeof(scsp));++loads;
}
void wf_ponesound_copy_to_scsp(const void *p,int n,int off) {
    assert(off>=0x14000 && off+n<=0x80000);
    memcpy(scsp+off,p,(size_t)n);
}
void wp02_pcm16_configure(unsigned v,unsigned samples,uint32_t off) {
    assert(v<3);cfg[v].bytes=samples*2;
    cfg[v].offset=off;cfg[v].rate=8000;
}
void wf_ponesound_pcm_play(short v,char type,char volume) {
    assert(v>=0&&v<3&&type==WP02_PCM_SEMI);
    cfg[v].volume=(uint8_t)volume;++starts;
}
void wf_ponesound_pcm_cease(short v) {assert(v>=0&&v<3);++ceases;}
void wf_ponesound_vblank(void *w) {(void)w;++blanks;}

static void ms(unsigned n) {assert(frt_handler);advance_ticks((uint64_t)n*3359u);}

static void test_clock_survives_latency_wrap_and_pending_irq(void) {
    /* A software FRC read/subtract/write loses these 3 physical ticks on
     * every interrupt. A free-running counter must retain all elapsed time. */
    uint64_t begin=wp02_audio_time_frt_ticks(),physical=physical_frt_ticks;
    frc_write_latency=3u;
    ms(1000);
    assert(wp02_audio_time_frt_ticks()-begin==physical_frt_ticks-physical);

    /* Reading immediately before a 16-bit rollover returns a coherent latched
     * value; the next getter must include the ticks elapsed during the read. */
    unsigned until_wrap=(uint16_t)(65534u-frt_count);
    advance_ticks(until_wrap);
    begin=wp02_audio_time_frt_ticks();
    read_latency_once=5u;
    assert(wp02_audio_time_frt_ticks()==begin);
    assert(wp02_audio_time_frt_ticks()==begin+5u);

    /* Three missed compare deadlines while masked must neither reverse the
     * clock nor lose sequencer time when the pending IRQ finally runs. */
    cpu_intc_mask_set(15);
    begin=wp02_audio_time_frt_ticks();physical=physical_frt_ticks;
    advance_ticks(10084u); /* 3*3359+7 */
    assert(wp02_audio_time_frt_ticks()==begin+10084u);
    cpu_intc_mask_set(0);
    assert(wp02_audio_time_frt_ticks()-begin==physical_frt_ticks-physical);
    unsigned interrupts_before=irq_count;
    ms(2);
    assert(irq_count>interrupts_before);

    /* 3359 and 65536 are coprime: after 65536 periods OCRA reaches 0.
     * Reusing cpu_frt_oca_set(0,...) would stop the timer here. */
    begin=wp02_audio_time_frt_ticks();physical=physical_frt_ticks;
    ms(70000);
    assert(wp02_audio_time_frt_ticks()-begin==physical_frt_ticks-physical);
    assert(zero_compares>0u);
}

int main(int argc,char **argv) {
    assert(argc==2);FILE *f=fopen(argv[1],"rb");assert(f);
    assert(fseek(f,0,SEEK_END)==0);pack_bytes=(size_t)ftell(f);rewind(f);
    pack=malloc(pack_bytes);assert(pack);
    assert(fread(pack,1,pack_bytes,f)==pack_bytes);fclose(f);
    const uint8_t driver[4]={0};
    const uint8_t ids[4]={0,1,7,9};
    assert(!wp02_audio_init(0,driver,4,ids));
    assert(wp02_audio_init(100,driver,4,ids));
    unsigned init_reads=cd_reads;
    assert(wp02_audio_state()==WP02_AUDIO_READY && cd_reads>0u && loads==1u);
    uint64_t begin=wp02_audio_time_frt_ticks();
    /* Common sample 17 uses original OKI gain 6/32. Raw PCM samples are
     * multiples of 16, so the independently calculated result is exact. */
    for (unsigned i=0;i<63956u;i+=2u) {
        int raw=(int16_t)((pack[i]<<8)|pack[i+1u]);
        int actual=(int16_t)((scsp[0x14000u+i]<<8)|scsp[0x14001u+i]);
        assert(actual==raw*3/16);
    }
    assert(!wp02_audio_emit(0x04)); /* No invented music producer/backend. */
    assert(wp02_audio_emit(0x7B));
    assert(starts==1u && cfg[0].bytes==49808);
    assert(cfg[0].rate==8000u && cfg[0].volume==7u);
    ms(1200);assert(starts==1u); /* F1 ignores repeated start while playing. */
    assert(wp02_audio_time_frt_ticks()-begin==1200u*CPU_FRT_NTSC_320_8_COUNT_1MS);
    assert(wp02_audio_time_us()==1199905u); /* Official NTSC320 clock rational. */
    advance_ticks(1000);
    uint64_t live=wp02_audio_time_frt_ticks();
    assert(live-begin==1200u*CPU_FRT_NTSC_320_8_COUNT_1MS+1000u);
    uint16_t untouched=frt_count;
    assert(wp02_audio_time_frt_ticks()==live && frt_count==untouched);
    assert(wp02_audio_emit(0x6D));
    assert(starts==2u && cfg[1].bytes==21364u);
    assert(wp02_audio_emit(0x31));
    assert(starts==3u && cfg[2].bytes==63956u && cfg[2].offset==0x14000);
    assert(wp02_audio_active_voices()==3u && cd_reads==init_reads);
    /* Advance audio alone after animation freeze: voices finish naturally. */
    ms(6000);wp02_audio_vblank(NULL);
    assert(wp02_audio_active_voices()==0u && blanks==1u && cd_reads==init_reads);
    assert(wp02_audio_last_command()==0x31u);
    wp02_audio_stop();assert(wp02_audio_state()==WP02_AUDIO_DISABLED);
    assert(!wp02_audio_emit(0x7C));
    const uint8_t team_ids[4]={4,5,10,11};
    assert(wp02_audio_init(100,driver,4,team_ids));
    assert(wp02_audio_emit(0x73));
    assert(cfg[0].bytes==26760u);
    assert(!wp02_audio_emit(0x75)); /* Unselected assets cannot play stale RAM. */
    const uint8_t worst_ids[4]={1,2,7,9};
    assert(wp02_audio_init(100,driver,4,worst_ids));
    assert(wp02_audio_sound_ram_end()==0x6C428u);
    assert(wp02_audio_time_frt_ticks()>=live); /* Reload never reverses epoch. */
    wp02_audio_stop();
    test_clock_survives_latency_wrap_and_pending_irq();
    /* Repeat initialization after the long frozen scene without reversing
     * the live epoch or restarting the hardware timebase. */
    begin=wp02_audio_time_frt_ticks();
    uint64_t physical=physical_frt_ticks;
    assert(wp02_audio_init(100,driver,4,worst_ids));
    assert(wp02_audio_time_frt_ticks()-begin==physical_frt_ticks-physical);
    assert(wp02_audio_emit(0x7C));
    assert(wf_sfx_voice(0)->ticks_remaining==90u);
    cpu_intc_mask_set(15);
    advance_ticks(20154u); /* 6 nominal milliseconds produce one Timer-A tick. */
    cpu_intc_mask_set(0);
    assert(wf_sfx_voice(0)->ticks_remaining==89u);
    wp02_audio_stop();free(pack);
    puts("PASS: PCM16 bank selection, concurrent cues, F1, natural tails, no runtime CD reads, worst roster fits");
    puts("PASS: free-running FRT retains latency, 16-bit wrap, pending IRQs, zero compare and >65s restart; audio catches up");
    return 0;
}
