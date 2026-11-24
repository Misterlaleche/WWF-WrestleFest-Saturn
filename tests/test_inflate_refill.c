/* Public streaming ABI differential. Both decoders run after every identical
 * input/output offer; the reference differs only in its pinned inffast.c. */
#include <assert.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "zlib.h"

extern int wf_reference_inflateInit2_(z_streamp,int,const char *,int);
extern int wf_reference_inflate(z_streamp,int);
extern int wf_reference_inflateEnd(z_streamp);
extern int wf_reference_inflateGetDictionary(z_streamp,Bytef *,uInt *);
extern int wf_reference_inflateSetDictionary(z_streamp,const Bytef *,uInt);
extern int wf_reference_inflateUndermine(z_streamp,int);

enum { VALID=1, PERMISSIVE=2, TRUNCATED=4, DATA_ERROR=8, PREFIX=16 };
enum { MAX_OUT=65536, GUARD=32, SCHEDULES=9 };
typedef struct allocation_stats {unsigned calls,frees;size_t live;} allocation_stats_t;
typedef union allocation_header {
    long double alignment;
    struct {allocation_stats_t *owner;size_t bytes;} value;
} allocation_header_t;
typedef struct stream_case {
    char name[100];int wbits;unsigned flags,packed_bytes,plain_bytes,dict_bytes,end_bytes,first_in,first_out;
    uint8_t *packed,*plain,*dictionary;
} stream_case_t;
static uint8_t output[2][MAX_OUT+GUARD*2+8],dictionary[2][32768];
static uint64_t calls,runs,decoded_bytes,allocation_pairs,digest=UINT64_C(14695981039346656037);
static uint64_t refill_cases[2][15][4];

