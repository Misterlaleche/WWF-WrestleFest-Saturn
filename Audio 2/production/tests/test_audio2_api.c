#include "wf_audio2.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint32_t tick;
static unsigned writes;
static bool exited=true;
static wf_native_mail_t mail;
static wf_music_module_shared_t shared;
static uint32_t now(void *u){(void)u;return tick;}
static void wait_us(uint32_t n,void *u){(void)u;tick+=n;}
static void idle(void *u){(void)u;tick+=10;mail.ack=mail.request;mail.result=WF_NATIVE_OK;}
static void barrier(void *u){(void)u;}
static bool enter(void *u){(void)u;return true;}
static void leave(void *u){(void)u;}
static void write16(uint32_t a,uint16_t v,void *u){(void)a;(void)v;(void)u;writes++;}
static bool sound(bool on,void *u){(void)u;if(on){memset(&mail,0,sizeof mail);mail.state=WF_NATIVE_READY;mail.magic_hi=0x5746;mail.magic_lo=0x4e31;mail.abi=WF_NATIVE_ABI;mail.reserved[2]=WF_NATIVE_MUSIC_PROFILE;}return true;}
static bool start(uint32_t w,void *u){(void)u;assert(w==0x06020000u);memset(&shared,0,sizeof shared);shared.state=WF_MUSIC_BOOTING;exited=false;return true;}
static bool quiescent(void *u){(void)u;return exited;}
static bool another_worker_idle(void *u){(void)u;return true;}
static void *read_file(const char *name,size_t *n){FILE *f=fopen(name,"rb");assert(f);fseek(f,0,SEEK_END);*n=(size_t)ftell(f);rewind(f);void *p=malloc(*n);assert(p&&fread(p,1,*n,f)==*n);fclose(f);return p;}
int main(int argc,char **argv)
{
    assert(argc==3);
    wf_audio2_config_t c={0};wf_audio2_t a={0};
    c.firmware_elf=read_file(argv[1],&c.firmware_bytes);
    c.sfx_pack=read_file(argv[2],&c.sfx_bytes);
    c.workspace=0x06020000u;c.mail=&mail;c.music=&shared;
    c.platform=(wf_native_platform_t){sound,write16,now,wait_us,idle,enter,leave,barrier,NULL};
    c.worker_start=start;c.worker_quiescent=quiescent;
    wf_audio2_config_t bad=c;bad.platform.now_us=NULL;bad.platform.barrier=NULL;
    assert(!wf_audio2_init(&a,&bad)&&writes==0&&exited);
    wf_audio2_stop(&a); /* failed init must not leave callable null hooks */
    memset(&a,0,sizeof a);
    /* Corrupt payload must fail before SMPC, uploads or worker launch. */
    uint8_t *pack=(uint8_t *)c.sfx_pack;pack[c.sfx_bytes-2]^=1;
    assert(!wf_audio2_init(&a,&c)&&writes==0&&exited);
    pack[c.sfx_bytes-2]^=1;memset(&a,0,sizeof a);
    assert(wf_audio2_init(&a,&c)&&a.status.state==WF_AUDIO2_STARTING);
    assert(wf_audio2_command(&a,1)==WF_AUDIO2_NOT_READY);
    shared.state=WF_MUSIC_RUNNING;wf_audio2_service(&a);
    assert(a.status.state==WF_AUDIO2_RUNNING);
    for(unsigned command=1;command<=18;command++)assert(wf_audio2_command(&a,(uint16_t)command)==WF_AUDIO2_ACCEPTED);
    assert(shared.producer==18&&shared.commands[17]==18);
    assert(wf_audio2_command(&a,0x1f)==WF_AUDIO2_ACCEPTED);
    assert(shared.commands[18]==0x1f);
    unsigned before=shared.producer;
    assert(wf_audio2_command(&a,0x13)==WF_AUDIO2_UNSUPPORTED&&shared.producer==before);
    shared.producer=64;shared.consumer=0;
    assert(wf_audio2_command(&a,1)==WF_AUDIO2_BUSY&&shared.producer==64);
    shared.consumer=shared.producer;
    for(unsigned command=0x20;command<=0x81;command++){
        assert(wf_audio2_command(&a,(uint16_t)command)==WF_AUDIO2_ACCEPTED);
        assert(mail.command==WF_NATIVE_ARCADE_WORD&&mail.argument==command);
        assert(wf_audio2_command(&a,(uint16_t)command)==WF_AUDIO2_BUSY);
        tick+=7;mail.ack=mail.request;mail.result=command==0x20?WF_NATIVE_PRIORITY_REJECTED:WF_NATIVE_OK;
        wf_audio2_service(&a);
    }
    assert(a.status.priority_rejections==1&&a.status.max_mail_latency_us==7);
    assert(wf_audio2_command(&a,0)==WF_AUDIO2_ACCEPTED);
    assert(shared.commands[64&63]==0&&mail.argument==0);
    mail.ack=mail.request;mail.result=WF_NATIVE_OK;wf_audio2_service(&a);
    assert(wf_audio2_command(&a,0x20)==WF_AUDIO2_ACCEPTED);
    tick+=100001;wf_audio2_service(&a);
    assert(a.status.state==WF_AUDIO2_FAULT&&a.status.error==WF_AUDIO2_ERROR_MAIL_TIMEOUT);
    assert(shared.shutdown); /* requested real worker exit, not just API FAULT */
    assert(!wf_audio2_recover(&a)); /* still executing: no unsafe re-prepare */
    bad=c;bad.worker_quiescent=another_worker_idle;
    before=writes;assert(!wf_audio2_init(&a,&bad)&&writes==before);
    exited=true;assert(wf_audio2_recover(&a));
    shared.state=WF_MUSIC_RUNNING;wf_audio2_service(&a);wf_audio2_stop(&a);
    assert(shared.shutdown&&a.status.state==WF_AUDIO2_STOPPING);
    uint32_t stop_started=a.state_started_us;tick+=50;wf_audio2_stop(&a);
    assert(a.state_started_us==stop_started&&!a.stop_sent);
    wf_audio2_service(&a);mail.ack=mail.request;mail.result=WF_NATIVE_OK;
    shared.state=0;exited=true;wf_audio2_service(&a);
    assert(a.status.state==WF_AUDIO2_STOPPED);
    assert(wf_audio2_recover(&a));shared.state=WF_MUSIC_RUNNING;wf_audio2_service(&a);
    shared.worker_frame=65520;shared.dsp_progress_frame=65520;
    wf_audio2_vblank(&a,8);wf_audio2_service(&a);
    assert(a.status.state==WF_AUDIO2_RUNNING); /* healthy through frame wrap */
    wf_audio2_vblank(&a,104);wf_audio2_service(&a); /* 120 elapsed modulo16 */
    assert(a.status.state==WF_AUDIO2_FAULT&&a.status.error==WF_AUDIO2_ERROR_WORKER_STALLED&&shared.shutdown);
    exited=true;assert(wf_audio2_recover(&a));shared.state=WF_MUSIC_RUNNING;
    shared.worker_frame=120;wf_audio2_vblank(&a,120);wf_audio2_service(&a);
    assert(a.status.state==WF_AUDIO2_FAULT&&a.status.error==WF_AUDIO2_ERROR_DSP_STALLED&&shared.shutdown);
    puts("audio2_api: payload corruption,18music,98SFX,1F,stop,backpressure,priority,timeout,recovery PASS");
    free((void *)c.firmware_elf);free(pack);return 0;
}
