#include "wf_audio2.h"
#include "wf_native_elf.h"
#include "wf_scsp_sfx.h"
#include <string.h>

static uint32_t be32(const uint8_t *p)
{return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
bool wf_audio2_resource_valid(const uint8_t *p,size_t n)
{
    if(!wf_scsp_sfx_pack_valid(p,n))return false;
    uint32_t crc=0xffffffffu;
    for(size_t i=32;i<n;i++){
        crc^=p[i];
        for(unsigned b=0;b<8;b++)crc=(crc>>1)^(0xedb88320u&(0u-(crc&1u)));
    }
    return (crc^0xffffffffu)==be32(p+28);
}
static uint32_t now(wf_audio2_t *a){return a->config.platform.now_us(a->config.platform.user);}
static void barrier(wf_audio2_t *a){a->config.platform.barrier(a->config.platform.user);}
static void fail(wf_audio2_t *a,wf_audio2_error_t error)
{
    if(!a->status.error)a->status.error=error;
    a->status.state=WF_AUDIO2_FAULT;
    /* A public transport/startup fault must also let the worker leave. Merely
     * setting API status would make recover() wait forever on a healthy but
     * still running slave. An actually wedged worker still needs cold reset. */
    if(a->config.music){
        a->config.music->shutdown=1;
        if(a->config.platform.barrier)a->config.platform.barrier(a->config.platform.user);
    }
}
static bool mail_ready(wf_audio2_t *a)
{
    volatile wf_native_mail_t *m=a->config.mail;
    return !a->pending&&m->state==WF_NATIVE_READY&&m->request==m->ack;
}
static void post(wf_audio2_t *a,uint16_t command,uint16_t argument)
{
    volatile wf_native_mail_t *m=a->config.mail;
    a->pending_sequence=(uint16_t)(m->request+1u);
    m->command=command;m->argument=argument;barrier(a);
    a->mail_started_us=now(a);a->pending=true;
    m->request=a->pending_sequence;barrier(a);
}
bool wf_audio2_init(wf_audio2_t *a,const wf_audio2_config_t *c)
{
    wf_native_segment_t segments[3];wf_native_image_t image;wf_native_host_t prepared_host;
    if(!a||!c)return false;
    if(a->status.state==WF_AUDIO2_RUNNING||a->status.state==WF_AUDIO2_STARTING||
       a->status.state==WF_AUDIO2_STOPPING)return false;
    /* A replacement config cannot vouch for the old worker's ownership.
     * Check retained ownership before it can be erased or replaced. */
    if(a->status.state==WF_AUDIO2_FAULT&&a->config.worker_quiescent&&
       !a->config.worker_quiescent(a->config.worker_user))return false;
    if(!c->mail||!c->music||!c->worker_start||!c->worker_quiescent||
       !c->worker_quiescent(c->worker_user)){
        fail(a,WF_AUDIO2_ERROR_ARGUMENT);return false;
    }
    if(!wf_native_host_init(&prepared_host,c->mail,&c->platform)){
        fail(a,WF_AUDIO2_ERROR_ARGUMENT);return false;
    }
    /* Validate all input spans before taking the sound owner or stopping it. */
    if(!wf_audio2_resource_valid(c->sfx_pack,c->sfx_bytes)||
       !wf_native_elf_image(c->firmware_elf,c->firmware_bytes,segments,&image)){
        fail(a,WF_AUDIO2_ERROR_RESOURCE);return false;
    }
    /* The generic historical loader also accepts a 0x70000 sine profile.
     * That profile would collide with this resident SFX pack. Audio2 accepts
     * only its actual three-segment 9216-byte-wave contract. */
    if(segments[0].address!=0||segments[1].address!=0xc100u||
       segments[2].address!=0x10000u||segments[2].file_bytes!=9216u){
        fail(a,WF_AUDIO2_ERROR_RESOURCE);return false;
    }
    wf_audio2_config_t config=*c;
    memset(a,0,sizeof *a);a->config=config;a->status.api_version=WF_AUDIO2_API_VERSION;
    c=&a->config;
    a->host=prepared_host;
    if(!wf_native_host_boot(&a->host,&image)){
        fail(a,WF_AUDIO2_ERROR_BOOT);return false;
    }
    if(c->mail->reserved[2]!=WF_NATIVE_MUSIC_PROFILE){
        fail(a,WF_AUDIO2_ERROR_PROTOCOL);return false;
    }
    for(size_t i=0;i<c->sfx_bytes;i+=2)
        c->platform.write16(WF_SFX_PACK_BASE+(uint32_t)i,
            (uint16_t)((uint16_t)c->sfx_pack[i]<<8|c->sfx_pack[i+1]),c->platform.user);
    barrier(a);
    if(!wf_native_host_command(&a->host,WF_NATIVE_SFX_BIND,0,NULL)){
        fail(a,WF_AUDIO2_ERROR_BOOT);return false;
    }
    a->status.state=WF_AUDIO2_STARTING;a->state_started_us=now(a);
    if(!c->worker_start(c->workspace,c->worker_user)){
        fail(a,WF_AUDIO2_ERROR_WORKER_START);return false;
    }
    return true;
}
void wf_audio2_service(wf_audio2_t *a)
{
    if(!a||a->status.state==WF_AUDIO2_CLOSED||a->status.state==WF_AUDIO2_STOPPED)return;
    volatile wf_music_module_shared_t *s=a->config.music;
    volatile wf_native_mail_t *m=a->config.mail;
    if(!s||!m||!a->config.platform.now_us)return;
    uint32_t current=now(a);
    a->status.worker_state=s->state;a->status.worker_error=s->error;
    a->status.native_state=m->state;
    a->status.command_queue_used=(uint16_t)(s->producer-s->consumer);
    if(a->pending){
        if(m->ack==a->pending_sequence){
            barrier(a);a->pending=false;
            uint32_t latency=current-a->mail_started_us;
            if(latency>a->status.max_mail_latency_us)a->status.max_mail_latency_us=latency;
            a->status.last_native_result=m->result;
            if(m->result==WF_NATIVE_PRIORITY_REJECTED)++a->status.priority_rejections;
            else if(m->result!=WF_NATIVE_OK)fail(a,WF_AUDIO2_ERROR_NATIVE);
        }else if(current-a->mail_started_us>=a->host.timeout_us){
            a->host.poisoned=true;fail(a,WF_AUDIO2_ERROR_MAIL_TIMEOUT);
        }
    }
    a->status.mail_pending=a->pending;
    if(a->status.state==WF_AUDIO2_FAULT)return;
    if(m->state!=WF_NATIVE_READY){fail(a,WF_AUDIO2_ERROR_NATIVE);return;}
    if(a->status.state==WF_AUDIO2_STOPPING){
        if(!a->stop_sent&&mail_ready(a)){
            post(a,WF_NATIVE_SFX_STOP,0);a->stop_sent=true;
        }
        if(a->stop_sent&&!a->pending&&a->config.worker_quiescent(a->config.worker_user))
            a->status.state=WF_AUDIO2_STOPPED;
        else if(current-a->state_started_us>=5000000u)fail(a,WF_AUDIO2_ERROR_STOP_TIMEOUT);
        return;
    }
    if(s->state==WF_MUSIC_FAULT){fail(a,WF_AUDIO2_ERROR_WORKER);return;}
    if(a->status.state==WF_AUDIO2_STARTING){
        if(s->state==WF_MUSIC_RUNNING)a->status.state=WF_AUDIO2_RUNNING;
        else if(current-a->state_started_us>=5000000u)fail(a,WF_AUDIO2_ERROR_START_TIMEOUT);
    }
    if(a->status.state==WF_AUDIO2_RUNNING){
        /* Progress first, master stamp last: VBlank may interrupt this code
         * and the slave may then publish the newer stamp. Reading master
         * first could turn a healthy one-frame lead into unsigned age65535. */
        uint16_t worker_frame=s->worker_frame;
        uint16_t dsp_frame=s->dsp_progress_frame;
        uint16_t master_frame=s->master_frame;
        if(s->state!=WF_MUSIC_RUNNING)fail(a,WF_AUDIO2_ERROR_WORKER);
        else if((uint16_t)(master_frame-worker_frame)>=120u)
            fail(a,WF_AUDIO2_ERROR_WORKER_STALLED);
        else if((uint16_t)(master_frame-dsp_frame)>=120u)
            fail(a,WF_AUDIO2_ERROR_DSP_STALLED);
    }
    if(a->status.command_queue_used>64u)fail(a,WF_AUDIO2_ERROR_PROTOCOL);
}
wf_audio2_result_t wf_audio2_command(wf_audio2_t *a,uint16_t word)
{
    if(!a)return WF_AUDIO2_FAILED;
    wf_audio2_service(a);
    if(a->status.state==WF_AUDIO2_FAULT)return WF_AUDIO2_FAILED;
    if(a->status.state!=WF_AUDIO2_RUNNING)return WF_AUDIO2_NOT_READY;
    unsigned command=word&255u;
    bool music=command<=0x12u||command==0x1fu;
    bool effect=command==0||(command>=0x20u&&command<=0x81u);
    if(!music&&!effect)return WF_AUDIO2_UNSUPPORTED;
    volatile wf_music_module_shared_t *s=a->config.music;
    /* Check every destination before publishing any part of command 00. */
    if(a->pending||!mail_ready(a)||(music&&(uint16_t)(s->producer-s->consumer)>=64u))
        return WF_AUDIO2_BUSY;
    if(music){
        uint16_t p=s->producer;s->commands[p&63u]=(uint16_t)command;
        barrier(a);s->producer=(uint16_t)(p+1u);barrier(a);
    }
    if(effect)post(a,WF_NATIVE_ARCADE_WORD,word);
    a->status.last_word=word;++a->status.accepted_commands;
    return WF_AUDIO2_ACCEPTED;
}
void wf_audio2_vblank(wf_audio2_t *a,uint16_t frame)
{if(a&&a->config.music)a->config.music->master_frame=frame;}
void wf_audio2_status(const wf_audio2_t *a,wf_audio2_status_t *out)
{if(a&&out)*out=a->status;}
void wf_audio2_stop(wf_audio2_t *a)
{
    if(!a||!a->config.music||a->status.state==WF_AUDIO2_CLOSED||a->status.state==WF_AUDIO2_STOPPED||
       a->status.state==WF_AUDIO2_STOPPING)return;
    a->config.music->shutdown=1;barrier(a);
    a->status.state=WF_AUDIO2_STOPPING;a->state_started_us=now(a);a->stop_sent=false;
}
bool wf_audio2_recover(wf_audio2_t *a)
{
    if(!a||(a->status.state!=WF_AUDIO2_FAULT&&a->status.state!=WF_AUDIO2_STOPPED)||
       !a->config.worker_quiescent||!a->config.worker_quiescent(a->config.worker_user))return false;
    wf_audio2_config_t c=a->config;
    /* A fresh SNDOFF/firmware boot resolves even an unknown timed-out mailbox
     * outcome. No old queue or musical timeline is resumed. */
    a->status.state=WF_AUDIO2_CLOSED;return wf_audio2_init(a,&c);
}
