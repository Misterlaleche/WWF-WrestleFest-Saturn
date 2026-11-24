#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "wf_r18_metasprite.h"

static uint8_t rom[0x80000],palette[256];
static uint8_t aligned[512*16+4] __attribute__((aligned(4)));
static uint8_t unaligned[512*16+5] __attribute__((aligned(4)));
static wf_r18_metasprite_cache_t cache;
static wf_status_t read_rom(void *user,uint32_t at,void *out,size_t n)
{
    (void)user;
    if(at>sizeof rom||n>sizeof rom-at)return WF_BLOCKED;
    memcpy(out,rom+at,n);return WF_OK;
}
int main(int argc,char **argv)
{
    assert(argc==2);FILE *file=fopen(argv[1],"rb");assert(file);
    assert(fread(rom,1,sizeof rom,file)==sizeof rom);assert(!fclose(file));
    for(unsigned i=0;i<256;i++)palette[i]=(uint8_t)((i*7u)&15u);
    static const uint16_t x[]={0xffefu,0u,319u},y[]={8u,90u,247u};
    unsigned cases=0,records=0;
    for(unsigned set=0;set<82;set++)for(unsigned pose=0;pose<48;pose++)
        for(unsigned flip=0;flip<2;flip++)for(unsigned at=0;at<3;at++){
            memset(aligned,0xa5,sizeof aligned);memset(unaligned,0xa5,sizeof unaligned);
            wf_r18_metasprite_ctx_t a={.read_rom=read_rom,.palette_map=palette,
                .records=(wf_r18_sprite_record_t *)(void *)aligned,.capacity=512,
                .d6=0x12345678,.d7=0x87654321,.cache=&cache};
            wf_r18_metasprite_ctx_t b=a;b.records=(wf_r18_sprite_record_t *)(void *)(unaligned+1);
            unsigned p=pose|(flip?0x8000u:0u),partner=(set+3u)%12u;
            wf_status_t sa=wf_r18_metasprite_emit(&a,(uint16_t)set,(uint16_t)p,x[at],y[at],(uint16_t)partner);
            wf_status_t sb=wf_r18_metasprite_emit(&b,(uint16_t)set,(uint16_t)p,x[at],y[at],(uint16_t)partner);
            assert(sa==sb&&a.count==b.count&&a.d6==b.d6&&a.d7==b.d7);
            assert(!memcmp(aligned,unaligned+1,a.count*16));
            for(unsigned j=0;j<4;j++)assert(aligned[512*16+j]==0xa5&&unaligned[512*16+1+j]==0xa5);
            assert(unaligned[0]==0xa5);++cases;records+=(unsigned)a.count;
        }
    assert(records>10000);
    printf("Jo metasprite alignment: %u cases / %u records, exact bytes, clipping and D6/D7: PASS\n",cases,records);
    return 0;
}
