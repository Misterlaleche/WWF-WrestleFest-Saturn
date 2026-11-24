#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "wf_jo_texture_cache.h"

static wf_jo_texture_cache_t normal,fast;
static uint32_t random_state=0xb39f1205u;
static unsigned comparisons,restores,frames;
static uint64_t digest=UINT64_C(14695981039346656037);
static uint32_t random32(void)
{random_state^=random_state<<13;random_state^=random_state>>17;random_state^=random_state<<5;return random_state;}
static void bytes(const void *a,const void *b,size_t n)
{
    assert(!memcmp(a,b,n));const uint8_t *p=a;
    for(size_t i=0;i<n;i++)digest=(digest^p[i])*UINT64_C(1099511628211);
}
static void equal(void)
{
#define SAME(member) bytes(&normal.member,&fast.member,sizeof normal.member)
    SAME(entries);SAME(index);SAME(valid);SAME(recent);SAME(pinned);SAME(ready);
    SAME(frame);SAME(repeat);SAME(allocated);SAME(hand);SAME(frame_count);
    SAME(repeat_count);SAME(building);SAME(repeat_valid);
#undef SAME
    ++comparisons;
}
static void remember(wf_jo_texture_cache_t *c,const void *owner,uint32_t generation,const uint8_t *active)
{
#ifndef TEST_BASELINE
    (void)wf_jo_cache_remember_preparation(c,owner,generation,active);
#else
    (void)c;(void)owner;(void)generation;(void)active;
#endif
}
static unsigned restore(const void *owner,uint32_t generation,unsigned count,const uint8_t *active)
{
#ifndef TEST_BASELINE
    if(wf_jo_cache_preparation_available(&fast,owner,generation,count)&&
       wf_jo_cache_restore_preparation(&fast,owner,generation,active)){++restores;return 1;}
#else
    (void)owner;(void)generation;(void)count;(void)active;
#endif
    return 0;
}
static void fill(unsigned i,uint16_t source,uint32_t shown,unsigned show_valid)
{
    unsigned a=99999,b=99999;
    int x=wf_jo_cache_resolve(&normal,i,source,1,show_valid,shown,&a);
    int y=wf_jo_cache_resolve(&fast,i,source,1,show_valid,shown,&b);
    assert(x==y&&a==b);
    if(x==WF_JO_CACHE_MISS){
        uint8_t top=(uint8_t)(source%5u),height=(uint8_t)(source%12u);
        assert(wf_jo_cache_filled(&normal,i,source,1,top,height));
        assert(wf_jo_cache_filled(&fast,i,source,1,top,height));
    }else assert(x==WF_JO_CACHE_HIT);
}
static void snapshot(unsigned count,const uint16_t *sources,const uint8_t *active,
                     const void *owner,uint32_t generation,uint32_t tag,unsigned allow)
{
    assert(wf_jo_cache_begin(&normal,count));assert(wf_jo_cache_begin(&fast,count));
    unsigned reused=allow?restore(owner,generation,count,active):0;
    for(unsigned i=0;i<count;i++)if(wf_jo_cache_bit(active,i)){
        assert(wf_jo_cache_protect(&normal,i,sources[i]));
        if(!reused)assert(wf_jo_cache_protect(&fast,i,sources[i]));
    }
    equal();
    for(unsigned i=0;i<count;i++)if(wf_jo_cache_bit(active,i))fill(i,sources[i],tag-1u,1);
    assert(wf_jo_cache_publish(&normal,tag));assert(wf_jo_cache_publish(&fast,tag));
    remember(&normal,owner,generation,active);remember(&fast,owner,generation,active);
    equal();++frames;
}
static void boundaries(void)
{
    uint16_t sources[256];uint8_t active[32]={0};
    const void *owner=(const void *)&sources,*other=(const void *)&active;
    for(unsigned i=0;i<256;i++)sources[i]=(uint16_t)(1000u+i);
    wf_jo_cache_init(&normal);wf_jo_cache_init(&fast);
    for(unsigned count=0;count<=256;count++){
        memset(active,0,sizeof active);
        for(unsigned i=0;i<count;i++)if(i%3u)wf_jo_cache_set(active,i);
        snapshot(count,sources,active,owner,9,1000u+count*2u,0);
        snapshot(count,sources,active,owner,9,1001u+count*2u,1);
    }
    /* A long aborted builder leaves an exposed tail not overwritten by the
     * next shorter begin/publish. Restore must not copy repeat's stale tail. */
    memset(active,255,sizeof active);snapshot(256,sources,active,owner,9,1600,0);
    assert(wf_jo_cache_begin(&normal,256));assert(wf_jo_cache_begin(&fast,256));
    fill(200,60000,1601,1);wf_jo_cache_abort(&normal);wf_jo_cache_abort(&fast);
    memset(active,0,sizeof active);active[0]=3;
    snapshot(2,sources,active,owner,9,1602,0);snapshot(2,sources,active,owner,9,1603,1);
    /* Foreign owner/generation and token0 never authorize a restoration. */
    assert(wf_jo_cache_begin(&normal,2));assert(wf_jo_cache_begin(&fast,2));
    assert(!restore(other,9,2,active));assert(!restore(owner,10,2,active));
    assert(!restore(owner,0,2,active));assert(!restore(owner,9,1,active));
    uint8_t changed[32]={1};assert(!restore(owner,9,2,changed));
    /* A successful protect invalidates the one-use candidate before any
     * attempt to reapply the previous complete state. */
    assert(wf_jo_cache_protect(&normal,0,sources[0]));assert(wf_jo_cache_protect(&fast,0,sources[0]));
    assert(!restore(owner,9,2,active));equal();
    wf_jo_cache_abort(&normal);wf_jo_cache_abort(&fast);
    assert(wf_jo_cache_repeat(&normal,UINT32_MAX));assert(wf_jo_cache_repeat(&fast,UINT32_MAX));
    assert(wf_jo_cache_begin(&normal,2));assert(wf_jo_cache_begin(&fast,2));
    assert(!restore(owner,9,2,active));wf_jo_cache_abort(&normal);wf_jo_cache_abort(&fast);
    snapshot(2,sources,active,owner,UINT32_MAX,0,0);
    assert(wf_jo_cache_repeat(&normal,1));assert(wf_jo_cache_repeat(&fast,1));
    snapshot(2,sources,active,owner,UINT32_MAX,2,1);
    assert(wf_jo_cache_invalidate_sources(&normal));assert(wf_jo_cache_invalidate_sources(&fast));
    snapshot(2,sources,active,owner,UINT32_MAX,3,0);
    /* Ready contains an unconsumed prepared slot: caller's dirty does not,
     * so this frame cannot become a reusable backend preparation. */
#ifndef TEST_BASELINE
    assert(!wf_jo_cache_remember_preparation(&fast,owner,UINT32_MAX,changed));
#endif
    equal();
}
static void pressure(void)
{
    uint16_t sources[256];uint8_t active[32];
    const void *owner=(const void *)&sources;
    wf_jo_cache_init(&normal);wf_jo_cache_init(&fast);
    unsigned count=0;uint32_t generation=1;
    for(unsigned frame=0;frame<2400;frame++){
        unsigned same=frame&&frame%4u;
        if(!same){
            count=random32()%257u;memset(active,0,sizeof active);
            unsigned base=random32()%64000u;
            for(unsigned i=0;i<count;i++){
                sources[i]=(uint16_t)(base+i);
                if(random32()%5u)wf_jo_cache_set(active,i);
            }
        }
        if(frame%127u==126u){
            assert(wf_jo_cache_invalidate_sources(&normal));assert(wf_jo_cache_invalidate_sources(&fast));
            ++generation;same=0;
        }
        snapshot(count,sources,active,owner,generation,frame+100u,same);
        if(frame%29u==28u){
            assert(wf_jo_cache_repeat(&normal,frame+100u));assert(wf_jo_cache_repeat(&fast,frame+100u));equal();
        }
        if(frame%73u==72u){
            assert(wf_jo_cache_begin(&normal,17));assert(wf_jo_cache_begin(&fast,17));
            for(unsigned i=0;i<17;i++)fill(i,(uint16_t)(65000u+i),frame+100u,1);
            wf_jo_cache_abort(&normal);wf_jo_cache_abort(&fast);equal();
        }
    }
}
int main(void)
{
    boundaries();pressure();
#ifndef TEST_BASELINE
    assert(restores>1900u);
#endif
    printf("PASS reusable cache legacy-state differential frames=%u comparisons=%u digest=%016llx restores=%u metadata=%zu\n",
           frames,comparisons,(unsigned long long)digest,restores,sizeof fast);
    return 0;
}
