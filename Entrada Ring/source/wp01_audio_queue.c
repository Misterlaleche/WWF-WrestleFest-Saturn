#include "wp01_audio_queue.h"
#include <string.h>
static uint16_t get16(const uint8_t *p){return (uint16_t)(((uint16_t)p[0]<<8)|p[1]);}
static uint32_t get32(const uint8_t *p){return ((uint32_t)get16(p)<<16)|get16(p+2);}
static bool valid_write(const uint8_t *p){
    unsigned offset=get16(p);
    return offset<0x400U&&!(offset&1U)&&(offset&31U)<=0x16U;
}
bool wp01_audio_queue_bind(wp01_audio_queue_t *q,const void *data,uint32_t bytes,wp01_audio_write_fn write,void *context){
    memset(q,0,sizeof(*q));
    if(!data||bytes<32U||!write)return false;
    const uint8_t *p=data;
    if(get32(p)!=0x5750314eU||get32(p+4)!=1U)return false;
    uint32_t ni=get32(p+8),ne=get32(p+12),duration=get32(p+16);
    if(ni>1024U||ne>16384U||duration==0U||get32(p+20)>0x80000U)return false;
    if(get32(p+24)!=120093750U||get32(p+28)!=143U)return false;
    if(bytes!=32U+ni*4U+ne*8U)return false;
    const uint8_t *initial=p+32,*events=initial+ni*4U;
    for(uint32_t i=0;i<ni;i++)if(!valid_write(initial+i*4U))return false;
    uint32_t last=0;
    for(uint32_t i=0;i<ne;i++){
        const uint8_t *e=events+i*8U;uint32_t tick=get32(e);
        if(tick<last||tick>duration||!valid_write(e+4))return false;
        last=tick;
    }
    q->initial=initial;q->events=events;q->initial_count=ni;q->event_count=ne;
    q->duration=duration;q->write=write;q->context=context;
    return true;
}
void wp01_audio_queue_restart(wp01_audio_queue_t *q){
    q->next=0;q->elapsed=0;q->max_lateness=0;q->max_burst=0;
    for(uint32_t i=0;i<q->initial_count;i++){
        const uint8_t *p=q->initial+i*4U;
        q->write(q->context,get16(p),get16(p+2));
    }
}
uint32_t wp01_audio_queue_advance(wp01_audio_queue_t *q,uint32_t elapsed){
    if(elapsed<q->elapsed)return 0;
    q->elapsed=elapsed;uint32_t count=0;
    while(q->next<q->event_count){
        const uint8_t *p=q->events+q->next*8U;uint32_t tick=get32(p);
        if(tick>elapsed)break;
        uint32_t late=elapsed-tick;if(late>q->max_lateness)q->max_lateness=late;
        q->write(q->context,get16(p+4),get16(p+6));
        ++q->next;++count;
    }
    if(count>q->max_burst)q->max_burst=count;
    return count;
}
