#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "wf_r18_metasprite.h"

extern wf_status_t wf_b12_metasprite_emit(wf_r18_metasprite_ctx_t *,uint16_t,uint16_t,uint16_t,uint16_t,uint16_t);
static uint8_t rom[0x80000],palette[2][256];
static uint8_t output[2][512*16+8] __attribute__((aligned(4)));
static wf_r18_metasprite_cache_t cache[2],alternate[2];
static unsigned cases,records,failed;
typedef struct probe { unsigned reads,fail_at,mutate;uint64_t digest;wf_r18_metasprite_ctx_t *ctx; wf_r18_metasprite_cache_t *alternate; } probe_t;
static wf_status_t read_rom(void *user,uint32_t at,void *out,size_t n)
{
    probe_t *p=user;++p->reads;p->digest=(p->digest^at)*UINT64_C(1099511628211);
    p->digest=(p->digest^n)*UINT64_C(1099511628211);
    if(p->mutate&&p->reads==2u){p->ctx->d6^=0xaaaaaaaa;p->ctx->d7+=0x10203u;}
    if((p->mutate&2u)&&p->reads==2u)p->ctx->cache=p->alternate;
    if(p->reads==p->fail_at)return WF_BLOCKED;
    if(at>sizeof rom||n>sizeof rom-at)return WF_BLOCKED;
    memcpy(out,rom+at,n);return WF_OK;
}
static void compare(unsigned set,unsigned pose,unsigned partner,uint16_t x,uint16_t y,
                    unsigned capacity,unsigned count,unsigned align,unsigned failure,unsigned mutation)
{
    probe_t p[2]={{.fail_at=failure,.mutate=mutation},{.fail_at=failure,.mutate=mutation}};
    wf_r18_metasprite_ctx_t c[2];
    for(unsigned k=0;k<2;k++){
        memset(output[k],0xa5,sizeof output[k]);
        c[k]=(wf_r18_metasprite_ctx_t){.read_rom=read_rom,.user=&p[k],.palette_map=palette[k],
            .records=(wf_r18_sprite_record_t *)(void *)(output[k]+align),.capacity=capacity,.count=count,
            .d6=0x12345678,.d7=0x87654321,.cache=&cache[k]};p[k].ctx=&c[k];p[k].alternate=&alternate[k];
    }
#ifdef WF_R18_METASPRITE_HINT_SLOTS
    /* Stale/sentinel/out-of-range values are accelerators only. They may
     * select a different valid entry but never change the fallback result. */
    if(cases%11u==0u)memset(cache[1].program_hint,255,sizeof cache[1].program_hint);
    if(cases%11u==1u)memset(cache[1].program_hint,0,sizeof cache[1].program_hint);
    if(cases%11u==2u)memset(cache[1].program_hint,16,sizeof cache[1].program_hint);
    if(cases%11u==3u)memset(cache[1].program_hint,17,sizeof cache[1].program_hint);
#endif
    wf_status_t a=wf_b12_metasprite_emit(c,(uint16_t)set,(uint16_t)pose,x,y,(uint16_t)partner);
    wf_status_t b=wf_r18_metasprite_emit(c+1,(uint16_t)set,(uint16_t)pose,x,y,(uint16_t)partner);
    assert(a==b&&c[0].count==c[1].count&&c[0].d6==c[1].d6&&c[0].d7==c[1].d7);
    assert(p[0].reads==p[1].reads&&p[0].digest==p[1].digest);
    assert(!memcmp(output[0],output[1],sizeof output[0]));
#ifdef WF_R18_METASPRITE_HINT_SLOTS
    assert(!memcmp(cache,cache+1,offsetof(wf_r18_metasprite_cache_t,program_hint)));
#else
    assert(!memcmp(cache,cache+1,sizeof cache[0]));
#endif
    assert((c[0].cache==alternate)==(c[1].cache==alternate+1));
#ifdef WF_R18_METASPRITE_HINT_SLOTS
    assert(!memcmp(alternate,alternate+1,offsetof(wf_r18_metasprite_cache_t,program_hint)));
#else
    assert(!memcmp(alternate,alternate+1,sizeof alternate[0]));
#endif
    records+=(unsigned)c[0].count;failed+=a!=WF_OK;++cases;
}
int main(int argc,char **argv)
{
    assert(argc==2);FILE *f=fopen(argv[1],"rb");assert(f);
    assert(fread(rom,1,sizeof rom,f)==sizeof rom&&!fclose(f));
    for(unsigned k=0;k<2;k++)for(unsigned i=0;i<256;i++)palette[k][i]=(uint8_t)((i*7u)&15u);
    for(unsigned set=0;set<82;set++)for(unsigned pose=0;pose<48;pose++)for(unsigned flip=0;flip<2;flip++){
        unsigned p=pose|(flip?0x8000u:0u),partner=(set+3u)%12u;
        compare(set,p,partner,64,90,512,0,0,0,0);
        /* The cached program must reevaluate clipping/capacity/registers and
         * both aligned and unaligned output, with no decoder read on a hit. */
        compare(set,p,partner,0xffef,8,512,0,1,0,0);
        compare(set,p,partner,319,247,3,1,2,0,0);
        for(unsigned k=0;k<2;k++)palette[k][pose]=(uint8_t)(palette[k][pose]^15u);
        compare(set,p,partner,12,100,512,0,3,0,0);
    }
    for(unsigned n=1;n<=24;n++)for(unsigned set=0;set<82;set++){
        memset(cache,0,sizeof cache);
        compare(set,(set*7u)%48u,(set+1u)%14u,48,80,512,0,n&3u,n,n&1u);
        compare(set,(set*7u)%48u,(set+1u)%14u,48,80,512,0,n&3u,0,0);
    }
    /* Invalid program op, malformed bounds and the entry limit are observable
     * public states; compare rollback and all bytes, including dirty tails. */
    memset(cache,0,sizeof cache);
    for(unsigned k=0;k<2;k++){
        cache[k].programs[0].valid=1;cache[k].programs[0].set=4;
        cache[k].programs[0].pose=2;cache[k].programs[0].partner=0;
        cache[k].programs[0].count=1;cache[k].programs[0].op[0][0]=255;
    }
    compare(4,2,0,64,90,512,1,0,0,0);
    compare(82,0,0,64,90,512,0,0,0,0);
    compare(0,0,0,64,90,512,512,0,0,0);
    compare(0,0,0,64,90,4,5,0,0,0);
    /* The original clock's wrap and eviction tie rules are preserved. */
    memset(cache,0,sizeof cache);
    for(unsigned k=0;k<2;k++)cache[k].program_clock=UINT32_MAX-8u;
    for(unsigned round=0;round<8;round++)for(unsigned set=0;set<12;set++)
        for(unsigned pose=0;pose<20;pose++)
            compare(set,pose|((round&1u)?0x8000u:0u),(set+round)%12u,
                    (uint16_t)(64+round),90,512,0,round&3u,0,0);
    /* A cache reset is also the source identity boundary. */
    for(unsigned k=0;k<2;k++)wf_r18_metasprite_cache_reset(cache+k);
    for(unsigned i=0;i<sizeof cache;i++)assert(((unsigned char *)cache)[i]==0u);
    compare(4,2,0,64,90,512,0,0,0,0);
    /* A reader may change the context's cache pointer. Hint ownership stays
     * with the cache that selected the program, while legacy writes match. */
    for(unsigned failure=0;failure<8;failure++){
        memset(cache,0,sizeof cache);memset(alternate,0,sizeof alternate);
        compare(4,2,0,64,90,512,0,0,failure,2u|(failure&1u));
        compare(4,2,0,64,90,512,0,1,0,0);
    }
    printf("PASS B12 metasprite differential: %u cases/%u records/%u failures; replay/cache bytes, callbacks, alignment, clipping, registers\n",cases,records,failed);
    return 0;
}
