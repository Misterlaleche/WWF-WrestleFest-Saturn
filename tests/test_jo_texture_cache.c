#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "wf_jo_texture_cache.h"

static wf_jo_texture_cache_t cache;
static uint16_t payload[WF_JO_GPU_TILES];
static uint32_t random_state=0xb54d1937u;
static uint32_t random_word(void)
{random_state^=random_state<<13;random_state^=random_state>>17;random_state^=random_state<<5;return random_state;}
static unsigned misses,hits,frames;
static unsigned fill(unsigned logical,uint16_t source,unsigned shown_valid,uint32_t shown)
{
    unsigned slot;
    int result=wf_jo_cache_resolve(&cache,logical,source,1,shown_valid,shown,&slot);
    assert(result==WF_JO_CACHE_MISS);
    assert(wf_jo_cache_filled(&cache,logical,source,1,0,16));
    return slot;
}

static void lifetime_and_reset(void)
{
    wf_jo_cache_init(&cache);
    assert(wf_jo_cache_begin(&cache,2));
    unsigned a=fill(0,77,0,0),b=fill(1,88,0,0);
    assert(a!=b&&wf_jo_cache_publish(&cache,UINT32_MAX-1u));
    assert(!wf_jo_cache_reusable(&cache,a,0,0));
    assert(!wf_jo_cache_reusable(&cache,a,1,UINT32_MAX-1u));
    assert(wf_jo_cache_repeat(&cache,UINT32_MAX));
    assert(!wf_jo_cache_reusable(&cache,a,1,UINT32_MAX));
    assert(wf_jo_cache_invalidate_sources(&cache));
    assert(!wf_jo_cache_reusable(&cache,a,1,UINT32_MAX));
    assert(wf_jo_cache_repeat(&cache,0));
    assert(!wf_jo_cache_reusable(&cache,a,1,0));
    assert(wf_jo_cache_reusable(&cache,a,1,1));
    assert(cache.repeat[0]==a&&cache.repeat[1]==b);
    assert(wf_jo_cache_begin(&cache,1));
    unsigned slot;
    assert(wf_jo_cache_resolve(&cache,0,99,1,1,1,&slot)==WF_JO_CACHE_MISS);
    assert(!wf_jo_cache_can_publish(&cache));
    assert(!wf_jo_cache_publish(&cache,2));
    wf_jo_cache_abort(&cache);
    assert(wf_jo_cache_repeat(&cache,2));
    assert(cache.entries[a].last_tag==2&&cache.entries[b].last_tag==2);
}

static void prepass_guards(void)
{
    wf_jo_cache_init(&cache);assert(wf_jo_cache_begin(&cache,2));
    unsigned a=fill(0,101,0,0),b=fill(1,202,0,0);
    assert(wf_jo_cache_publish(&cache,9));
    assert(wf_jo_cache_begin(&cache,1));
    assert(wf_jo_cache_protect(&cache,0,101));
    assert(!wf_jo_cache_protect(&cache,0,303)); /* Absent relabel. */
    assert(!wf_jo_cache_protect(&cache,0,202)); /* Resident relabel. */
    unsigned slot;
    assert(wf_jo_cache_resolve(&cache,0,101,1,1,9,&slot)==WF_JO_CACHE_HIT&&slot==a);
    assert(wf_jo_cache_resolve(&cache,0,303,1,1,9,&slot)==WF_JO_CACHE_ERROR);
    assert(wf_jo_cache_resolve(&cache,0,0,0,1,9,&slot)==WF_JO_CACHE_ERROR);
    wf_jo_cache_abort(&cache);
    assert(wf_jo_cache_begin(&cache,1));
    assert(wf_jo_cache_resolve(&cache,0,303,1,1,9,&slot)==WF_JO_CACHE_MISS);
    assert(!wf_jo_cache_protect(&cache,0,303)); /* Reserved, not ready. */
    wf_jo_cache_abort(&cache);
    assert(cache.repeat[0]==a&&cache.repeat[1]==b);
}

