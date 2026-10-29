#include "wp0_audio_player.h"
#include <string.h>

static bool fail(wp0_audio_player_t *p,wp0_audio_error_t error)
{
    if(p->state==WP0_AUDIO_PLAYING)p->ops.key_off(p->ops.user);
    p->error=error;p->state=WP0_AUDIO_ERROR;return false;
}
static bool queue_block(wp0_audio_player_t *p)
{
    unsigned n=0;
    if(p->stream.next_block<p->stream.block_count){
        if(!wp0_rice_next(&p->stream,p->pcm,&n))return fail(p,WP0_AUDIO_BAD_SOURCE);
        p->decoded_blocks++;
    }
    if(n<WP0_PCM_BLOCK)memset(p->pcm+n,0,(WP0_PCM_BLOCK-n)*sizeof p->pcm[0]);
    const unsigned offset=p->queued_samples&(WP0_AUDIO_RING_SAMPLES-1u);
    if(!p->ops.write_pcm(offset,p->pcm,WP0_PCM_BLOCK,p->ops.user))
        return fail(p,WP0_AUDIO_BAD_PLATFORM);
    p->queued_samples+=WP0_PCM_BLOCK;return true;
}
bool wp0_audio_prepare(wp0_audio_player_t *p,wp0_audio_read_fn read,
                      void *u,uint32_t bytes,const wp0_audio_ops_t *ops)
{
    if(!p||!ops||!ops->write_pcm||!ops->read_region||!ops->key_on||!ops->key_off)
        return false;
    memset(p,0,sizeof *p);p->ops=*ops;
    if(!wp0_rice_open(&p->stream,read,u,bytes))return fail(p,WP0_AUDIO_BAD_SOURCE);
    while(p->queued_samples<WP0_AUDIO_RING_SAMPLES)
        if(!queue_block(p))return false;
    p->state=WP0_AUDIO_PREPARED;return true;
}
bool wp0_audio_start(wp0_audio_player_t *p)
{
    if(!p||p->state!=WP0_AUDIO_PREPARED)return false;
    if(!p->ops.key_on(p->ops.user))return fail(p,WP0_AUDIO_BAD_PLATFORM);
    p->previous_region=0;p->consumed_lower_bound=0;p->state=WP0_AUDIO_PLAYING;
    return true;
}
void wp0_audio_stop(wp0_audio_player_t *p)
{
    if(!p)return;
    if(p->state==WP0_AUDIO_PLAYING)p->ops.key_off(p->ops.user);
    if(p->state!=WP0_AUDIO_ERROR)p->state=WP0_AUDIO_STOPPED;
}
bool wp0_audio_service(wp0_audio_player_t *p,uint32_t us)
{
    if(!p)return false;
    if(p->state!=WP0_AUDIO_PLAYING)return p->state!=WP0_AUDIO_ERROR;
    p->service_calls++;
    p->last_service_us=us;
    if(us>p->max_service_us)p->max_service_us=us;
    p->last_region=UINT32_MAX;p->last_advance=UINT32_MAX;
    /* Fail before a half-ring gap can hide repeated monitor values. */
    if(us>=371519u)return fail(p,WP0_AUDIO_SERVICE_GAP);
    const unsigned region=p->ops.read_region(p->ops.user);
    p->last_region=region;
    if(region>=WP0_AUDIO_RING_REGIONS)return fail(p,WP0_AUDIO_BAD_PLATFORM);
    const unsigned advance=(region-p->previous_region)&(WP0_AUDIO_RING_REGIONS-1u);
    p->last_advance=advance;
    if(advance>1u)return fail(p,WP0_AUDIO_SERVICE_GAP);
    p->previous_region=region;
    p->consumed_lower_bound+=advance*WP0_AUDIO_REGION_SAMPLES;
    if(p->consumed_lower_bound>=p->stream.total_samples){wp0_audio_stop(p);return true;}
    /* Coarse CA gives a lower bound. Keep its complete active region intact. */
    if(p->queued_samples<p->consumed_lower_bound+WP0_AUDIO_REGION_SAMPLES)
        return fail(p,WP0_AUDIO_UNDERRUN);
    if(p->queued_samples+WP0_PCM_BLOCK<=p->consumed_lower_bound+WP0_AUDIO_RING_SAMPLES)
        return queue_block(p);
    return true;
}
