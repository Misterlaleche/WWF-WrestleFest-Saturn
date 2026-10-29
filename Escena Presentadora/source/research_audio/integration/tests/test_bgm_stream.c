#define _GNU_SOURCE
#include "wp02_bgm.h"
#include "ponesound_min.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

static uint8_t *pack, *original;
static unsigned pack_bytes, original_bytes, reads, starts, stops;
static bool playback;
static unsigned active_half;
static wf_pcm_ctrl_t controls[32];
static volatile uint16_t *const mon = (volatile uint16_t *)0x25B00408u;
static uint8_t *const ring = (uint8_t *)SCSP_RAM_ADDR(WP02_BGM_RING_OFFSET);

static void *load(const char *p, unsigned *n)
{
    FILE *f=fopen(p,"rb");assert(f);assert(!fseek(f,0,SEEK_END));
    *n=(unsigned)ftell(f);rewind(f);
    uint8_t *x=calloc(1,(*n+2047u)&~2047u);assert(x);
    assert(fread(x,1,*n,f)==*n);fclose(f);return x;
}
int cd_block_sectors_read(uint32_t fad,void *dst,size_t n)
{
    assert(fad>=100 && n%2048u==0 && n<=32768u);
    unsigned off=(fad-100u)*2048u;
    assert(off<pack_bytes && off+n<=((pack_bytes+2047u)&~2047u));
    memcpy(dst,pack+off,n);++reads;return 0;
}
void wf_ponesound_copy_to_scsp(const void *p,int n,int off)
{
    assert(off>=WP02_BGM_RING_OFFSET && off+n<=WP02_BGM_RING_OFFSET+WP02_BGM_RING_BYTES+2);
    if(playback && off<WP02_BGM_RING_OFFSET+WP02_BGM_RING_BYTES)
        assert((unsigned)(off-WP02_BGM_RING_OFFSET)/WP02_BGM_HALF_BYTES!=active_half);
    memcpy((void *)SCSP_RAM_ADDR((unsigned)off),p,(size_t)n);
}
void wf_ponesound_pcm_play(short n,char type,char volume)
{
    assert(n==3 && type==1 && volume==7);++starts;
    controls[n].loopType=type;controls[n].volume=(uint8_t)volume;controls[n].sh2_permit=1;
}
void wf_ponesound_pcm_cease(short n)
{ assert(n==3);++stops;controls[n].sh2_permit=0;playback=false; }

int main(int argc,char **argv)
{
    assert(argc==3);pack=load(argv[1],&pack_bytes);original=load(argv[2],&original_bytes);
    assert(mmap((void *)0x00200000u,0x100000,PROT_READ|PROT_WRITE,
                MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED,-1,0)==(void *)0x00200000u);
    assert(mmap((void *)0x25A00000u,0x110000,PROT_READ|PROT_WRITE,
                MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED,-1,0)==(void *)0x25A00000u);
    wf_sys_com_t *com=(wf_sys_com_t *)SCSP_RAM_ADDR(0x408+DRV_SYS_END);
    com->pcmCtrl=controls;controls[3].icsr_target=-1;
    assert(wp02_bgm_load(100,pack_bytes));unsigned initial_reads=reads;
    assert(controls[3].playsize==32769 && controls[3].pitchword==0 && controls[3].bitDepth==0);
    assert(!memcmp(ring,original,WP02_BGM_RING_BYTES));
    assert(!memcmp(ring,ring+WP02_BGM_RING_BYTES,2));
    assert(wp02_bgm_start());assert(starts==1);
    assert(wp02_bgm_service(1000)); /* Still waiting for M68K allocation. */
    controls[3].icsr_target=6;
    assert(wp02_bgm_service(2000));assert(*mon==(6u<<11));
    assert(wp02_bgm_service(2010)); /* Old latched CA must not yet be accepted. */
    *mon=0;assert(wp02_bgm_service(2100));playback=true;
    unsigned played=0;
    for(unsigned frame=1;frame<=1700;frame++) {
        unsigned goal=(unsigned)((uint64_t)frame*44100u*1000000u/59826000u);
        for(;played<goal;played++) {
            /* Assert every output sample against the original source, also
             * at every32768-sample transport wrap. */
            unsigned p=played%32768u;
            assert(!memcmp(ring+p*2u,original+played*2u,2));
        }
        active_half=(played%32768u)/16384u;
        *mon=(uint16_t)(((played%32768u)/4096u)<<7);
        assert(wp02_bgm_service(2100u+(uint64_t)frame*1000000000000u/59826000u));
    }
    assert(played*2u<original_bytes && reads==initial_reads && wp02_bgm_refills()>1000);
    wp02_bgm_stop();assert(stops==1 && !wp02_bgm_is_running());
    /* A sound-driver reload reprimes but does not reread unchanged CD data. */
    controls[3].icsr_target=-1;memset(ring,0,WP02_BGM_RING_BYTES+2);
    assert(wp02_bgm_load(100,pack_bytes));assert(reads==initial_reads);
    assert(!memcmp(ring,original,WP02_BGM_RING_BYTES));
    assert(wp02_bgm_start());controls[3].icsr_target=4;
    assert(wp02_bgm_service(40000000));*mon=0;
    assert(wp02_bgm_service(40001000));
    assert(!wp02_bgm_service(40401000));assert(!wp02_bgm_is_running());
    assert(wp02_bgm_failures()==1);
    /* Losing an allocated physical slot is not a new allocation wait. */
    controls[3].icsr_target=-1;assert(wp02_bgm_load(100,pack_bytes));
    assert(wp02_bgm_start());controls[3].icsr_target=4;
    assert(wp02_bgm_service(50000000));*mon=0;
    assert(wp02_bgm_service(50001000));controls[3].icsr_target=-1;
    assert(!wp02_bgm_service(50002000));
    free(pack);free(original);
    puts("PASS resident BGM: every emitted PCM16 sample, ring seams, inactive writes, cached reload, no runtime CD, delayed monitor, late-service stop");
    return 0;
}
