#include "wf_scsp_sfx.h"
static void work_count(wf_scsp_sfx_t *,unsigned);
bool wf_scsp_sfx_configure_pcm(wf_scsp_sfx_t *s,unsigned stride,unsigned guard,wf_sfx_pcm_control_fn fn)
{
    if(!s||s->bound||stride!=8194||guard>33||!guard||!fn)return false;
    s->pcm_stride=stride;s->start_guard=guard;s->pcm_control=fn;return true;
}
static uint16_t be16(const uint8_t *p){return (uint16_t)((uint16_t)p[0]*256u+p[1]);}
static uint32_t be32(const uint8_t *p){return (uint32_t)be16(p)*65536u+be16(p+2);}
#ifndef WF_SFX_FAST_68000
static const int16_t steps[49]={16,17,19,21,23,25,28,31,34,37,41,45,50,55,60,66,73,80,88,97,107,118,130,143,157,173,190,209,230,253,279,307,337,371,408,449,494,544,598,658,724,796,876,963,1060,1166,1282,1411,1552};
static const int8_t shifts[8]={-1,-1,-1,-1,2,4,6,8};
int16_t wf_scsp_sfx_decode(wf_sfx_decoder_t *d,const uint8_t *p)
{
    int nibble,step,delta,signal,index;
    if(!d->remaining)return 0;
    nibble=d->low?(p[d->offset++]&15):(p[d->offset]>>4);
    d->low^=1u;--d->remaining;
    step=steps[d->index];delta=step>>3;
    if(nibble&1)delta+=step>>2;
    if(nibble&2)delta+=step>>1;
    if(nibble&4)delta+=step;
    signal=d->signal+((nibble&8)?-delta:delta);
    if(signal>2047)signal=2047;
    if(signal< -2048)signal= -2048;
    index=d->index+shifts[nibble&7];
    if(index>48)index=48;
    if(index<0)index=0;
    d->signal=(int16_t)signal;d->index=(int16_t)index;
    return (int16_t)(signal*16);
}
#else
extern void wf_sfx_decode_block(wf_sfx_decoder_t *,const uint8_t *,volatile uint16_t *,unsigned);
_Static_assert(sizeof(wf_sfx_decoder_t)==14,"assembly decoder layout");
_Static_assert(offsetof(wf_sfx_decoder_t,low)==12,"assembly decoder layout");
int16_t wf_scsp_sfx_decode(wf_sfx_decoder_t *d,const uint8_t *p)
{
    uint16_t out;wf_sfx_decode_block(d,p,&out,1);return (int16_t)out;
}
#endif
/* TL bit4 halves amplitude (SEGA SCSP manual table4.18).
 * Nearest TL to OKI amplitudes {32,22,16,11,8,6,4,3,2}/32. */
