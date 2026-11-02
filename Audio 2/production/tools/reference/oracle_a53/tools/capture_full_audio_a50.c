#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "z80.h"

#define WF_CLK 3579545ULL

typedef struct {
    z80 cpu;
    uint8_t rom[0x10000];
    uint8_t ram[0x800];
    uint8_t latch;
    uint8_t ym_addr;
    uint8_t ym_status;
    uint8_t ym_tb;
    uint8_t ym_ctl;
    uint8_t bank;
    uint64_t next_tb;
    FILE *log;
} wf_ctx_t;

static uint64_t now(wf_ctx_t *c){ return c->cpu.cyc; }

static void schedule_tb(wf_ctx_t *c){
    uint64_t p=(uint64_t)(256u-c->ym_tb)*1024u;
    if(!p)p=1024;
    c->next_tb=now(c)+p;
}
static void update_timer(wf_ctx_t *c){
    if((c->ym_ctl&2u) && now(c)>=c->next_tb){
        uint64_t p=(uint64_t)(256u-c->ym_tb)*1024u;
        if(!p)p=1024;
        while(now(c)>=c->next_tb)c->next_tb+=p;
        c->ym_status|=2u;
        if(c->ym_ctl&8u)z80_gen_int(&c->cpu,0xff);
    }
}
static void loge(wf_ctx_t*c,const char*kind,unsigned a,unsigned b){
    fprintf(c->log,"%llu,%s,%u,%u\n",
            (unsigned long long)now(c),kind,a,b);
}
static uint8_t rb(void*u,uint16_t a){
    wf_ctx_t*c=u;
    if(a<0xc000)return c->rom[a];
    if(a<0xc800)return c->ram[a-0xc000];
    if(a==0xc800||a==0xc801)return c->ym_status;
    if(a==0xd800)return 0;
    if(a==0xe000)return c->latch;
    return 0xff;
}
static void wb(void*u,uint16_t a,uint8_t v){
    wf_ctx_t*c=u;
    if(a>=0xc000&&a<0xc800){c->ram[a-0xc000]=v;return;}
    if(a==0xc800){
        c->ym_addr=v;
        loge(c,"YM_A",0,v);
        return;
    }
    if(a==0xc801){
        uint8_t r=c->ym_addr;
        loge(c,"YM_D",r,v);
        if(r==0x12)c->ym_tb=v;
        if(r==0x14){
            uint8_t old=c->ym_ctl;
            c->ym_ctl=v;
            if(v&0x20)c->ym_status&=~2u;
            if((v&2u)&&!(old&2u))schedule_tb(c);
        }
        return;
    }
    if(a==0xd800){loge(c,"OKI",0,v);return;}
    if(a==0xe800){c->bank=v&1u;loge(c,"BANK",0,c->bank);return;}
}
static uint8_t pin(z80*z,uint8_t p){(void)z;(void)p;return 0xff;}
static void pout(z80*z,uint8_t p,uint8_t v){(void)z;(void)p;(void)v;}
static int load(const char*p,uint8_t*b,size_t n){
    FILE*f=fopen(p,"rb");size_t x;
    if(!f)return 0;
    x=fread(b,1,n,f);
    fclose(f);
    return x==n;
}
int main(int ac,char**av){
    wf_ctx_t*c;
    uint64_t boot_end,run_end;
    unsigned command;
    double seconds;
    if(ac!=5){
        fprintf(stderr,"usage: %s SDRV.BIN CMD_HEX SECONDS OUT.csv\n",av[0]);
        return 2;
    }
    c=calloc(1,sizeof *c);
    if(!c||!load(av[1],c->rom,sizeof c->rom))return 3;
    command=(unsigned)strtoul(av[2],0,16);
    seconds=strtod(av[3],0);
    if(command<1u||command>0x12u||seconds<=0)return 4;
    c->log=fopen(av[4],"w");if(!c->log)return 5;
    fprintf(c->log,"cycle,kind,a,b\n");

    z80_init(&c->cpu);
    c->cpu.read_byte=rb;c->cpu.write_byte=wb;
    c->cpu.port_in=pin;c->cpu.port_out=pout;c->cpu.userdata=c;

    /* Preserve boot writes so the synth can be reconstructed from reset. */
    boot_end=2u*WF_CLK;
    while(now(c)<boot_end){z80_step(&c->cpu);update_timer(c);}

    fprintf(c->log,"%llu,ORIGIN,%u,0\n",
            (unsigned long long)now(c),command);
    c->latch=(uint8_t)command;
    z80_gen_nmi(&c->cpu);

    run_end=now(c)+(uint64_t)(seconds*(double)WF_CLK);
    while(now(c)<run_end){z80_step(&c->cpu);update_timer(c);}
    fprintf(c->log,"%llu,END,0,0\n",(unsigned long long)now(c));
    fclose(c->log);free(c);return 0;
}
