#include "wp01_oki.h"
#include <string.h>

enum { KEYON = 0x0800, KEYEXEC = 0x1000, STWINH = 0x0200, SDIR = 0x0100 };

/* Original OKI amplitude numerators are 32,22,16,11,8,6,4,3,2 over32.
 * SCSP nominal gain is 2^(-TL/16). Round -16*log2(numerator/32)
 * to the nearest TL code. Codes >=9 are exact mute through DISDL=0.
 * The production entrance uses pre-scaled assets instead, bypassing this
 * quantization via SDIR. */
static const uint8_t volume_tl[16] = {
    0,9,16,25,32,39,48,55,64,255,255,255,255,255,255,255
};

uint8_t wp01_oki_volume_tl(unsigned nibble) { return volume_tl[nibble & 15u]; }

static void reg(wp01_oki_t *s,unsigned voice,unsigned off,uint16_t value) {
    if(s->write16)
        s->write16(s->user,(uint16_t)((WP01_OKI_SLOT_FIRST+voice)*32u+off),value);
}

static void stop_voice(wp01_oki_t *s,unsigned voice) {
    wp01_oki_voice_t *v=&s->voices[voice];
    /* SDIR bypasses the envelope, so mute the mixer before key-off. */
    reg(s,voice,0x16,0);
    reg(s,voice,0,(uint16_t)(v->control|KEYEXEC));
    v->active=0;
}

void wp01_oki_reset(wp01_oki_t *s) {
    for(unsigned i=0;i<WP01_OKI_VOICES;i++)stop_voice(s,i);
    memset(s->voices,0,sizeof(s->voices));
    memset(&s->stats,0,sizeof(s->stats));
    s->pending_sample=-1;
    s->bank=0;
}

void wp01_oki_init(wp01_oki_t *s,const wp01_oki_asset_t *assets,size_t count,
                  wp01_oki_write16_fn write16,void *user) {
    memset(s,0,sizeof(*s));
    s->assets=assets;s->asset_count=count;s->write16=write16;s->user=user;
    for(unsigned i=0;i<WP01_OKI_VOICES;i++)
        for(unsigned off=0;off<=0x16;off+=2)reg(s,i,off,0);
    wp01_oki_reset(s);
}

void wp01_oki_advance(wp01_oki_t *s,uint32_t time_44100) {
    for(unsigned i=0;i<WP01_OKI_VOICES;i++){
        wp01_oki_voice_t *v=&s->voices[i];
        if(v->active&&(uint32_t)(time_44100-v->started_at)>=v->duration){
            stop_voice(s,i);
            s->stats.natural_ends++;
        }
    }
}

static const wp01_oki_asset_t *asset(const wp01_oki_t *s,unsigned id,unsigned vol) {
    const wp01_oki_asset_t *raw=0;
    for(size_t i=0;i<s->asset_count;i++){
        const wp01_oki_asset_t *a=&s->assets[i];
        if(a->bank!=s->bank||a->sample_id!=id)continue;
        if(a->baked_volume==vol||vol>=9u)return a;
        if(a->baked_volume==WP01_OKI_RAW_VOLUME)raw=a;
    }
    return raw;
}

static void start_voice(wp01_oki_t *s,unsigned voice,unsigned id,unsigned vol,
                        uint32_t time_44100) {
    wp01_oki_voice_t *v=&s->voices[voice];
    if(v->active){s->stats.ignored_busy++;return;}
    const wp01_oki_asset_t *a=asset(s,id,vol);
    if(!a){s->stats.missing_asset++;return;}
    if(!a->samples||(a->scsp_address&1u)||
       a->scsp_address>=0x80000u||
       (uint32_t)a->samples*2u+2u>0x80000u-a->scsp_address){
        s->stats.invalid_asset++;return;
    }
    v->control=(uint16_t)((a->scsp_address>>16)&15u);
    /* Stop before replacing addresses. Only this module owns slots24..27. */
    stop_voice(s,voice);
    reg(s,voice,2,(uint16_t)a->scsp_address);
    reg(s,voice,4,0);
    reg(s,voice,6,a->samples);
    reg(s,voice,8,0x003f); /* EGHOLD + AR31; bypassed by SDIR for baked PCM. */
    reg(s,voice,0xA,0x001f); /* RR31, D1/D2=0, no loop-linked envelope. */
    const int direct=a->baked_volume==vol||vol>=9u;
    reg(s,voice,0xC,(uint16_t)(STWINH|(direct?SDIR:volume_tl[vol])));
    reg(s,voice,0xE,0); /* No FM inputs. */
    reg(s,voice,0x10,WP01_OKI_PITCH_8000);
    reg(s,voice,0x12,0); /* No LFO. */
    reg(s,voice,0x14,0); /* No DSP send. */
    reg(s,voice,0x16,vol>=9u?0:0xE000); /* Direct to both outputs, centered. */
    reg(s,voice,0,(uint16_t)(v->control|KEYON|KEYEXEC));
    v->started_at=time_44100;
    v->duration=((uint32_t)a->samples*44100u+7999u)/8000u;
    v->sample_id=(uint8_t)id;v->volume=(uint8_t)vol;v->active=1;
    s->stats.starts++;
}

void wp01_oki_write(wp01_oki_t *s,uint8_t data,uint32_t time_44100) {
    wp01_oki_advance(s,time_44100);
    if(s->pending_sample>=0){
        const unsigned id=(unsigned)s->pending_sample;
        s->pending_sample=-1;
        for(unsigned i=0;i<WP01_OKI_VOICES;i++)
            if((data>>4)&(1u<<i))start_voice(s,i,id,data&15u,time_44100);
    }else if(data&0x80u){
        s->pending_sample=data&0x7fu;
    }else{
        for(unsigned i=0;i<WP01_OKI_VOICES;i++)
            if((data>>3)&(1u<<i)){
                if(s->voices[i].active)s->stats.stops++;
                stop_voice(s,i);
            }
    }
}

int wp01_oki_set_bank(wp01_oki_t *s,unsigned bank) {
    if(bank!=0){s->stats.unsupported_bank++;return 0;}
    s->bank=0;return 1;
}

unsigned wp01_oki_status(const wp01_oki_t *s) {
    unsigned status=0xf0u;
    for(unsigned i=0;i<WP01_OKI_VOICES;i++)if(s->voices[i].active)status|=1u<<i;
    return status;
}