static void exhausted_and_late_reference(void)
{
    wf_jo_cache_init(&cache);
    for(unsigned base=0;base<WF_JO_GPU_TILES;base+=256u){
        unsigned n=WF_JO_GPU_TILES-base;if(n>256u)n=256u;
        assert(wf_jo_cache_begin(&cache,n));
        for(unsigned i=0;i<n;i++)assert(fill(i,(uint16_t)(base+i),0,0)==base+i);
        assert(wf_jo_cache_publish(&cache,10));
    }
    assert(cache.allocated==WF_JO_GPU_TILES);
    assert(wf_jo_cache_begin(&cache,2));
    assert(wf_jo_cache_protect(&cache,1,0));
    unsigned slot;
    assert(wf_jo_cache_resolve(&cache,0,65535u,1,1,10,&slot)==WF_JO_CACHE_BLOCKED);
    assert(cache.frame[0]==WF_JO_GPU_NONE&&cache.frame[1]==0);
    assert(wf_jo_cache_resolve(&cache,0,65535u,1,1,11,&slot)==WF_JO_CACHE_MISS);
    assert(slot!=0);assert(wf_jo_cache_filled(&cache,0,65535u,1,0,16));
    assert(wf_jo_cache_resolve(&cache,1,0,1,1,11,&slot)==WF_JO_CACHE_HIT&&slot==0);
    assert(wf_jo_cache_publish(&cache,12));
}

static void hash_wrap_removal(void)
{
    uint16_t keys[4];unsigned count=0;
    for(unsigned source=0;source<65536u&&count<4u;source++)
        if(wf_jo_cache_hash((uint16_t)source)==WF_JO_GPU_INDEX-1u)keys[count++]=(uint16_t)source;
    assert(count==4u);
    for(unsigned remove=0;remove<4u;remove++){
        wf_jo_cache_init(&cache);
        for(unsigned i=0;i<4u;i++){
            cache.entries[i].source=keys[i];
            unsigned at=wf_jo_cache_find(&cache,keys[i]);
            assert(at==(WF_JO_GPU_INDEX-1u+i)%WF_JO_GPU_INDEX);
            cache.index[at]=(uint16_t)(i+1u);
        }
        wf_jo_cache_remove(&cache,wf_jo_cache_find(&cache,keys[remove]));
        for(unsigned i=0;i<4u;i++)
            assert(cache.index[wf_jo_cache_find(&cache,keys[i])]==(i==remove?0u:i+1u));
    }
}

typedef struct live_frame {
    uint32_t tag;
    uint16_t slots[256],source[256],count;
    unsigned valid;
} live_frame_t;
static live_frame_t live[8];

static void randomized_content(void)
{
    wf_jo_cache_init(&cache);memset(live,0,sizeof live);
    for(unsigned frame=0;frame<2048u;frame++){
        uint32_t tag=UINT32_MAX-15u+frame,shown=tag-3u;
        unsigned shown_valid=frame>=3u;
        for(unsigned j=0;j<8u;j++)if(live[j].valid&&shown_valid&&(int32_t)(shown-live[j].tag)>0)
            live[j].valid=0;
        live_frame_t next={0};next.tag=tag;next.valid=1;
        next.count=(uint16_t)(random_word()%257u);
        unsigned base=random_word();
        for(unsigned i=0;i<next.count;i++)next.source[i]=(uint16_t)(base+i*127u);
        assert(wf_jo_cache_begin(&cache,next.count));
        for(unsigned i=0;i<next.count;i++)assert(wf_jo_cache_protect(&cache,i,next.source[i]));
        for(unsigned i=0;i<next.count;i++){
            unsigned slot;int result=wf_jo_cache_resolve(&cache,i,next.source[i],1,shown_valid,shown,&slot);
            assert(result==WF_JO_CACHE_HIT||result==WF_JO_CACHE_MISS);
            if(result==WF_JO_CACHE_MISS){
                for(unsigned j=0;j<8u;j++)if(live[j].valid)
                    for(unsigned k=0;k<live[j].count;k++)assert(live[j].slots[k]!=slot);
                payload[slot]=next.source[i];
                assert(wf_jo_cache_filled(&cache,i,next.source[i],1,0,16));++misses;
            }else ++hits;
            assert(payload[slot]==next.source[i]);next.slots[i]=(uint16_t)slot;
        }
        assert(wf_jo_cache_publish(&cache,tag));
        assert(!live[frame%8u].valid);live[frame%8u]=next;
        for(unsigned j=0;j<8u;j++)if(live[j].valid)
            for(unsigned k=0;k<live[j].count;k++)assert(payload[live[j].slots[k]]==live[j].source[k]);
        ++frames;
    }
}

int main(void)
{
    assert(sizeof(wf_jo_texture_entry_t)==8u);
    assert(sizeof cache+2048u<=50856u);
    lifetime_and_reset();prepass_guards();exhausted_and_late_reference();hash_wrap_removal();randomized_content();
    printf("PASS GPU cache: %u frames/%u misses/%u hits, fences/repeat/reset/wrap/exhaustion/prepass, metadata%zuB\n",
           frames,misses,hits,sizeof cache);
    return 0;
}
