#include "wf_native_wp24.h"
static bool read_source(uint32_t off,void *dst,uint32_t bytes,void *user)
{
    wf_native_wp24_t *b=user;
    return b->source.read_source_be &&
           b->source.read_source_be(off,dst,bytes,b->source.user);
}
static bool render(void *dst,uint32_t bytes,void *user)
{
    wf_native_wp24_t *b=user;
    return b->source.render_live_be &&
           b->source.render_live_be(dst,bytes,b->source.user);
}
static bool copy_half(unsigned half,const void *src,uint32_t bytes,void *user)
{
    wf_native_wp24_t *b=user;
    const uint8_t *p=src;
    wf_native_host_t *h=b->host;
    uint32_t n,addr;
    if(half>1 || !p || bytes!=WF24_MUSIC_HALF_BYTES ||
       !h->ready || h->poisoned || !h->ops.enter_owner(h->ops.user)) return false;
    addr=WF24_MUSIC_RING_OFFSET+half*WF24_MUSIC_HALF_BYTES;
    for(n=0;n<bytes;n+=2)
        h->ops.write16(addr+n,(uint16_t)(((uint16_t)p[n]<<8)|p[n+1]),h->ops.user);
    h->ops.barrier(h->ops.user);
    h->ops.leave_owner(h->ops.user);
    return true;
}
static bool prepare(const wf24_slot31_format_t *f,void *user)
{
    wf_native_wp24_t *b=user;
    if(!f || f->slot!=31 || f->sound_ram_offset!=0x60000 ||
       f->ring_samples!=32768 || f->lsa!=0 || f->lea!=32767 ||
       f->rate_hz!=44100 || f->channels!=1 || f->bits!=16 ||
       !f->signed_pcm || !f->big_endian || !f->centered_mono ||
       !f->unity_stream_gain) return false;
    return wf_native_host_command(b->host,WF_NATIVE_RING_PREPARE,0,NULL);
}
static bool start(void *user)
{
    return wf_native_host_command(((wf_native_wp24_t *)user)->host,
                                   WF_NATIVE_RING_START,0,NULL);
}
static bool stop(void *user)
{
    return wf_native_host_command(((wf_native_wp24_t *)user)->host,
                                   WF_NATIVE_RING_STOP,0,NULL);
}
static bool ca(unsigned *out,void *user)
{
    uint16_t value;
    if(!out || !wf_native_host_command(((wf_native_wp24_t *)user)->host,
                                       WF_NATIVE_RING_CA,0,&value) || value>7)
        return false;
    *out=value;
    return true;
}
bool wf_native_wp24_bind(wf_native_wp24_t *b,wf_native_host_t *h,
                         const wf24_stream_ops_t *source,wf24_stream_ops_t *out)
{
    if(!b || !h || !source || !out ||
       (!source->read_source_be && !source->render_live_be)) return false;
    b->host=h; b->source=*source;
    out->read_source_be=source->read_source_be ? read_source : NULL;
    out->render_live_be=source->render_live_be ? render : NULL;
    out->copy_half_be=copy_half;
    out->slot31_prepare_serialized=prepare;
    out->slot31_start_serialized=start;
    out->slot31_stop_serialized=stop;
    out->slot31_read_ca_serialized=ca;
    out->user=b;
    return true;
}
