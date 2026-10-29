#include "wp0_audio_player.h"
#include <yaul.h>
#include <smpc/smc.h>

/* Official Sega ST-077-R2-052594: individual-slot map p25, common map p28,
 * SDIR p72, native sample-rate pitch p73, CA granularity p89.
 * Only this SH2 module owns key execution. No M68K sound driver is running. */
#define SCSP_WORD(off) (*(volatile uint16_t *)(uintptr_t)(0x25B00000u+(off)))
#define SOUND_WORD(off) (*(volatile uint16_t *)(uintptr_t)(0x25A00000u+(off)))
enum { LOOP_FORWARD=0x0020u,KEY_ON=0x0800u,KEY_EXECUTE=0x1000u };

void wp0_scsp_initialize(void)
{
    smpc_smc_call(SMPC_SMC_SNDOFF);smpc_smc_wait(0);
    SCSP_WORD(0x400)=0x020Fu; /*512KiB DRAM,16bit DAC,full master volume. */
    SCSP_WORD(0x41E)=0;SCSP_WORD(0x42A)=0; /*No sound/main CPU IRQ sources. */
    SCSP_WORD(0x422)=0x07FFu;SCSP_WORD(0x42E)=0x07FFu;
    for(unsigned slot=0;slot<32;slot++)
        for(unsigned reg=0;reg<0x18;reg+=2)SCSP_WORD(slot*0x20u+reg)=0;
    SCSP_WORD(0)=KEY_EXECUTE; /*Stop any inherited slots. */
    /*Disable inherited DSP memory writes without touching waveform RAM. */
    for(unsigned reg=0x800;reg<0xC00;reg+=2)SCSP_WORD(reg)=0;

    SCSP_WORD(0x00)=LOOP_FORWARD;
    SCSP_WORD(0x02)=0; /*SA=0 bytes; PCM16. */
    SCSP_WORD(0x04)=0; /*LSA=0 samples. */
    /*Normal loop wraps at LEA:32768 is the exclusive endpoint of samples
     *0..32767. Sega p38/p52 also require the interpolation sample at LEA
     *to equal LSA. copy_pcm keeps that extra word at0x10000 in sync. */
    SCSP_WORD(0x06)=WP0_AUDIO_RING_SAMPLES;
    SCSP_WORD(0x08)=0x001Fu; /*Immediate attack; zero decay rates. */
    SCSP_WORD(0x0A)=0x001Fu; /*Fast release; zero key-rate scaling. */
    SCSP_WORD(0x0C)=0x0300u; /*SDIR=1,STWINH=1,TL=0: unmodified PCM. */
    SCSP_WORD(0x0E)=0;SCSP_WORD(0x10)=0; /*No FM;44.1kHz native pitch. */
    SCSP_WORD(0x12)=0;SCSP_WORD(0x14)=0; /*No LFO or DSP send. */
    SCSP_WORD(0x16)=0; /*Mute until exact start event. */
    SCSP_WORD(0x408)=0; /*Monitor slot0. CA is bits10..7,4096samples/unit. */
}
static bool copy_pcm(unsigned offset,const int16_t *samples,unsigned n,void *u)
{
    (void)u;if(offset>WP0_AUDIO_RING_SAMPLES||n>WP0_AUDIO_RING_SAMPLES-offset)return false;
    for(unsigned i=0;i<n;i++)SOUND_WORD(2u*(offset+i))=(uint16_t)samples[i];
    /*SCSP may fetch both neighboring samples even at the loop edge.
     *Reserve this word: compressed storage must begin at0x10002 or later. */
    if(offset==0&&n)SOUND_WORD(2u*WP0_AUDIO_RING_SAMPLES)=(uint16_t)samples[0];
    return true;
}
static unsigned read_region(void *u)
{ (void)u;return (SCSP_WORD(0x408)>>7)&15u; }
static bool key_on(void *u)
{
    (void)u;SCSP_WORD(0x408)=0;SCSP_WORD(0x16)=0xE000u;
    SCSP_WORD(0)=LOOP_FORWARD|KEY_ON|KEY_EXECUTE;return true;
}
static void key_off(void *u)
{
    (void)u;SCSP_WORD(0x16)=0;SCSP_WORD(0)=LOOP_FORWARD|KEY_EXECUTE;
}
wp0_audio_ops_t wp0_scsp_ops(void)
{
    const wp0_audio_ops_t ops={copy_pcm,read_region,key_on,key_off,NULL};return ops;
}
