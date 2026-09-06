#include "z80.h"
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define CLK 3579545ULL

typedef struct { uint64_t until; } oki_voice_t;
typedef struct {
    z80 cpu;
    uint8_t rom[0x10000], okirom[0x80000], ram[0x800];
    uint8_t latch, ym_addr, ym_regs[256], ym_status, ym_ctrl, bank;
    uint64_t ta_next, tb_next, capture_origin, last_activity;
    bool ta_run, tb_run, capture;
    int oki_pending;
    oki_voice_t ov[4];
    FILE *log;
} ctx_t;

static uint64_t now(ctx_t *c){ return (uint64_t)c->cpu.cyc; }
static uint32_t ta_value(ctx_t *c){ return ((uint32_t)c->ym_regs[0x10]<<2)|(c->ym_regs[0x11]&3u); }
static uint64_t ta_period(ctx_t *c){ return 64ULL*(1024ULL-ta_value(c)); }
static uint64_t tb_period(ctx_t *c){ return 1024ULL*(256ULL-c->ym_regs[0x12]); }
static void update_timers(ctx_t *c){
    uint64_t n=now(c);
    if(c->ta_run){ uint64_t p=ta_period(c); if(!p)p=1; while(n>=c->ta_next){ if(c->ym_ctrl&0x04)c->ym_status|=1; c->ta_next+=p; } }
    if(c->tb_run){ uint64_t p=tb_period(c); if(!p)p=1; while(n>=c->tb_next){ if(c->ym_ctrl&0x08)c->ym_status|=2; c->tb_next+=p; } }
}
static void ym_control(ctx_t *c,uint8_t v){
    update_timers(c);
    if(v&0x10)c->ym_status&=(uint8_t)~1u;
    if(v&0x20)c->ym_status&=(uint8_t)~2u;
    bool na=(v&1)!=0, nb=(v&2)!=0;
    if(na&&!c->ta_run)c->ta_next=now(c)+ta_period(c);
    if(nb&&!c->tb_run)c->tb_next=now(c)+tb_period(c);
    c->ta_run=na; c->tb_run=nb; c->ym_ctrl=v;
}
static uint32_t oki_pcm_samples(ctx_t *c,unsigned bank,unsigned sid){
    if(sid>=128)return 0;
    uint32_t base=bank?0x40000u:0, p=base+sid*8u;
    uint32_t st=((uint32_t)c->okirom[p]<<16)|((uint32_t)c->okirom[p+1]<<8)|c->okirom[p+2];
    uint32_t en=((uint32_t)c->okirom[p+3]<<16)|((uint32_t)c->okirom[p+4]<<8)|c->okirom[p+5];
    if(st<0x400u||en<st||en>=0x40000u)return 0;
    return (en-st+1u)*2u;
}
static uint8_t oki_status(ctx_t *c){
    uint64_t n=now(c); uint8_t m=0; for(unsigned i=0;i<4;i++)if(c->ov[i].until>n)m|=(uint8_t)(1u<<i); return m;
}
static void logev(ctx_t *c,const char *chip,unsigned a,unsigned b){
    if(!c->capture||!c->log)return;
    fprintf(c->log,"%llu,%s,%u,%u\n",(unsigned long long)(now(c)-c->capture_origin),chip,a,b);
}
static void oki_write(ctx_t *c,uint8_t v){
    logev(c,"OKI",c->bank,v); if(c->capture)c->last_activity=now(c);
    if(c->oki_pending>=0){
        unsigned sid=(unsigned)c->oki_pending, mask=(v>>4)&0x0f;
        uint64_t dur=((uint64_t)oki_pcm_samples(c,c->bank,sid)*CLK+7999u)/8000u, n=now(c);
        for(unsigned i=0;i<4;i++)if(mask&(1u<<i)){ if(c->ov[i].until<=n)c->ov[i].until=n+dur; }
        c->oki_pending=-1;
    }else if(v&0x80)c->oki_pending=v&0x7f;
    else{ unsigned mask=(v>>3)&0x0f; for(unsigned i=0;i<4;i++)if(mask&(1u<<i))c->ov[i].until=0; }
}
static uint8_t rb(void *u,uint16_t a){
    ctx_t *c=(ctx_t*)u; update_timers(c);
    if(a<0xC000)return c->rom[a];
    if(a<=0xC7FF)return c->ram[a-0xC000];
    if(a==0xC800||a==0xC801)return c->ym_status;
    if(a==0xD800)return oki_status(c);
    if(a==0xE000)return c->latch;
    return 0xff;
}
static void wb(void *u,uint16_t a,uint8_t v){
    ctx_t *c=(ctx_t*)u; update_timers(c);
    if(a>=0xC000&&a<=0xC7FF){c->ram[a-0xC000]=v;return;}
    if(a==0xC800){c->ym_addr=v;return;}
    if(a==0xC801){ c->ym_regs[c->ym_addr]=v; logev(c,"YM",c->ym_addr,v); if(c->capture)c->last_activity=now(c); if(c->ym_addr==0x14)ym_control(c,v); return; }
    if(a==0xD800){oki_write(c,v);return;}
    if(a==0xE800){c->bank=v&1u;return;}
}
static uint8_t pin(z80 *z,uint8_t p){(void)z;(void)p;return 0xff;}
static void pout(z80 *z,uint8_t p,uint8_t v){(void)z;(void)p;(void)v;}
static void run_until(ctx_t *c,uint64_t target){ while(now(c)<target)z80_step(&c->cpu); update_timers(c); }

int main(int argc,char **argv){
    if(argc!=6){fprintf(stderr,"usage: %s sound.bin oki.bin command_hex seconds trace.csv\n",argv[0]);return 2;}
    ctx_t *c=calloc(1,sizeof(*c)); if(!c)return 3;
    FILE *f=fopen(argv[1],"rb"); if(!f)return 4; if(fread(c->rom,1,sizeof(c->rom),f)!=sizeof(c->rom))return 5; fclose(f);
    f=fopen(argv[2],"rb"); if(!f)return 6; if(fread(c->okirom,1,sizeof(c->okirom),f)!=sizeof(c->okirom))return 7; fclose(f);
    c->oki_pending=-1;
    z80_init(&c->cpu); c->cpu.read_byte=rb; c->cpu.write_byte=wb; c->cpu.port_in=pin; c->cpu.port_out=pout; c->cpu.userdata=c;
    run_until(c,CLK/10u);
    unsigned cmd=(unsigned)strtoul(argv[3],0,16); double sec=strtod(argv[4],0);
    c->log=fopen(argv[5],"w"); if(!c->log)return 8; fprintf(c->log,"cycle,chip,a,b\n");
    c->capture_origin=now(c); c->capture=true; c->last_activity=c->capture_origin;
    logev(c,"YM",0x10,c->ym_regs[0x10]); logev(c,"YM",0x11,c->ym_regs[0x11]); logev(c,"YM",0x12,c->ym_regs[0x12]); logev(c,"YM",0x14,c->ym_regs[0x14]);
    c->latch=(uint8_t)cmd; z80_gen_nmi(&c->cpu);
    uint64_t target=c->capture_origin+(uint64_t)(sec*(double)CLK);
    while(now(c)<target){ z80_step(&c->cpu); update_timers(c); }
    fclose(c->log);
    fprintf(stderr,"cmd=%02X elapsed=%.6f pc=%04X cycles=%lu status=%02X bank=%u\n",cmd,(double)(now(c)-c->capture_origin)/(double)CLK,c->cpu.pc,c->cpu.cyc,c->ym_status,c->bank);
    free(c); return 0;
}
