/* Host-only MSM6295 digital model used by the board-reference experiment.
 * Equations/protocol checked against pinned MAME5c60f61d… okim6295.cpp and
 * okiadpcm.cpp. No analog filter is modeled. This is never a Saturn renderer.
 */
#ifndef WF_REFERENCE_OKI_H
#define WF_REFERENCE_OKI_H
#include <stdint.h>
#include <stddef.h>
#include <string.h>
typedef struct {
    uint32_t start,position,count;
    int signal,index,current;
    unsigned volume,playing;
} reference_oki_voice_t;
typedef struct {
    const uint8_t *rom;size_t bytes;
    reference_oki_voice_t voices[4];
    unsigned bank;int pending,fault;
    uint64_t bank_changes_while_active,ignored_busy_starts,invalid_starts;
} reference_oki_t;
static const int reference_oki_steps[49]={16,17,19,21,23,25,28,31,34,37,41,45,50,55,60,66,73,80,88,97,107,118,130,143,157,173,190,209,230,253,279,307,337,371,408,449,494,544,598,658,724,796,876,963,1060,1166,1282,1411,1552};
static const int reference_oki_levels[16]={32,22,16,11,8,6,4,3,2,0,0,0,0,0,0,0};
static void reference_oki_reset(reference_oki_t *o,const uint8_t *rom,size_t bytes)
{memset(o,0,sizeof *o);o->rom=rom;o->bytes=bytes;o->pending=-1;o->fault=bytes!=0x80000;}
static uint8_t reference_oki_status(const reference_oki_t *o)
{uint8_t value=0xf0;for(unsigned v=0;v<4;v++)if(o->voices[v].playing)value|=1u<<v;return value;}
static void reference_oki_bank(reference_oki_t *o,unsigned bank)
{if(o->bank!=(bank&1)&&(reference_oki_status(o)&15))o->bank_changes_while_active++;o->bank=bank&1;}
static unsigned reference_oki_read(reference_oki_t *o,unsigned address)
{size_t at=(size_t)o->bank*0x40000+(address&0x3ffff);if(at>=o->bytes){o->fault=1;return 0;}return o->rom[at];}
static void reference_oki_write(reference_oki_t *o,unsigned value)
{
    if(o->pending>=0){
        for(unsigned v=0;v<4;v++)if(value&(0x10u<<v)){
            reference_oki_voice_t *p=&o->voices[v];
            if(p->playing){o->ignored_busy_starts++;continue;}
            unsigned base=(unsigned)o->pending*8;
            unsigned start=(reference_oki_read(o,base)<<16)|(reference_oki_read(o,base+1)<<8)|reference_oki_read(o,base+2);
            unsigned end=(reference_oki_read(o,base+3)<<16)|(reference_oki_read(o,base+4)<<8)|reference_oki_read(o,base+5);
            start&=0x3ffff;end&=0x3ffff;
            if(start>=end){o->invalid_starts++;continue;}
            *p=(reference_oki_voice_t){.start=start,.count=2*(end-start+1),.volume=value&15,.playing=1};
        }
        o->pending=-1;
    }else if(value&0x80)o->pending=value&0x7f;
    else for(unsigned v=0;v<4;v++)if(value&(8u<<v)){o->voices[v].playing=0;o->voices[v].current=0;}
}
static void reference_oki_tick(reference_oki_t *o)
{
    static const int shift[8]={-1,-1,-1,-1,2,4,6,8};
    for(unsigned v=0;v<4;v++){
        reference_oki_voice_t *p=&o->voices[v];if(!p->playing){p->current=0;continue;}
        unsigned byte=reference_oki_read(o,p->start+p->position/2),n=(p->position&1)?byte&15:byte>>4;
        int step=reference_oki_steps[p->index];
        int delta=step/8+((n&1)?step/4:0)+((n&2)?step/2:0)+((n&4)?step:0);
        p->signal+=(n&8)?-delta:delta;
        if(p->signal>2047)p->signal=2047;
        if(p->signal< -2048)p->signal=-2048;
        p->index+=shift[n&7];if(p->index<0)p->index=0;if(p->index>48)p->index=48;
        p->current=p->signal*16;if(++p->position>=p->count)p->playing=0;
    }
}
static int reference_oki_mix(const reference_oki_t *o)
{int sum=0;for(unsigned v=0;v<4;v++)sum+=o->voices[v].current*reference_oki_levels[o->voices[v].volume]/32;return sum;}
#endif
