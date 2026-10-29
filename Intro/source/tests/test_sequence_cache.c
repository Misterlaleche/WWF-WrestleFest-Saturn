#include "wp0_sequence_cache.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint32_t be32(const uint8_t *p) {
    return ((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|((uint32_t)p[2]<<8)|p[3];
}
int main(int argc,char **argv) {
    assert(argc==2);
    FILE *f=fopen(argv[1],"rb"); assert(f);
    assert(fseek(f,0,SEEK_END)==0); long length=ftell(f); assert(length==398444);
    rewind(f); uint8_t *seq=malloc((size_t)length+4); assert(seq);
    assert(fread(seq,1,(size_t)length,f)==(size_t)length); fclose(f);
    uint32_t *guarded=malloc(131072+8); assert(guarded);
    uint8_t *storage=(uint8_t *)(guarded+1);
    guarded[0]=0x12345678; guarded[32769]=0x87654321;
    wp0_sequence_cache_t cache;
    assert(wp0_sequence_cache_prepare(&cache,seq,(uint32_t)length,storage,131072));
    assert(cache.count==6 && cache.used==106072);
    unsigned packets=0,cached=0;
    for(uint32_t offset=0;offset<(uint32_t)length;) {
        uint32_t size=be32(seq+offset);
        const uint8_t *p=wp0_sequence_cache_resolve(&cache,offset);
        assert(p && memcmp(p,seq+offset,size)==0);
        if(offset && size>=4096) {
            assert(p>=storage && p+size<=storage+106072); ++cached;
        } else assert(p==seq+offset);
        offset+=size; ++packets;
    }
    assert(packets==2711 && cached==6);
    assert(guarded[0]==0x12345678 && guarded[32769]==0x87654321);
    assert(!wp0_sequence_cache_resolve(&cache,(uint32_t)length));
    assert(!wp0_sequence_cache_resolve(&cache,UINT32_MAX));
    assert(!wp0_sequence_cache_prepare(&cache,seq,(uint32_t)length,storage,106071));
    assert(!wp0_sequence_cache_resolve(&cache,0));
    assert(!wp0_sequence_cache_prepare(&cache,seq,(uint32_t)length-4,storage,131072));
    assert(!wp0_sequence_cache_prepare(&cache,seq+1,(uint32_t)length,storage,131072));
    assert(!wp0_sequence_cache_prepare(&cache,seq,(uint32_t)length,storage+1,131071));
    uint8_t header[4]; memcpy(header,seq,4);
    memset(seq,0,4);
    assert(!wp0_sequence_cache_prepare(&cache,seq,(uint32_t)length,storage,131072));
    memset(seq,0xff,4);
    assert(!wp0_sequence_cache_prepare(&cache,seq,(uint32_t)length,storage,131072));
    memcpy(seq,header,4);
    assert(wp0_sequence_cache_prepare(&cache,seq,(uint32_t)length,storage,106072));
    assert(cache.count==6 && cache.used==106072);
    free(guarded); free(seq);
    puts("sequence cache PASS: all2711 packets exact;6active bulk packets106072bytes inHWRAM; bounds/alignment/canaries");
    return 0;
}
