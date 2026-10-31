#include "wf_native_driver.h"
#include "wf_scsp_fm_bundle.h"

static volatile wf_native_mail_t *const mail =
    (volatile wf_native_mail_t *)WF_NATIVE_MAIL_OFFSET;
static volatile uint16_t *const ring = (volatile uint16_t *)(0x100000+31*32);
static const uintptr_t sound_base=0;

void wf_native_driver_init(void)
{
    unsigned i;
    volatile uint16_t *p=(volatile uint16_t *)mail;
    for(i=0;i<sizeof(*mail)/2u;++i) p[i]=0;
    mail->state=WF_NATIVE_INIT;
    mail->magic_hi=0x5746;
    mail->magic_lo=0x4e31;
    mail->abi=WF_NATIVE_ABI;
    /* Monitor remains on slot 31, avoiding a read before changed MSLC latches. */
    *(volatile uint16_t *)0x100408=(uint16_t)(31u<<11);
    if(!wf_scsp_fm_probe_upload(&wf_scsp_fm_first_patch,
        wf_scsp_fm_wave, WF_FM_WAVE_BYTES,
        wf_scsp_fm_probe_mmio_write16, (void *)&sound_base)) {
        mail->state=WF_NATIVE_FAULT;
        return;
    }
    mail->state=WF_NATIVE_READY;
}

static void ring_stop(void)
{
    ring[0]=(uint16_t)((ring[0]&0xe7ffu)|0x1000u);
    mail->ring_active=0;
}

static void ring_prepare(void)
{
    unsigned i;
    ring_stop();
    for(i=0;i<16;++i) ring[i]=0;
    ring[0]=0x0026; /* forward PCM16 loop, SA high = 6 */
    ring[1]=0; ring[2]=0; ring[3]=32767;
    ring[4]=31; /* instant attack */
    ring[5]=31; /* release */
    ring[11]=0xe000; /* direct send 7, centered mono */
    mail->ring_prepared=1;
}

void wf_native_driver_service(void)
{
    uint16_t seq, cmd, result=WF_NATIVE_OK, value=0;
    if(mail->state!=WF_NATIVE_READY) return;
    seq=mail->request;
    if(seq==mail->ack) return;
    cmd=mail->command;
    switch(cmd) {
    case WF_NATIVE_FM_START:
    case WF_NATIVE_FM_STOP:
        /* One masked-IRQ 68000 thread. poneSound never allocates 0..3. */
        if(!(cmd==WF_NATIVE_FM_START ?
            wf_scsp_fm_probe_start(&wf_scsp_fm_first_patch,
                wf_scsp_fm_probe_mmio_write16,(void *)&sound_base) :
            wf_scsp_fm_probe_stop(&wf_scsp_fm_first_patch,
                wf_scsp_fm_probe_mmio_write16,(void *)&sound_base))) {
            result=WF_NATIVE_BAD_STATE;
            mail->state=WF_NATIVE_FAULT;
        } else mail->fm_active=(uint16_t)(cmd==WF_NATIVE_FM_START);
        break;
    case WF_NATIVE_RING_PREPARE: ring_prepare(); break;
    case WF_NATIVE_RING_START:
        if(!mail->ring_prepared) result=WF_NATIVE_BAD_STATE;
        else {
            ring[0]=(uint16_t)(ring[0]|0x1800u);
            mail->ring_active=1;
        }
        break;
    case WF_NATIVE_RING_STOP: ring_stop(); break;
    case WF_NATIVE_RING_CA:
        if(!mail->ring_prepared) result=WF_NATIVE_BAD_STATE;
        else value=(uint16_t)((*(volatile uint16_t *)0x100408>>7)&15u);
        break;
    case WF_NATIVE_ARCADE_WORD:
        /* Raw arcade WORD is preserved for the future native YM/OKI engine.
         * Do not map its command byte onto diagnostic FM or ring controls. */
        value=mail->argument;
        mail->reserved[0]=value;
        mail->reserved[1]=(uint16_t)(mail->reserved[1]+1u);
        if((value&255u)>0x81u) result=WF_NATIVE_BAD_COMMAND;
        else if((value&255u)==0u) {
            ring_stop();
            if(!wf_scsp_fm_probe_stop(&wf_scsp_fm_first_patch,
                wf_scsp_fm_probe_mmio_write16,(void *)&sound_base))
                result=WF_NATIVE_BAD_STATE;
            else mail->fm_active=0;
        } else result=WF_NATIVE_UNIMPLEMENTED;
        break;
    default: result=WF_NATIVE_BAD_COMMAND; break;
    }
    mail->value=value;
    mail->result=result;
    mail->ack=seq;
}
