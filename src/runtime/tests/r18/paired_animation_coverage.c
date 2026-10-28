#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "wf_r18_metasprite.h"

static uint8_t rom[524288];
static wf_r18_sprite_record_t records[512];
static uint8_t palette[256];
static wf_r18_metasprite_cache_t cache;
static uint16_t be16(const uint8_t *p){return (uint16_t)(((unsigned)p[0]<<8)|p[1]);}
static uint32_t be32(const uint8_t *p){return ((uint32_t)be16(p)<<16)|be16(p+2);}
static wf_status_t read_rom(void *user,uint32_t at,void *dst,size_t n){
    (void)user;
    if(at>sizeof rom||n>sizeof rom-at)return WF_BLOCKED;
    memcpy(dst,rom+at,n);return WF_OK;
}
int main(int argc,char **argv){
    if(argc!=2)return 2;
    FILE *f=fopen(argv[1],"rb");if(!f)return 2;
    size_t bytes=fread(rom,1,sizeof rom,f);fclose(f);
    if(bytes!=sizeof rom)return 2;
    wf_r18_metasprite_ctx_t ctx={0};
    ctx.read_rom=read_rom;ctx.palette_map=palette;ctx.records=records;
    ctx.capacity=512;ctx.cache=&cache;
    unsigned paired=0,attempts=0,blocked=0,nonempty=0,max_records=0;
    for(unsigned set=0;set<12;set++){
        int16_t rel=(int16_t)be16(rom+0x38f14u+set*2u);
        if(!rel)continue;
        uint32_t index=(uint32_t)(0x38f14+(int32_t)rel);
        uint32_t base=be32(rom+0x38fb8u+set*4u);
        for(unsigned pose=0;pose<565;pose++){
            uint16_t off=be16(rom+index+pose*2u);
            if(off==0xfffeu||off==0xffffu)continue;
            uint32_t at=base+off;
            if(at+2u>sizeof rom)return 3;
            if((be16(rom+at)>>14)!=2u)continue;
            paired++;
            for(unsigned partner=0;partner<12;partner++)for(unsigned mirror=0;mirror<2;mirror++){
                ctx.count=0;ctx.d6=0x12345678u;ctx.d7=0x87654321u;
                wf_status_t status=wf_r18_metasprite_emit(&ctx,(uint16_t)set,
                    (uint16_t)(pose|(mirror<<15)),160u,120u,(uint16_t)partner);
                attempts++;
                if(status!=WF_OK){
                    if(blocked<24u)printf("BLOCK set=%u pose=%u partner=%u mirror=%u\n",
                                         set,pose,partner,mirror);
                    blocked++;
                }else{
                    nonempty+=(ctx.count!=0u);
                    if(ctx.count>max_records)max_records=(unsigned)ctx.count;
                }
            }
        }
    }
    printf("paired_poses=%u attempts=%u blocked=%u nonempty=%u max_records=%u\n",
           paired,attempts,blocked,nonempty,max_records);
    return blocked?1:0;
}
