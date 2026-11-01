#include "wf_native_host.h"

static bool in_span(uint32_t a,uint32_t n,uint32_t lo,uint32_t hi)
{
    return n && a>=lo && a<hi && n<=hi-a;
}
static uint32_t be32(const uint8_t *p)
{
    return ((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|
           ((uint32_t)p[2]<<8)|p[3];
}
bool wf_native_image_valid(const wf_native_image_t *im)
{
    unsigned i,j;
    bool vectors=false,wave=false;
    uint32_t pc=0;
    if(!im || !im->segments || im->count!=3) return false;
    for(i=0;i<im->count;++i) {
        const wf_native_segment_t *s=&im->segments[i];
        if((s->address|s->file_bytes|s->memory_bytes)&1u ||
           s->file_bytes>s->memory_bytes || (s->file_bytes && !s->data_be) ||
           !(in_span(s->address,s->memory_bytes,0,0xbc00) ||
             in_span(s->address,s->memory_bytes,0xc100,0x12400) ||
             in_span(s->address,s->memory_bytes,0x70000,0x71800))) return false;
        if(s->address==0) {
            if(s->file_bytes<1024 || be32(s->data_be)!=0x14000) return false;
            pc=be32(s->data_be+4);
            if((pc&1u) || pc<1024 || pc>=s->file_bytes) return false;
            vectors=true;
        }
        if((s->address==0x70000||s->address==0x10000) && (s->file_bytes==6144||(s->address==0x10000&&s->file_bytes==9216)) && s->memory_bytes==s->file_bytes)
            wave=true;
        for(j=0;j<i;++j) {
            const wf_native_segment_t *t=&im->segments[j];
            if(s->address<t->address+t->memory_bytes &&
               t->address<s->address+s->memory_bytes) return false;
        }
    }
    if(wave)for(i=0;i<im->count;i++)if(im->segments[i].address==0x10000){
        /* Resident SFX profile: PCM rings and BSS may not overlap code/wave. */
        for(j=0;j<im->count;j++){
            const wf_native_segment_t *s=&im->segments[j];
            if(s->address==0){if(s->memory_bytes>0x3000)return false;}
            else if(s->address==0xc100){if(s->memory_bytes>0x10000-0xc100)return false;}
            else if(s->address==0x10000){if((s->memory_bytes!=6144&&s->memory_bytes!=9216)||s->file_bytes!=s->memory_bytes)return false;}
            else return false;
        }
    }
    return vectors && wave;
}

static bool ops_valid(const wf_native_platform_t *o)
{
    return o && o->sound_enable && o->write16 && o->now_us && o->wait_us &&
           o->idle && o->enter_owner && o->leave_owner && o->barrier;
}
bool wf_native_host_init(wf_native_host_t *h,volatile wf_native_mail_t *mail,
                         const wf_native_platform_t *o)
{
    if(!h || !mail || !ops_valid(o)) return false;
    h->mail=mail; h->ops=*o; h->timeout_us=100000;
    h->ready=false; h->poisoned=false;
    return true;
}
static bool ready(const wf_native_host_t *h)
{
    return h->mail->state==WF_NATIVE_READY &&
           h->mail->magic_hi==0x5746 && h->mail->magic_lo==0x4e31 &&
           h->mail->abi==WF_NATIVE_ABI;
}
bool wf_native_host_boot(wf_native_host_t *h,const wf_native_image_t *im)
{
    unsigned i;
    uint32_t n,start;
    bool ok=false;
    wf_native_platform_t *o;
    if(!h || !h->mail || !ops_valid(&h->ops) ||
       !wf_native_image_valid(im)) return false;
    o=&h->ops;
    if(!o->enter_owner(o->user)) return false;
    h->ready=false; h->poisoned=true;
    if(!o->sound_enable(false,o->user)) goto done;
    o->wait_us(40,o->user);
    o->write16(0x100400,0x0200,o->user); /* MEM4MB=1, mute old slots during reset/copies */
    /* SNDOFF stops the68000, not the DSP. Disable every old RAM-write
     * operation before replacing executable RAM, then drain one DSP frame. */
    for(n=0;n<128;n++)o->write16(0x100804u+n*8u,0,o->user);
    o->barrier(o->user);o->wait_us(40,o->user);
    for(i=0;i<im->count;++i) {
        const wf_native_segment_t *s=&im->segments[i];
        for(n=0;n<s->memory_bytes;n+=2) {
            uint16_t v=n<s->file_bytes ?
                (uint16_t)(((uint16_t)s->data_be[n]<<8)|s->data_be[n+1]) : 0;
            o->write16(s->address+n,v,o->user);
        }
    }
    for(n=0;n<20;n+=2) o->write16(WF_NATIVE_COM_OFFSET+n,0,o->user);
    o->write16(0xc010,6631,o->user);
    o->write16(0xc012,(uint16_t)(int16_t)-2685,o->user);
    for(n=0;n<sizeof(*h->mail);n+=2)
        o->write16(WF_NATIVE_MAIL_OFFSET+n,0,o->user);
    o->barrier(o->user);
    if(!o->sound_enable(true,o->user)) goto done;
    start=o->now_us(o->user);
    while(!ready(h)) {
        if(h->mail->state==WF_NATIVE_FAULT ||
           (uint32_t)(o->now_us(o->user)-start)>=5000000u) goto done;
        o->idle(o->user);
    }
    o->write16(0x100400,0x020f,o->user); /* legacy firmware also needs host restore */
    o->barrier(o->user);
    h->ready=true; h->poisoned=false; ok=true;
done:
    o->leave_owner(o->user);
    return ok;
}
bool wf_native_host_command(wf_native_host_t *h,uint16_t cmd,
                            uint16_t arg,uint16_t *value)
{
    uint16_t seq;
    uint32_t start;
    bool ok=false;
    wf_native_platform_t *o;
    if(!h || !h->mail || !h->ready || h->poisoned || !h->timeout_us ||
       h->timeout_us>0x7fffffffu) return false;
    o=&h->ops;
    if(!o->enter_owner(o->user)) return false;
    if(!ready(h) || h->mail->request!=h->mail->ack) goto done;
    seq=(uint16_t)(h->mail->request+1u);
    h->mail->command=cmd;
    h->mail->argument=arg;
    o->barrier(o->user);
    h->mail->request=seq;
    o->barrier(o->user);
    start=o->now_us(o->user);
    while(h->mail->ack!=seq) {
        if(h->mail->state!=WF_NATIVE_READY ||
           (uint32_t)(o->now_us(o->user)-start)>=h->timeout_us) {
            h->poisoned=true;
            goto done;
        }
        o->idle(o->user);
    }
    o->barrier(o->user);
    if(value) *value=h->mail->value;
    ok=h->mail->result==WF_NATIVE_OK && ready(h);
done:
    o->leave_owner(o->user);
    return ok;
}
void wf_native_host_pcm_tick(wf_native_host_t *h)
{
    if(h && h->ready && !h->poisoned)
        h->ops.write16(WF_NATIVE_COM_OFFSET+4,1,h->ops.user);
}