static void hash_bytes(const void *memory,size_t bytes)
{const uint8_t *p=memory;for(size_t i=0;i<bytes;i++)digest=(digest^p[i])*UINT64_C(1099511628211);}
static void hash_word(uint64_t word)
{for(unsigned i=0;i<8;i++){uint8_t b=(uint8_t)(word>>(i*8));hash_bytes(&b,1);}}
static voidpf allocate(voidpf opaque,uInt items,uInt size)
{
    allocation_stats_t *stats=opaque;assert(stats&&size&&items<=SIZE_MAX/size);
    size_t bytes=(size_t)items*size;
    allocation_header_t *h=calloc(1,sizeof *h+bytes+GUARD);assert(h);
    h->value.owner=stats;h->value.bytes=bytes;++stats->calls;stats->live+=bytes;
    memset((uint8_t *)(h+1)+bytes,0xb7,GUARD);return h+1;
}
static void release(voidpf opaque,voidpf memory)
{
    if(!memory)return;
    allocation_header_t *h=(allocation_header_t *)memory-1;
    allocation_stats_t *stats=opaque;assert(h->value.owner==stats);
    for(unsigned i=0;i<GUARD;i++)assert(((uint8_t *)memory)[h->value.bytes+i]==0xb7);
    assert(stats->live>=h->value.bytes);stats->live-=h->value.bytes;++stats->frees;free(h);
}
static uint32_t word(FILE *file)
{
    uint8_t p[4];assert(fread(p,1,4,file)==4);
    return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;
}
static uint8_t *blob(FILE *file,unsigned size)
{uint8_t *p=malloc(size?size:1);assert(p);assert(fread(p,1,size,file)==size);return p;}
static unsigned input_offer(unsigned schedule,unsigned step,unsigned bytes)
{
    static const unsigned small[]={1,2,5,6,7,31,257,4096};
    if(schedule==3)return small[step%8];
    if(schedule==6)return 257u+(step*113u)%4096u;
    if(schedule==8&&bytes<2048u)return 1;
    return bytes;
}
static unsigned output_offer(unsigned schedule,unsigned step)
{
    static const unsigned threshold[]={257,258,259,0,512,1023};
    static const unsigned varied[]={300,50000,313,123,32768,8191};
    if(schedule==1)return 31;
    if(schedule==2)return threshold[step%6];
    if(schedule==3)return step%3u==0u?258:1024;
    if(schedule==4)return 32768;
    if(schedule==5)return 8191;
    if(schedule==6)return varied[step%6];
    if(schedule==7)return 1024;
    return MAX_OUT;
}
static int flush_offer(unsigned schedule,unsigned step)
{
    static const int flushes[]={Z_BLOCK,Z_NO_FLUSH,Z_TREES,Z_NO_FLUSH};
    if(schedule==7)return flushes[step%4];
    return schedule==0?Z_FINISH:Z_NO_FLUSH;
}
static void equal_public(const z_stream *a,const z_stream *b,
                         const allocation_stats_t *aa,const allocation_stats_t *bb)
{
    assert(a->total_in==b->total_in&&a->total_out==b->total_out);
    assert(a->avail_in==b->avail_in&&a->avail_out==b->avail_out&&a->next_in==b->next_in);
    assert(a->adler==b->adler&&a->data_type==b->data_type&&a->reserved==b->reserved);
    assert((a->msg==NULL)==(b->msg==NULL));if(a->msg)assert(!strcmp(a->msg,b->msg));
    assert(a->opaque==aa&&b->opaque==bb&&a->zalloc==allocate&&b->zalloc==allocate);
    assert(a->zfree==release&&b->zfree==release);
    assert(aa->calls==bb->calls&&aa->frees==bb->frees&&aa->live==bb->live);
}
static void run_case(const stream_case_t *test,unsigned schedule)
{
    z_stream a,b;allocation_stats_t aa={0},bb={0};memset(&a,0,sizeof a);memset(&b,0,sizeof b);
    a.zalloc=b.zalloc=allocate;a.zfree=b.zfree=release;a.opaque=&aa;b.opaque=&bb;
    a.reserved=b.reserved=UINT32_C(0x98764321);
    int x=inflateInit2_(&a,test->wbits,ZLIB_VERSION,sizeof a);
    int y=wf_reference_inflateInit2_(&b,test->wbits,ZLIB_VERSION,sizeof b);
    assert(x==y&&x==Z_OK);
    unsigned valid=!!(test->flags&VALID),prefix=!!(test->flags&PREFIX);
    if(test->flags&PERMISSIVE){
        x=inflateUndermine(&a,1);y=wf_reference_inflateUndermine(&b,1);assert(x==y);
#ifdef INFLATE_ALLOW_INVALID_DISTANCE_TOOFAR_ARRR
        assert(x==Z_OK);valid=1;
#else
        assert(x==Z_DATA_ERROR);
#endif
    }
    if(test->dict_bytes&&test->wbits<0){
        x=inflateSetDictionary(&a,test->dictionary,test->dict_bytes);
        y=wf_reference_inflateSetDictionary(&b,test->dictionary,test->dict_bytes);assert(x==y&&x==Z_OK);
    }
    a.next_in=b.next_in=test->packed;
    unsigned offered=0,produced=0,stalls=0,step;
    for(step=0;step<1000000u;step++){
        unsigned take=input_offer(schedule,step,test->packed_bytes);
        if(step==0&&test->first_in)take=test->first_in;
        if(take>test->packed_bytes-offered)take=test->packed_bytes-offered;
        a.avail_in+=take;b.avail_in+=take;offered+=take;
        unsigned capacity=output_offer(schedule,step),alignment=step&3u;
        if(step==0&&test->first_out)capacity=test->first_out;
        assert(capacity<=MAX_OUT);
        unsigned prefix_bytes=GUARD+alignment,buffer_bytes=prefix_bytes+capacity+GUARD;
        memset(output[0],0xa5,buffer_bytes);memset(output[1],0xa5,buffer_bytes);
        a.next_out=output[0]+prefix_bytes;b.next_out=output[1]+prefix_bytes;
        a.avail_out=b.avail_out=capacity;
        uLong before_in=a.total_in,before_out=a.total_out;
        x=inflate(&a,flush_offer(schedule,step));y=wf_reference_inflate(&b,flush_offer(schedule,step));
        if(x!=y){fprintf(stderr,"status mismatch %s schedule=%u call=%u: %d/%d\n",test->name,schedule,step,x,y);assert(0);}
        equal_public(&a,&b,&aa,&bb);
        unsigned wrote=capacity-a.avail_out;
        assert(a.next_out==output[0]+prefix_bytes+wrote&&b.next_out==output[1]+prefix_bytes+wrote);
        assert(!memcmp(output[0],output[1],buffer_bytes));
        for(unsigned i=0;i<prefix_bytes;i++)assert(output[0][i]==0xa5);
        for(unsigned i=prefix_bytes+capacity;i<buffer_bytes;i++)assert(output[0][i]==0xa5);
        if(valid||prefix){
            assert(produced<=test->plain_bytes&&wrote<=test->plain_bytes-produced);
            assert(!memcmp(output[0]+prefix_bytes,test->plain+produced,wrote));
        }
        uInt an=99999,bn=99999;
        int ad=inflateGetDictionary(&a,dictionary[0],&an),bd=wf_reference_inflateGetDictionary(&b,dictionary[1],&bn);
        assert(ad==bd&&ad==Z_OK&&an==bn&&an<=32768u&&!memcmp(dictionary[0],dictionary[1],an));
        hash_word((uint32_t)x);hash_word(a.total_in);hash_word(a.total_out);hash_word(a.avail_in);
        hash_word(a.avail_out);hash_word(a.adler);hash_word(a.data_type);hash_word(an);
        if(a.msg)hash_bytes(a.msg,strlen(a.msg));
        hash_bytes(output[0]+prefix_bytes,wrote);
        produced+=wrote;decoded_bytes+=wrote;++calls;
        assert(produced<1048576u);
        if(x==Z_NEED_DICT){
            assert(test->dict_bytes);uint8_t *wrong=malloc(test->dict_bytes);assert(wrong);
            memcpy(wrong,test->dictionary,test->dict_bytes);wrong[0]^=0x80;
            int ar=inflateSetDictionary(&a,wrong,test->dict_bytes);
            int br=wf_reference_inflateSetDictionary(&b,wrong,test->dict_bytes);assert(ar==br&&ar==Z_DATA_ERROR);free(wrong);
            ar=inflateSetDictionary(&a,test->dictionary,test->dict_bytes);
            br=wf_reference_inflateSetDictionary(&b,test->dictionary,test->dict_bytes);assert(ar==br&&ar==Z_OK);
            equal_public(&a,&b,&aa,&bb);continue;
        }
        if(x==Z_STREAM_END||(x<0&&x!=Z_BUF_ERROR))break;
        if(a.total_in==before_in&&a.total_out==before_out&&offered==test->packed_bytes&&capacity){
            if(++stalls==2)break;
        }else stalls=0;
    }
    assert(step<1000000u);
    /* NEED_DICT returns before zlib's total_in accounting, so the public
     * input pointer is the independent full-stream consumption oracle. */
    if(valid)assert(x==Z_STREAM_END&&produced==test->plain_bytes&&
                    a.next_in==test->packed+test->end_bytes);
    else if(test->flags&(DATA_ERROR|PERMISSIVE))assert(x==Z_DATA_ERROR);
    else if(test->flags&TRUNCATED)assert(x==Z_OK||x==Z_BUF_ERROR);
    x=inflateEnd(&a);y=wf_reference_inflateEnd(&b);assert(x==y&&x==Z_OK);
    assert(!a.state&&!b.state&&!aa.live&&!bb.live&&aa.calls==aa.frees&&bb.calls==bb.frees);
    allocation_pairs+=aa.calls;++runs;
}

