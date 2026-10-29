/* Original-register driven native SCSP FM bridge for WP01 entrance.
 * Waveform/frequency data derived from ymfm (BSD3 Aaron Giles; see license).
 * No channel or mixed soundtrack is stored or rendered by this module.
 */
#include "wp01_fm.h"
#include "wp01_fm_tables.h"
#include <string.h>

static uint8_t ym[256];
static uint16_t scsp[24][12];
static wp01_fm_writer_t writer;
static void *writer_context;
static uint32_t unsupported;
static const int8_t channel_group[8]={0,1,2,3,4,-1,-1,5};
static const uint8_t natural_offset[4]={0,16,8,24};
static const uint8_t physical_to_natural[4]={0,2,1,3};
static const uint8_t carrier_mask[8]={8,8,8,8,10,14,14,15};
/* Each pair is a pair of natural operator indices+1; zero means no input. */
static const uint8_t routes[8][3][2]={
    {{1,1},{2,2},{3,3}}, {{0,0},{1,2},{3,3}},
    {{0,0},{2,2},{1,3}}, {{1,1},{0,0},{2,3}},
    {{1,1},{0,0},{3,3}}, {{1,1},{1,1},{1,1}},
    {{1,1},{0,0},{0,0}}, {{0,0},{0,0},{0,0}}
};

static void emit(unsigned slot,unsigned offset,uint16_t value){
    unsigned word=offset/2;
    if(scsp[slot][word]==value)return;
    scsp[slot][word]=value;
    writer(writer_context,(uint16_t)(slot*32+offset),value);
}
static void execute(unsigned slot){
    writer(writer_context,(uint16_t)(slot*32),(uint16_t)(scsp[slot][0]|0x1000));
}
static unsigned effective(unsigned rate,unsigned scale){
    if(!rate)return 0;
    return rate+scale>63?63:rate+scale;
}

uint16_t wp01_fm_pitch_word(uint8_t kc,uint8_t kf,uint8_t dt_mul,uint8_t dt2){
    unsigned block=(kc>>4)&7;
    int code=((kc&15)-((kc>>2)&3))*64+(kf>>2);
    static const unsigned coarse[4]={0,384,500,608};
    code+=(int)coarse[dt2&3];
    while(code>=768){code-=768;if(++block>7){block=7;code=767;break;}}
    int32_t step=(int32_t)(fm_phase_step[code]>>(block^7));
    unsigned dt=(dt_mul>>4)&7, key=(kc>>2)&31;
    int delta=fm_detune[key*4+(dt&3)];
    step+=(dt&4)?-delta:delta;
    unsigned multiple=(dt_mul&15)*2;if(!multiple)multiple=1;
    step=(step*(int32_t)multiple)>>1;
    if(step<1)step=1;
    /* SCSP's 1024-sample oscillator phase rate, converted from OPM clock/64.
     * Quantize only the final 10-bit FNS mantissa, retain original OPM detunes.
     */
    uint64_t numerator=(uint64_t)(uint32_t)step*3579545U*256U;
    uint64_t denominator=64U*44100U;
    int octave=-8;
    while(numerator>=denominator*2048U && octave<7){denominator*=2U;++octave;}
    unsigned mantissa=(unsigned)((numerator+denominator/2U)/denominator);
    if(mantissa>=2048 && octave<7){mantissa=(mantissa+1)/2;++octave;}
    if(mantissa<1024){mantissa=1024;unsupported|=WP01_FM_PITCH_CLAMPED;}
    if(mantissa>2047){mantissa=2047;unsupported|=WP01_FM_PITCH_CLAMPED;}
    return (uint16_t)(((octave&15)<<11)|(mantissa-1024));
}