static const uint8_t levels[9]={0,9,16,25,32,39,48,55,64};
bool wf_scsp_sfx_pack_valid(const uint8_t *p,size_t n)
{
    unsigned i,j;uint32_t samples,commands,events,raw;
    if(!p||n<32||n>WF_SFX_PACK_CAPACITY||(n&1u)||
       be32(p)!=0x57465358u||be16(p+4)!=1||be16(p+6)!=116||be32(p+8)!=n)return false;
    samples=be32(p+12);commands=be32(p+16);events=be32(p+20);raw=be32(p+24);
    if(samples!=32||commands!=960||events!=1548||raw<events||raw>n||(raw-events)%4u)return false;
    for(i=0;i<116;i++){
        const uint8_t *e=p+samples+i*8u;
        uint32_t a=be32(e),length=be16(e+4);
        if(i==0||i==74){if(a||length||be16(e+6))return false;continue;}
        if(!length||a<raw||a>n||length>n-a||!be16(e+6))return false;
    }
    for(i=0;i<98;i++){
        const uint8_t *c=p+commands+i*6u;uint32_t off=be16(c+4);
        if(c[1]>2||c[2]>9||c[3]>1||(off&3u)||off>raw-events||c[2]*4u>raw-events-off)return false;
        for(j=0;j<c[2];j++){
            const uint8_t *e=p+events+off+j*4u;
            if(!e[0]||e[0]>(c[1]==1?41u:(c[1]==0?73u:19u))||e[2]>15||e[3]>3)return false;
        }
    }
    return true; /* CRC is verified by the SH2 uploader before BIND. */
}
void wf_scsp_sfx_init(wf_scsp_sfx_t *s,wf_sfx_write16_fn w,void *u)
{
    uint8_t *p=(uint8_t *)s;size_t n;
    for(n=0;n<sizeof *s;n++)p[n]=0;
    s->write16=w;s->user=u;s->pcm_stride=8192;s->start_guard=33;
}
static uint32_t reg(unsigned v){return 0x100000u+(WF_SFX_FIRST_SLOT+v)*32u;}
static bool put(wf_scsp_sfx_t *s,uint32_t a,uint16_t v)
{
    if(!s->write16||!s->write16(a,v,s->user)){s->fault=1;return false;}return true;
}
void wf_scsp_sfx_stop(wf_scsp_sfx_t *s,unsigned v)
{
    if(v>=4)return;
    s->pcm[v].playing=0;s->pcm[v].suppress=0;
    if(s->pcm_control){if(!s->pcm_control(v,false,0,s->user))s->fault=1;return;}
    (void)put(s,reg(v)+12,0x02ff); /* STWINH; TL mute before immediate reuse */
    (void)put(s,reg(v),0x1020); /* PCM16, forward loop, KYONB=0, global execute */
}
void wf_scsp_sfx_stop_all(wf_scsp_sfx_t *s)
{
    unsigned v;for(v=0;v<4;v++)wf_scsp_sfx_stop(s,v);
    for(v=0;v<3;v++)s->sequence[v].active=0;
}
bool wf_scsp_sfx_bind(wf_scsp_sfx_t *s,const uint8_t *p,size_t n)
{
    if(!s||!s->write16)return false;
    wf_scsp_sfx_stop_all(s);s->bound=0;
    if(!wf_scsp_sfx_pack_valid(p,n))return false;
    s->pack=p;s->bytes=(uint32_t)n;
    s->samples=be32(p+12);s->commands=be32(p+16);s->events=be32(p+20);s->raw=be32(p+24);
    s->bank=0;s->bound=1;return !s->fault;
}
static void fill(wf_scsp_sfx_t *s,unsigned v,unsigned count)
{
    wf_scsp_sfx_pcm_t *p=&s->pcm[v];
    unsigned boundary=4096u-(p->write_at&4095u);
    if(count>boundary)count=boundary;
#ifdef WF_SFX_FAST_68000
    if(count>p->write_end-p->write_at)count=(unsigned)(p->write_end-p->write_at);
    if(count&&!s->fault){
        uint32_t base=WF_SFX_PCM_BASE+v*s->pcm_stride;
        uint32_t a=base+(p->write_at&4095u)*2u;
        wf_sfx_decode_block(&p->decoder,s->pack,(volatile uint16_t *)(uintptr_t)a,count);
        if(s->pcm_stride==8194&&!(p->write_at&4095u))
            *(volatile uint16_t *)(uintptr_t)(base+8192u)=*(volatile uint16_t *)(uintptr_t)base;
        p->write_at+=count;
    }
#else
    while(count--&&p->write_at<p->write_end&&!s->fault){
        int16_t sample=wf_scsp_sfx_decode(&p->decoder,s->pack);
        uint32_t base=WF_SFX_PCM_BASE+v*s->pcm_stride;
        if(!put(s,base+(p->write_at&4095u)*2u,(uint16_t)sample))break;
        if(s->pcm_stride==8194&&!(p->write_at&4095u)&&!put(s,base+8192u,(uint16_t)sample))break;
        ++p->write_at;
    }
#endif
    if(p->write_at==p->write_end)p->initial_fill=0;
}
bool wf_scsp_sfx_prepare_step(const wf_scsp_sfx_t *s,wf_sfx_prepared_t *prepared,unsigned bank,unsigned id,uint16_t *filled,unsigned budget)
{
    const uint8_t *e;
    if(!s||!s->bound||s->fault||!prepared||!filled||*filled>256u||!budget||bank>1||!id||id>(bank?41u:73u))return false;
    if(!*filled){
        e=s->pack+s->samples+(bank*74u+id)*8u;
        prepared->decoder=(wf_sfx_decoder_t){0};
        prepared->decoder.offset=be32(e);prepared->decoder.remaining=be16(e+4)*2u;
        prepared->total_frames=prepared->decoder.remaining;prepared->pack=s->pack;
        prepared->bank=(uint8_t)bank;prepared->id=(uint8_t)id;
    }else if(prepared->pack!=s->pack||prepared->bank!=bank||prepared->id!=id)return false;
    if(budget>256u-*filled)budget=256u-*filled;
#ifdef WF_SFX_FAST_68000
    wf_sfx_decode_block(&prepared->decoder,s->pack,(volatile uint16_t *)(prepared->samples+*filled),budget);
#else
    for(unsigned n=0;n<budget;n++)prepared->samples[*filled+n]=wf_scsp_sfx_decode(&prepared->decoder,s->pack);
#endif
    *filled=(uint16_t)(*filled+budget);
    return true;
}
bool wf_scsp_sfx_prepare(const wf_scsp_sfx_t *s,wf_sfx_prepared_t *prepared,unsigned bank,unsigned id)
{
    uint16_t filled=0;
    return wf_scsp_sfx_prepare_step(s,prepared,bank,id,&filled,256);
}
static bool start(wf_scsp_sfx_t *s,unsigned v,unsigned bank,unsigned id,unsigned vol,bool suppress,const wf_sfx_prepared_t *prepared)
{
    wf_scsp_sfx_pcm_t *p;const uint8_t *e;unsigned i;uint32_t base;
    uint16_t words[12]={0x20,0,0,4096,31,31,0x200,0,0x69ce,0,0,0xe000};
    if(!s||!s->bound||s->fault||v>=4||bank>1||!id||id>(bank?41u:73u)||vol>15)return false;
    if(prepared&&(prepared->pack!=s->pack||prepared->bank!=bank||prepared->id!=id))return false;
    p=&s->pcm[v];
    if(p->playing&&p->suppress){p->suppress=(uint8_t)suppress;return true;}
    /* Refill other voices before the new decode. The launch buffer must cover
     * a delayed music event or one long SCSP service iteration. */
    if(!prepared)work_count(s,32);
    wf_scsp_sfx_stop(s,v);e=s->pack+s->samples+(bank*74u+id)*8u;
    p->decoder=(wf_sfx_decoder_t){0};p->decoder.offset=be32(e);p->decoder.remaining=be16(e+4)*2u;
    p->total_frames=p->decoder.remaining;p->played=p->phase=p->write_at=0;
    p->write_end=4096;p->initial_fill=1;
    p->sample_id=(uint8_t)id;p->bank=(uint8_t)bank;p->volume=(uint8_t)vol;
    p->suppress=(uint8_t)suppress;
    if(prepared){
        p->decoder=prepared->decoder;
        base=WF_SFX_PCM_BASE+v*s->pcm_stride;
#ifdef WF_SFX_FAST_68000
        volatile uint16_t *target=(volatile uint16_t *)(uintptr_t)base;
        const int16_t *sample=prepared->samples,*end=sample+256;
        *target++=(uint16_t)*sample++;
        if(s->pcm_stride==8194)*(volatile uint16_t *)(uintptr_t)(base+8192u)=(uint16_t)prepared->samples[0];
        while(sample!=end)*target++=(uint16_t)*sample++;
#else
        for(i=0;i<256;i++){
            if(!put(s,base+i*2u,(uint16_t)prepared->samples[i]))return false;
            if(!i&&s->pcm_stride==8194&&!put(s,base+8192u,(uint16_t)prepared->samples[0]))return false;
        }
#endif
        p->write_at=256;
    }else fill(s,v,256);
    if(s->pcm_control){
        p->last_frame=s->now?s->now(s->user)+s->start_guard:s->frames;
        if(!s->pcm_control(v,true,vol,s->user)){s->fault=1;return false;}
        p->playing=1;return !s->fault;
    }
    base=WF_SFX_PCM_BASE+v*8192u;words[1]=(uint16_t)base;
    words[6]=(uint16_t)(0x200u|(vol<9?levels[vol]:255u));
    for(i=0;i<12;i++)if(!put(s,reg(v)+i*2u,words[i]))return false;
    /* Account for prefill before KEYON. CA is quantized to ~32 Fs; the
     * extra33 Fs keeps our cursor conservative, so sample-end cannot be early. */
    p->last_frame=s->now?s->now(s->user)+s->start_guard:s->frames;
    if(!put(s,reg(v),0x1820))return false;
    p->playing=1;return true;
}
bool wf_scsp_sfx_oki_start(wf_scsp_sfx_t *s,unsigned v,unsigned bank,unsigned id,unsigned vol,bool suppress)
{
    return start(s,v,bank,id,vol,suppress,NULL);
}
bool wf_scsp_sfx_oki_start_prepared(wf_scsp_sfx_t *s,unsigned v,unsigned bank,unsigned id,unsigned vol,bool suppress,const wf_sfx_prepared_t *prepared)
{
    if(!prepared)return false;
    return start(s,v,bank,id,vol,suppress,prepared);
}
static const uint8_t *command(const wf_scsp_sfx_t *s,unsigned c){return s->pack+s->commands+(c-0x20u)*6u;}
static void fire(wf_scsp_sfx_t *s,unsigned v,const uint8_t *c)
{
    wf_scsp_sfx_sequence_t *q=&s->sequence[v];
    const uint8_t *e=s->pack+s->events+be16(c+4)+q->event_index*4u;
    if(!wf_scsp_sfx_oki_start(s,v,q->bank,e[0],e[2],(e[3]&1u)!=0))s->fault=1;
    q->ticks_remaining=e[1];
}
int wf_scsp_sfx_command(wf_scsp_sfx_t *s,uint8_t cmd)
{
    const uint8_t *c;unsigned v,best=0;
    if(!s||!s->bound||s->fault||cmd<0x20||cmd>0x81)return -2;
    c=command(s,cmd);if(!c[2])return 3;
    for(v=0;v<3;v++)if(!s->sequence[v].active)break;
    if(v==3){
        for(v=1;v<3;v++)if(s->sequence[v].priority<s->sequence[best].priority)best=v;
        if(c[0]<s->sequence[best].priority)return -1;
        v=best;wf_scsp_sfx_stop(s,v);
    }
    if(c[1]<2)s->bank=c[1];
    s->sequence[v]=(wf_scsp_sfx_sequence_t){1,cmd,0,0,c[0],s->bank};
    fire(s,v,c);return s->fault?-2:(int)v;
}
static void tick(wf_scsp_sfx_t *s)
{
    unsigned v;++s->ticks;
    for(v=0;v<3;v++){
        wf_scsp_sfx_sequence_t *q=&s->sequence[v];const uint8_t *c;
        if(!q->active)continue;
        c=command(s,q->command);
        if(q->ticks_remaining&&--q->ticks_remaining)continue;
        if(++q->event_index>=c[2]){if(c[3])wf_scsp_sfx_stop(s,v);q->active=0;}
        else fire(s,v,c);
    }
}
static void underrun(wf_scsp_sfx_t *s,unsigned voice)
{
    /* An exhausted PCM voice is audible, but it is not a corrupt sound
     * driver. Stop that voice before it reads unwritten samples and keep the
     * firmware, the other voices, and the game running. */
    ++s->underruns;wf_scsp_sfx_stop(s,voice);
}
void wf_scsp_sfx_advance(wf_scsp_sfx_t *s,unsigned n)
{
    unsigned v;
    if(!s||s->fault)return;
    s->frames+=n;
    if(!s->bound)return;
    if(n>512){
        /* No audio data can underrun while every PCM voice and delayed
         * sequence is idle. This happens during the initial pack bind. */
        for(v=0;v<4;v++)if(s->pcm[v].playing){
            ++s->underruns;wf_scsp_sfx_stop_all(s);s->timer_phase=0;return;
        }
        for(v=0;v<3;v++)if(s->sequence[v].active){
            ++s->underruns;wf_scsp_sfx_stop_all(s);s->timer_phase=0;return;
        }
        s->timer_phase=0;
        return;
    }
    for(v=0;v<4;v++){
        wf_scsp_sfx_pcm_t *p=&s->pcm[v];uint32_t safe_end,elapsed;
        if(!p->playing)continue;
        if((int32_t)(s->frames-p->last_frame)<0)continue;
        elapsed=s->frames-p->last_frame;p->last_frame=s->frames;
        p->phase+=elapsed*1486u;p->played+=p->phase>>13;p->phase&=8191u;
        if(p->played>=p->total_frames){wf_scsp_sfx_stop(s,v);continue;}
        if(p->played+(p->initial_fill?8u:64u)>=p->write_at){underrun(s,v);continue;}
        /* Continuous producer: do not wait for a half-ring batch. Frame N
         * may replace N-4096 only after the consumer passed it by 64 frames.
         * Every target is block aligned, so the fast decoder never crosses
         * the physical ring boundary in one write. */
        safe_end=(p->played+4096u-64u)&~31u;
        if(!p->initial_fill&&safe_end>p->write_end)p->write_end=safe_end;
    }
    s->timer_phase+=n*3579545u;
    while(s->timer_phase>=714067200u){s->timer_phase-=714067200u;tick(s);}
}
static void work_count(wf_scsp_sfx_t *s,unsigned count)
{
    unsigned v;if(!s||!s->bound||s->fault)return;
    for(unsigned n=0;n<4;n++){
        v=s->work_voice;s->work_voice=(uint8_t)((v+1u)&3u);
        if(s->pcm[v].playing&&s->pcm[v].write_at<s->pcm[v].write_end){fill(s,v,count);return;}
    }
}
void wf_scsp_sfx_work(wf_scsp_sfx_t *s){work_count(s,128);}
