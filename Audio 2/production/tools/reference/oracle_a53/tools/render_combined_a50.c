#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "opm.h"

#define MASTER_HZ 3579545ULL
#define OUT_HZ 44100ULL
#define OKI_HZ 8000ULL

typedef struct {
    uint64_t cycle;
    char kind[8];
    unsigned a,b;
} event_t;

typedef struct {
    uint32_t offset,bytes;
    int valid;
} asset_t;

typedef struct {
    int playing;
    unsigned sample_id;
    uint32_t pos,bytes,offset;
    unsigned volume;
    int8_t current;
} voice_t;

static const int oki_num[16]={32,22,16,11,8,6,4,3,2,0,0,0,0,0,0,0};

static int load_file(const char*p,uint8_t**buf,size_t*sz){
    FILE*f=fopen(p,"rb");long n;
    if(!f)return 0;
    fseek(f,0,SEEK_END);
    n=ftell(f);
    rewind(f);
    if(n<=0){
        fclose(f);
        return 0;
    }
    *buf=malloc((size_t)n);if(!*buf){fclose(f);return 0;}
    if(fread(*buf,1,(size_t)n,f)!=(size_t)n){fclose(f);free(*buf);return 0;}
    fclose(f);*sz=(size_t)n;return 1;
}
static int load_index(const char*p,asset_t bank0[128]){
    FILE*f=fopen(p,"r");char line[256];
    if(!f)return 0;
    memset(bank0,0,sizeof(asset_t)*128);
    if(!fgets(line,sizeof line,f)){fclose(f);return 0;}
    while(fgets(line,sizeof line,f)){
        unsigned bank,sid,sector,offset,bytes;
        if(sscanf(line,"%u,%u,%u,%u,%u",&bank,&sid,&sector,&offset,&bytes)==5 &&
           bank==0u && sid<128u){
            bank0[sid].offset=offset;
            bank0[sid].bytes=bytes;
            bank0[sid].valid=bytes!=0u;
        }
    }
    fclose(f);return 1;
}
static int load_events(const char*p,event_t**out,size_t*nout,
                       uint64_t*origin,uint64_t*end){
    FILE*f=fopen(p,"r");char line[256];size_t n=0,cap=4096;event_t*e;
    if(!f)return 0;
    e=malloc(cap*sizeof(*e));
    if(!e){
        fclose(f);
        return 0;
    }
    if(!fgets(line,sizeof line,f)){free(e);fclose(f);return 0;}
    *origin=UINT64_MAX;*end=0;
    while(fgets(line,sizeof line,f)){
        unsigned long long cyc;char kind[8];unsigned a,b;
        if(sscanf(line,"%llu,%7[^,],%u,%u",&cyc,kind,&a,&b)!=4)continue;
        if(!strcmp(kind,"ORIGIN")){*origin=(uint64_t)cyc;continue;}
        if(!strcmp(kind,"END")){*end=(uint64_t)cyc;continue;}
        if(n==cap){cap*=2;e=realloc(e,cap*sizeof(*e));if(!e){fclose(f);return 0;}}
        e[n].cycle=(uint64_t)cyc;strncpy(e[n].kind,kind,7);e[n].kind[7]=0;
        e[n].a=a;e[n].b=b;n++;
    }
    fclose(f);
    if(*origin==UINT64_MAX||!*end){free(e);return 0;}
    *out=e;*nout=n;return 1;
}
static void oki_write(uint8_t data,voice_t v[4],int*pending,
                      const asset_t bank0[128]){
    unsigned i;
    if(*pending>=0){
        unsigned mask=(data>>4)&15u,vol=data&15u,sid=(unsigned)*pending;
        for(i=0;i<4;i++)if(mask&(1u<<i)){
            if(!v[i].playing && sid<128u && bank0[sid].valid){
                v[i].playing=1;v[i].sample_id=sid;v[i].pos=0;
                v[i].bytes=bank0[sid].bytes;v[i].offset=bank0[sid].offset;
                v[i].volume=vol;v[i].current=0;
            }
        }
        *pending=-1;return;
    }
    if(data&0x80u){*pending=(int)(data&0x7fu);return;}
    {
        unsigned mask=data>>3;
        for(i=0;i<4;i++)if(mask&(1u<<i)){v[i].playing=0;v[i].current=0;}
    }
}
static int32_t oki_mix(voice_t v[4]){
    int32_t sum=0;unsigned i;
    for(i=0;i<4;i++)if(v[i].playing){
        int s=(int)v[i].current;
        sum += (s*256*oki_num[v[i].volume&15u])/32;
    }
    return sum;
}
static void oki_tick(voice_t v[4],const uint8_t*pak,size_t pak_sz){
    unsigned i;
    for(i=0;i<4;i++)if(v[i].playing){
        if(v[i].pos>=v[i].bytes || (uint64_t)v[i].offset+v[i].pos>=pak_sz){
            v[i].playing=0;v[i].current=0;continue;
        }
        v[i].current=(int8_t)pak[v[i].offset+v[i].pos++];
        if(v[i].pos>=v[i].bytes)v[i].playing=0;
    }
}
static int16_t clip16(int64_t x){
    if(x>32767)return 32767;
    if(x<-32768)return -32768;
    return (int16_t)x;
}
int main(int ac,char**av){
    event_t*ev=0;size_t nev=0,ei=0;uint64_t origin,end,cyc;
    uint8_t*pak=0;size_t pak_sz=0;asset_t bank0[128];
    voice_t voices[4]={{0}};int pending=-1;unsigned bank=0;
    opm_t opm;int32_t y[2]={0,0};uint64_t out_phase=0,oki_phase=0;
    FILE*outf;uint64_t samples=0;
    if(ac!=5){
        fprintf(stderr,"usage: %s TRACE.csv AUDPCM.PAK AUDPCM-INDEX.csv OUT.pcm\n",av[0]);
        return 2;
    }
    if(!load_events(av[1],&ev,&nev,&origin,&end))return 3;
    if(!load_file(av[2],&pak,&pak_sz)||!load_index(av[3],bank0))return 4;
    outf=fopen(av[4],"wb");if(!outf)return 5;

    OPM_Reset(&opm,opm_flags_none);

    for(cyc=0;cyc<=end;cyc++){
        while(ei<nev && ev[ei].cycle==cyc){
            if(!strcmp(ev[ei].kind,"YM_A"))OPM_Write(&opm,0,ev[ei].b);
            else if(!strcmp(ev[ei].kind,"YM_D"))OPM_Write(&opm,1,ev[ei].b);
            else if(!strcmp(ev[ei].kind,"OKI"))oki_write((uint8_t)ev[ei].b,voices,&pending,bank0);
            else if(!strcmp(ev[ei].kind,"BANK")){
                bank=ev[ei].b&1u;
                if(bank!=0u){
                    fprintf(stderr,"A50 bank assertion failed at cycle %llu\n",
                            (unsigned long long)cyc);
                    fclose(outf);return 6;
                }
            }
            ei++;
        }

        OPM_Clock(&opm,y,0,0,0);

        oki_phase+=OKI_HZ;
        if(oki_phase>=MASTER_HZ){
            oki_phase-=MASTER_HZ;
            oki_tick(voices,pak,pak_sz);
        }

        if(cyc>=origin){
            out_phase+=OUT_HZ;
            if(out_phase>=MASTER_HZ){
                int32_t o=oki_mix(voices);
                /*
                 * MAME WWF reference routes YM L=0.33, YM R=0.33, OKI=0.66.
                 * Q15 approximations: 0.33=10813, 0.66=21627.
                 * This is a production calibration reference, not ROM data.
                 */
                int64_t m=(int64_t)y[0]*10813 + (int64_t)y[1]*10813 +
                          (int64_t)o*21627;
                int16_t s=clip16(m>>15);
                uint8_t be[2]={(uint8_t)((uint16_t)s>>8),(uint8_t)s};
                fwrite(be,1,2,outf);samples++;
                out_phase-=MASTER_HZ;
            }
        }
    }
    fclose(outf);free(ev);free(pak);
    fprintf(stderr,"A50 samples=%llu seconds=%.9f bank=%u\n",
            (unsigned long long)samples,(double)samples/44100.0,bank);
    return 0;
}
