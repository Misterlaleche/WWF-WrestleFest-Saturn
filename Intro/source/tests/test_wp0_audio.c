#include "wp0_rice.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct { const uint8_t *data; size_t bytes; } memory_t;
static bool memory_read(uint32_t off,void *dst,uint32_t count,void *user)
{
    memory_t *m=user;
    if(off>m->bytes||count>m->bytes-off)return false;
    memcpy(dst,m->data+off,count);return true;
}
static uint8_t *load(const char *name,size_t *bytes)
{
    FILE *f=fopen(name,"rb");assert(f);
    assert(fseek(f,0,SEEK_END)==0);long n=ftell(f);assert(n>=0);
    rewind(f);uint8_t *p=malloc((size_t)n+1);assert(p);
    assert(fread(p,1,(size_t)n,f)==(size_t)n);fclose(f);
    *bytes=(size_t)n;return p;
}
static void test_known_block(void)
{
    /* Independent hand-encoded Rice vector: residuals +1,-2,0, k=1. */
    const uint8_t packet[]={0,4,0,3,1,1,0,0,0,100,0x4e};
    int16_t out[6]={12345,0,0,0,0,23456};unsigned n=0;
    assert(wp0_rice_decode_block(packet,sizeof packet,out+1,4,&n));
    assert(n==4&&out[1]==100&&out[2]==101&&out[3]==99&&out[4]==99);
    assert(out[0]==12345&&out[5]==23456);
    for(size_t i=0;i<sizeof packet;i++)
        assert(!wp0_rice_decode_block(packet,i,out+1,4,&n));
    assert(!wp0_rice_decode_block(packet,sizeof packet,out+1,3,&n));
    uint8_t bad[sizeof packet];memcpy(bad,packet,sizeof bad);
    bad[5]=17;assert(!wp0_rice_decode_block(bad,sizeof bad,out+1,4,&n));
    memcpy(bad,packet,sizeof bad);bad[6]=1;
    assert(!wp0_rice_decode_block(bad,sizeof bad,out+1,4,&n));
    memcpy(bad,packet,sizeof bad);bad[10]=0;
    assert(!wp0_rice_decode_block(bad,sizeof bad,out+1,4,&n));
}
int main(int argc,char **argv)
{
    test_known_block();
    if(argc!=3){puts("known/malformed block checks PASS");return 0;}
    size_t np,nr;uint8_t *packed=load(argv[1],&np),*raw=load(argv[2],&nr);
    memory_t m={packed,np};wp0_rice_stream_t s;
    assert(wp0_rice_open(&s,memory_read,&m,(uint32_t)np));
    assert(s.total_samples*2u<=nr);
    uint32_t h=2166136261u,seen=0;
    while(s.next_block<s.block_count){
        int16_t guarded[WP0_PCM_BLOCK+2];guarded[0]=12345;
        guarded[WP0_PCM_BLOCK+1]=23456;unsigned n=0;
        assert(wp0_rice_next(&s,guarded+1,&n));
        for(unsigned i=0;i<n;i++){
            uint16_t v=(uint16_t)guarded[i+1];
            assert((v>>8)==raw[2u*seen]&&(v&255u)==raw[2u*seen+1u]);
            h=(h^(v>>8))*16777619u;h=(h^(v&255u))*16777619u;seen++;
        }
        assert(guarded[0]==12345&&guarded[WP0_PCM_BLOCK+1]==23456);
    }
    assert(seen==s.total_samples&&s.offset==np&&h==s.pcm_fnv1a);
    unsigned n=123;int16_t tmp[WP0_PCM_BLOCK];
    assert(!wp0_rice_next(&s,tmp,&n));
    /* A declared file length that omits the last byte must fail on open. */
    assert(!wp0_rice_open(&s,memory_read,&m,(uint32_t)np-1u));
    printf("full lossless byte comparison PASS: %u samples, %zu packed bytes\n",seen,np);
    free(packed);free(raw);return 0;
}
