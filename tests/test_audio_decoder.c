/* Exact PCM and decoder state at every upload boundary, including failures.
 * Include the real adapter to exercise its private chunk decoder. The oracle
 * computes each OKI nibble arithmetically, independently of production LUTs. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wf_jo_audio.c"

static decoder_t expected, *observed;
static const jo_sound *expected_allocation;
static unsigned remaining, writes, fail_at, cases;
static uint64_t samples, digest=1469598103934665603ull;
static uint32_t random_state=0x41287895u;

static uint32_t random32(void)
{random_state^=random_state<<13;random_state^=random_state>>17;random_state^=random_state<<5;return random_state;}
static void same_state(const decoder_t *a,const decoder_t *b)
{
    assert(a->offset==b->offset&&a->frames==b->frames&&a->decoded==b->decoded);
    assert(a->signal==b->signal&&a->index==b->index);
}
static uint16_t reference_sample(decoder_t *d)
{
    static const int steps_ref[49]={16,17,19,21,23,25,28,31,34,37,41,45,50,55,60,66,73,80,88,97,107,118,130,143,157,173,190,209,230,253,279,307,337,371,408,449,494,544,598,658,724,796,876,963,1060,1166,1282,1411,1552};
    static const int shifts_ref[8]={-1,-1,-1,-1,2,4,6,8};
    unsigned b=pack[d->offset+d->decoded/2u];
    unsigned n=d->decoded%2u?b%16u:b/16u;
    int step=steps_ref[d->index];
    int delta=step/8+((n&1)?step/4:0)+((n&2)?step/2:0)+((n&4)?step:0);
    d->signal+=(n&8)?-delta:delta;
    if(d->signal>2047)d->signal=2047;
    if(d->signal< -2048)d->signal= -2048;
    d->index+=shifts_ref[n&7];
    if(d->index<0)d->index=0;
    if(d->index>48)d->index=48;
    ++d->decoded;return (uint16_t)(d->signal*16);
}
bool jo_audio_write_resident(const jo_sound *allocation,unsigned at,const void *data,unsigned bytes)
{
    const uint8_t *actual=data;
    unsigned n=remaining>256u?256u:remaining;
    assert(allocation==expected_allocation&&at==expected.decoded*2u);
    assert(n&&bytes==n*2u&&data==stage);
    for(unsigned i=0;i<n;++i){
        unsigned value=reference_sample(&expected);
        assert(actual[2u*i]==value/256u&&actual[2u*i+1u]==value%256u);
        digest^=actual[2u*i];digest*=1099511628211ull;
        digest^=actual[2u*i+1u];digest*=1099511628211ull;
    }
    samples+=n;remaining-=n;++writes;
    /* The complete chunk is visible even if its upload reports failure. */
    same_state(observed,&expected);
    return !fail_at||writes!=fail_at;
}
static void exercise(decoder_t *d,unsigned count,unsigned failure)
{
    jo_sound allocation={0};
    expected=*d;observed=d;expected_allocation=&allocation;
    remaining=count<d->frames-d->decoded?count:d->frames-d->decoded;
    unsigned expected_writes=(remaining+255u)/256u;
    fail_at=failure;writes=0;
    bool success=decode_to(d,&allocation,count);
    assert(success==(!failure||failure>expected_writes));
    assert(writes==(success?expected_writes:failure));
    assert(!success||!remaining);same_state(d,&expected);++cases;
}
int main(int argc,char **argv)
{
    assert(argc==2);FILE *f=fopen(argv[1],"rb");assert(f);
    assert(!fseek(f,0,SEEK_END));long length=ftell(f);assert(length>0);rewind(f);
    uint8_t *input=malloc((size_t)length);assert(input);
    assert(fread(input,1,(size_t)length,f)==(size_t)length);fclose(f);
    assert(wf_jo_audio_pack_valid(input,(size_t)length));
    pack=input;sample_offset=be32(input+12);decoder_tables();
    unsigned rows=0;
    for(unsigned row=1;row<SAMPLE_ROWS;++row){
        if(row==74)continue;
        decoder_t d=decoder_start(row);++rows;
        while(d.decoded<d.frames)exercise(&d,1u+random32()%800u,0);
        exercise(&d,0,0);exercise(&d,UINT32_MAX,0);
    }
    for(unsigned i=0;i<30000u;++i){
        unsigned row=1u+random32()%115u;if(row==74)row=73;
        decoder_t d=decoder_start(row);
        d.decoded=random32()%(d.frames+1u);
        d.signal=(int)(random32()%4096u)-2048;d.index=(int)(random32()%49u);
        unsigned n=i%31u==0?0u:i%37u==0?UINT32_MAX:random32()%900u;
        exercise(&d,n,i%4u);
    }
    printf("Audio decoder: rows=%u cases=%u samples=%llu digest=%016llx; PCM, chunk states, odd offsets, clamp and write failures PASS\n",rows,cases,(unsigned long long)samples,(unsigned long long)digest);
    free(input);return 0;
}
