/* Resident ADPCM profile. No poneSound allocator, ADX, IRQ or VBlank dependency. */
#include "wf_scsp_sfx.h"
#include "wf_native_driver.h"
#include "wf_scsp_fm_probe.h"
extern const wf_scsp_fm_probe_patch_t wf_scsp_fm_first_patch;
static volatile wf_native_mail_t *const mail=(volatile wf_native_mail_t *)WF_NATIVE_MAIL_OFFSET;
wf_scsp_sfx_t wf_sfx_engine;
static uint32_t clock_phase,clock_frames,last_poll;
static unsigned last_ca;
static bool write16(uint32_t a,uint16_t v,void *u)
{
    (void)u;
    if((a&1u)||!((a>=0x3000&&a<0xb000)||(a>=0x100000&&a<0x100400)))return false;
    *(volatile uint16_t *)(uintptr_t)a=v;return true;
}
static uint32_t clock_now(void *user)
{
    unsigned ca=(*(volatile uint16_t *)0x100408u>>7)&15u;
    unsigned delta,n;
    uint32_t quotient;
    (void)user;
    if(ca==15){wf_sfx_engine.fault=1;return clock_frames;}
    delta=ca>=last_ca?ca-last_ca:ca+15u-last_ca;last_ca=ca;
    if(!delta)return clock_frames;
    /* Slot30: LEA=15*4096, PCM8, OCT6/FNS1023. One CA bin is
     * 65536/2047 SCSP frames. Poll interval must remain below 10.89 ms.
     * Unlike SCSP TimerA this cursor wraps; TimerA saturates at expiry. */
    quotient=clock_phase+((uint32_t)delta<<16);
    /* Numerator < 14*65536+2047, quotient < 449: the original 68000's
     * 32/16 DIVU is exact and cannot overflow its 16-bit quotient. */
    __asm__ volatile("divu.w #2047,%0":"+d"(quotient)::"cc");
    clock_phase=quotient>>16;n=quotient&65535u;
    clock_frames+=n;return clock_frames;
}
static void poll_clock(void)
{
    uint32_t frame=clock_now(NULL),elapsed=frame-last_poll;last_poll=frame;
    if(elapsed)wf_scsp_sfx_advance(&wf_sfx_engine,(unsigned)elapsed);
}
static void service(void)
{
    uint16_t req=mail->request,result=WF_NATIVE_OK,value=0,arg=mail->argument;
    unsigned v;int status;
    if(req==mail->ack)return;
    switch(mail->command){
    case WF_NATIVE_FM_START:
        if(!wf_scsp_fm_probe_start(&wf_scsp_fm_first_patch,write16,NULL))result=WF_NATIVE_BAD_STATE;
        else mail->fm_active=1;
        break;
    case WF_NATIVE_FM_STOP:
        if(!wf_scsp_fm_probe_stop(&wf_scsp_fm_first_patch,write16,NULL))result=WF_NATIVE_BAD_STATE;
        else mail->fm_active=0;
        break;
    case WF_NATIVE_SFX_BIND: {
        const uint8_t *p=(const uint8_t *)WF_SFX_PACK_BASE;
        uint32_t bytes=(uint32_t)p[8]<<24|(uint32_t)p[9]<<16|(uint32_t)p[10]<<8|p[11];
        if(!wf_scsp_sfx_bind(&wf_sfx_engine,p,bytes))result=WF_NATIVE_BAD_STATE;
        clock_phase=0;last_ca=(*(volatile uint16_t *)0x100408u>>7)&15u;
        clock_frames=last_poll=wf_sfx_engine.frames;
        break; }
    case WF_NATIVE_SFX_STOP:wf_scsp_sfx_stop_all(&wf_sfx_engine);wf_sfx_engine.bound=0;break;
    case WF_NATIVE_SFX_QUERY:
        for(v=0;v<4;v++)if(wf_sfx_engine.pcm[v].playing)value|=(uint16_t)(1u<<v);
        if(wf_sfx_engine.bound)value|=0x100u;
        break;
    case WF_NATIVE_OKI_START:
        if(!wf_scsp_sfx_oki_start(&wf_sfx_engine,(arg>>12)&3u,(arg>>11)&1u,(arg>>4)&127u,arg&15u,false))result=WF_NATIVE_BAD_STATE;
        break;
    case WF_NATIVE_OKI_STOP:
        for(v=0;v<4;v++)if(arg&(1u<<v))wf_scsp_sfx_stop(&wf_sfx_engine,v);
        break;
    case WF_NATIVE_ARCADE_WORD:
        mail->reserved[0]=arg;value=arg;
        if((arg&255u)==0){
            (void)wf_scsp_fm_probe_stop(&wf_scsp_fm_first_patch,write16,NULL);mail->fm_active=0;
            wf_scsp_sfx_stop_all(&wf_sfx_engine);
        }else if((arg&255u)>=0x20u&&(arg&255u)<=0x81u){
            status=wf_scsp_sfx_command(&wf_sfx_engine,(uint8_t)arg);
            if(status== -1)result=WF_NATIVE_PRIORITY_REJECTED;
            else if(status<0)result=WF_NATIVE_BAD_STATE;
        }else result=WF_NATIVE_UNIMPLEMENTED;
        break;
    case WF_NATIVE_RING_PREPARE:case WF_NATIVE_RING_START:case WF_NATIVE_RING_STOP:case WF_NATIVE_RING_CA:
        result=WF_NATIVE_UNIMPLEMENTED;break;
    default:result=WF_NATIVE_BAD_COMMAND;break;
    }
    mail->reserved[1]=(uint16_t)wf_sfx_engine.underruns;
    mail->value=value;mail->result=result;mail->ack=req;
}
void wf_sfx_main(void)
{
    unsigned i;
    mail->state=WF_NATIVE_INIT;
    for(i=0;i<512;i++)*(volatile uint16_t *)(0x100000u+i*2u)=0;
    *(volatile uint16_t *)0x1003e0u=0x1000; /* apply key-off to every slot */
    *(volatile uint16_t *)0x10041eu=0; /* SCIEB: no IRQ to 68000 */
    *(volatile uint16_t *)0x10042au=0; /* MCIEB: no residual IRQ to SH2 */
    *(volatile uint16_t *)0x100400u=0x020f;
    wf_scsp_sfx_init(&wf_sfx_engine,write16,NULL);
    wf_sfx_engine.now=clock_now;
    /* Muted clock; STWINH avoids polluting the FM modulation ring. */
    {
        static const uint16_t clock[12]={0x30,0,0,0xf000,31,31,0x2ff,0,0x33ff,0,0,0};
        for(i=0;i<12;i++)*(volatile uint16_t *)(0x1003c0u+i*2u)=clock[i];
        *(volatile uint16_t *)0x1003c0u=0x1830;
        *(volatile uint16_t *)0x100408u=WF_SFX_CLOCK_SLOT<<11;
    }
    last_ca=(*(volatile uint16_t *)0x100408u>>7)&15u;
    mail->magic_hi=0x5746;mail->magic_lo=0x4e31;mail->abi=WF_NATIVE_ABI;
    mail->reserved[2]=WF_NATIVE_SFX_PROFILE;mail->state=WF_NATIVE_READY;
    for(;;){
        poll_clock();service();poll_clock();wf_scsp_sfx_work(&wf_sfx_engine);
        if(wf_sfx_engine.fault){wf_scsp_sfx_stop_all(&wf_sfx_engine);mail->reserved[1]=(uint16_t)wf_sfx_engine.underruns;mail->state=WF_NATIVE_FAULT;for(;;){} }
    }
}