#ifdef WF_TEST_INSTRUMENT
/* Test-only observations never influence decoding or expected outcomes. */
void wf_test_refill_observe(unsigned site,unsigned bits,uintptr_t input)
{
    assert(site<2u&&bits<15u);
    ++refill_cases[site][bits][input&3u];
}
#endif

int main(int argc,char **argv)
{
    assert(argc==2);FILE *file=fopen(argv[1],"rb");assert(file);
    char magic[8];assert(fread(magic,1,8,file)==8&&!memcmp(magic,"WFZW0001",8));
    unsigned cases=word(file);assert(cases&&cases<1000u);
    for(unsigned i=0;i<cases;i++){
        stream_case_t test;memset(&test,0,sizeof test);unsigned name_bytes=word(file);
        test.wbits=(int32_t)word(file);test.flags=word(file);test.packed_bytes=word(file);
        test.plain_bytes=word(file);test.dict_bytes=word(file);test.end_bytes=word(file);
        test.first_in=word(file);test.first_out=word(file);
        assert(name_bytes<sizeof test.name&&test.packed_bytes<1048576u&&test.plain_bytes<1048576u&&test.dict_bytes<=32768u);
        assert(fread(test.name,1,name_bytes,file)==name_bytes);
        test.packed=blob(file,test.packed_bytes);test.plain=blob(file,test.plain_bytes);test.dictionary=blob(file,test.dict_bytes);
        for(unsigned schedule=0;schedule<SCHEDULES;schedule++)run_case(&test,schedule);
        free(test.packed);free(test.plain);free(test.dictionary);
    }
    assert(fgetc(file)==EOF);assert(!fclose(file));
#ifdef WF_TEST_INSTRUMENT
    for(unsigned site=0;site<2;site++)for(unsigned bits=0;bits<15;bits++){
        uint64_t count=0;for(unsigned alignment=0;alignment<4;alignment++)count+=refill_cases[site][bits][alignment];
        if(!count)fprintf(stderr,"Uncovered refill site=%u bits=%u\n",site,bits);
        assert(count);
    }
#endif
    printf("{\"cases\":%u,\"runs\":%"PRIu64",\"calls\":%"PRIu64",\"output_bytes_compared\":%"PRIu64
           ",\"allocation_pairs\":%"PRIu64",\"digest\":\"%016"PRIx64"\",\"refill_cases\":[",
           cases,runs,calls,decoded_bytes,allocation_pairs,digest);
    for(unsigned site=0;site<2;site++){
        printf("%s[",site?",":"");
        for(unsigned bits=0;bits<15;bits++){
            printf("%s[",bits?",":"");
            for(unsigned alignment=0;alignment<4;alignment++)printf("%s%"PRIu64,alignment?",":"",refill_cases[site][bits][alignment]);
            putchar(']');
        }
        putchar(']');
    }
    puts("]}");return 0;
}
