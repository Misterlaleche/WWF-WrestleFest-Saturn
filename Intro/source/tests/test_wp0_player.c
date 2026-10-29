#include "wp0_audio_player.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct {
    uint8_t *packed,*raw;size_t packed_bytes,raw_bytes;
    int16_t ring[WP0_AUDIO_RING_SAMPLES];
    uint32_t played;unsigned starts,stops,writes;bool running;
} model_t;
static bool read_source(uint32_t off,void *dst,uint32_t n,void *user)
{
    model_t *m=user;if(off>m->packed_bytes||n>m->packed_bytes-off)return false;
    memcpy(dst,m->packed+off,n);return true;
}
static bool write_ring(unsigned off,const int16_t *src,unsigned n,void *user)
{
    model_t *m=user;assert(off+n<=WP0_AUDIO_RING_SAMPLES);
    if(m->running){
        unsigned active=(m->played%WP0_AUDIO_RING_SAMPLES)/WP0_AUDIO_REGION_SAMPLES;
        assert(off/WP0_AUDIO_REGION_SAMPLES!=active);
    }
    memcpy(m->ring+off,src,n*sizeof *src);m->writes++;return true;
}
static unsigned region(void *user)
{model_t *m=user;return (m->played%WP0_AUDIO_RING_SAMPLES)/WP0_AUDIO_REGION_SAMPLES;}
static bool start(void *user)
{model_t *m=user;m->starts++;m->running=true;return true;}
static void stop(void *user)
{model_t *m=user;m->stops++;m->running=false;}
static uint8_t *load(const char *path,size_t *size)
{
    FILE *f=fopen(path,"rb");assert(f);assert(!fseek(f,0,SEEK_END));
    long n=ftell(f);assert(n>0);rewind(f);uint8_t *b=malloc((size_t)n);assert(b);
    assert(fread(b,1,(size_t)n,f)==(size_t)n);fclose(f);*size=(size_t)n;return b;
}
int main(int argc,char **argv)
{
    assert(argc==3);model_t *m=calloc(1,sizeof *m);assert(m);
    m->packed=load(argv[1],&m->packed_bytes);m->raw=load(argv[2],&m->raw_bytes);
    wp0_audio_ops_t ops={write_ring,region,start,stop,m};wp0_audio_player_t p;
    assert(wp0_audio_prepare(&p,read_source,m,(uint32_t)m->packed_bytes,&ops));
    assert(m->writes==32&&p.queued_samples==WP0_AUDIO_RING_SAMPLES);
    assert(wp0_audio_start(&p));assert(!wp0_audio_start(&p));assert(m->starts==1);
    unsigned frames=0;
    while(p.state==WP0_AUDIO_PLAYING){
        /*59.94-ish host sequence; every emitted sample is checked. */
        unsigned consume=735u+(frames%10u==0u);
        for(unsigned i=0;i<consume;i++){
            int16_t got=m->ring[m->played%WP0_AUDIO_RING_SAMPLES];
            int16_t want=m->played<p.stream.total_samples?
                (int16_t)(((unsigned)m->raw[m->played*2u]<<8)|m->raw[m->played*2u+1u]):0;
            assert(got==want);m->played++;
        }
        unsigned before=p.stream.next_block,writes=m->writes;
        assert(wp0_audio_service(&p,16683));
        assert(p.stream.next_block-before<=1&&m->writes-writes<=1);
        assert(++frames<2500);
    }
    assert(p.state==WP0_AUDIO_STOPPED&&m->stops==1);
    assert(p.stream.decoded_samples==p.stream.total_samples);
    assert(m->played>=p.stream.total_samples);
    wp0_audio_stop(&p);assert(m->stops==1);
    /* A gap large enough to conceal a complete ring wrap fails loudly. */
    m->played=0;
    assert(wp0_audio_prepare(&p,read_source,m,(uint32_t)m->packed_bytes,&ops));
    assert(wp0_audio_start(&p));
    assert(!wp0_audio_service(&p,800000));
    assert(p.state==WP0_AUDIO_ERROR&&p.error==WP0_AUDIO_SERVICE_GAP&&!m->running);
    printf("ring playback PASS: %u frames, every emitted sample exact; oneblock/call; no active-region overwrite\n",frames);
    free(m->packed);free(m->raw);free(m);return 0;
}