static void pitch(unsigned channel,unsigned op){
    unsigned physical=natural_offset[op]+channel;
    unsigned slot=(unsigned)channel_group[channel]*4+op;
    emit(slot,0x10,wp01_fm_pitch_word(ym[0x28+channel],ym[0x30+channel],
                                    ym[0x40+physical],ym[0xc0+physical]>>6));
}
static void envelope(unsigned channel,unsigned op){
    unsigned physical=natural_offset[op]+channel;
    unsigned slot=(unsigned)channel_group[channel]*4+op;
    unsigned ks=ym[0x80+physical]>>6;
    unsigned scale=((ym[0x28+channel]>>2)&31)>>(ks^3);
    unsigned ar=effective((ym[0x80+physical]&31)*2,scale);
    unsigned d1=effective((ym[0xa0+physical]&31)*2,scale);
    unsigned d2=effective((ym[0xc0+physical]&31)*2,scale);
    unsigned rr=effective((ym[0xe0+physical]&15)*4+2,scale);
    unsigned dl=ym[0xe0+physical]>>4;if(dl==15)dl=31;
    unsigned hold=ar>=62?0x20:0;
    emit(slot,8,(uint16_t)((fm_rate[d2]<<11)|(fm_rate[d1]<<6)|hold|fm_attack_rate[ar]));
    emit(slot,10,(uint16_t)(0x3c00|(dl<<5)|fm_rate[rr]));
}
static void level(unsigned channel,unsigned op){
    unsigned physical=natural_offset[op]+channel;
    unsigned slot=(unsigned)channel_group[channel]*4+op;
    emit(slot,12,(uint16_t)((ym[0x60+physical]&127)*2));
}
static void topology(unsigned channel){
    unsigned group=(unsigned)channel_group[channel],algorithm=ym[0x20+channel]&7;
    unsigned feedback=(ym[0x20+channel]>>3)&7;
    unsigned panning=ym[0x20+channel]>>6;
    for(unsigned op=0;op<4;op++){
        unsigned slot=group*4+op;
        unsigned carrier=(carrier_mask[algorithm]>>op)&1;
        uint32_t sa=carrier?WP01_FM_CARRIER_SA:WP01_FM_SINE_SA;
        emit(slot,0,(uint16_t)((scsp[slot][0]&0x800)|0x20|(sa>>16)));
        emit(slot,2,(uint16_t)sa);
        uint16_t modulation=0;
        if(op==0){
            if(feedback)modulation=(uint16_t)(((feedback+4)<<12)|(0x20<<6)|0x00);
        }else{
            unsigned x=routes[algorithm][op-1][0],y=routes[algorithm][op-1][1];
            if(x){
                int dx=(int)x-1-(int)op,dy=(int)y-1-(int)op;
                unsigned mdl=x==y?12:13;
                modulation=(uint16_t)((mdl<<12)|(((32+dx)&63)<<6)|((32+dy)&63));
            }
        }
        emit(slot,14,modulation);
        /* Original cue uses both YM outputs, summed to mono at0.45 each.
         * Native carrier bank has0.90 gain; DISDL5 divides the 4x sine by4.
         * Preserve channel pan if used outside the audited cue.
         */
        unsigned pan=panning==1?31:(panning==2?15:0);
        emit(slot,22,(uint16_t)(carrier&&panning?(5U<<13)|(pan<<8):0));
    }
}

void wp01_fm_init(wp01_fm_writer_t callback,void *context){
    writer=callback;writer_context=context;unsupported=0;
    memset(ym,0,sizeof(ym));memset(scsp,0xff,sizeof(scsp));
    for(unsigned slot=0;slot<24;slot++){
        emit(slot,0,0x21);emit(slot,2,0);emit(slot,4,0);emit(slot,6,1024);
        emit(slot,8,31);emit(slot,10,0x3c1f);emit(slot,12,255);
        emit(slot,14,0);emit(slot,16,0);emit(slot,18,0);emit(slot,20,0);emit(slot,22,0);
    }
    execute(0);
}
void wp01_fm_load_registers(const uint8_t original[256]){
    memcpy(ym,original,sizeof(ym));
    for(unsigned c=0;c<8;c++)if(channel_group[c]>=0){
        topology(c);
        for(unsigned op=0;op<4;op++){pitch(c,op);envelope(c,op);level(c,op);}
    }
    if(ym[0x0f]&0x80)unsupported|=WP01_FM_UNSUPPORTED_NOISE;
    if((ym[0x19]&127)||(ym[0x1a]&127))unsupported|=WP01_FM_UNSUPPORTED_LFO;
}
void wp01_fm_write(uint8_t address,uint8_t value){
    unsigned stored=address;
    if(address==0x19)stored+=value>>7;
    if(address!=0x1a)ym[stored]=value;
    if(address==8){
        unsigned c=value&7;
        if(channel_group[c]<0){
            if((value>>3)>1)unsupported|=WP01_FM_UNSUPPORTED_ACTIVE_CHANNEL;
            return;
        }
        unsigned start=(unsigned)channel_group[c]*4;
        for(unsigned op=0;op<4;op++)
            emit(start+op,0,(uint16_t)((scsp[start+op][0]&~0x800U)|(((value>>(op+3))&1)<<11)));
        execute(start);
        return;
    }
    if(address==0x0f){if(value&0x80)unsupported|=WP01_FM_UNSUPPORTED_NOISE;return;}
    if(address==0x19){if(value&127)unsupported|=WP01_FM_UNSUPPORTED_LFO;return;}
    if(address>=0x20&&address<0x28){unsigned c=address&7;if(channel_group[c]>=0)topology(c);return;}
    if(address>=0x28&&address<0x38){
        unsigned c=address&7;if(channel_group[c]<0)return;
        for(unsigned op=0;op<4;op++){pitch(c,op);if(address<0x30)envelope(c,op);}
        return;
    }
    if(address>=0x38&&address<0x40){if(value&0x73)unsupported|=WP01_FM_UNSUPPORTED_LFO;return;}
    if(address<0x40)return;
    unsigned c=address&7;if(channel_group[c]<0)return;
    unsigned op=physical_to_natural[(address>>3)&3];
    switch(address&0xe0){
        case 0x40:pitch(c,op);break;
        case 0x60:level(c,op);break;
        case 0x80:case 0xa0:case 0xe0:envelope(c,op);break;
        case 0xc0:pitch(c,op);envelope(c,op);break;
    }
}
void wp01_fm_stop(void){
    for(unsigned slot=0;slot<24;slot++){
        emit(slot,0,(uint16_t)(scsp[slot][0]&~0x800U));emit(slot,22,0);
    }
    execute(0);
}
uint32_t wp01_fm_unsupported(void){return unsupported;}
