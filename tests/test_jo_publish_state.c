/* Compare every public cache byte with a frozen C26 header. Catch missing
 * guards, wrong tags/ready/repeat/preparation state and sparse-tail writes. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "wf_jo_texture_cache.h"
#include "reference_cache.h"
static wf_jo_texture_cache_t seed,checked,fast;
static reference_wf_jo_texture_cache_t old;
static unsigned cases,invalid;
static void compare(uint32_t tag,int expected)
{
    checked=fast=seed;memcpy(&old,&seed,sizeof old);
    assert(reference_wf_jo_cache_publish(&old,tag)==expected);
    assert(wf_jo_cache_publish(&checked,tag)==expected);
    assert(!memcmp(&old,&checked,sizeof checked));
    if(expected){
        assert(wf_jo_cache_can_publish(&fast));
        assert(wf_jo_cache_publish_validated(&fast,tag));
        assert(!memcmp(&old,&fast,sizeof fast));
        assert(!fast.building&&fast.repeat_valid&&fast.repeat_count==seed.frame_count);
        for(unsigned i=0;i<seed.frame_count;i++){
            assert(fast.repeat[i]==seed.frame[i]);
            if(seed.frame[i]!=WF_JO_GPU_NONE)assert(fast.entries[seed.frame[i]].last_tag==tag);
        }
    }else{assert(!memcmp(&seed,&checked,sizeof seed));++invalid;}
    ++cases;
}
int main(void)
{
    _Static_assert(sizeof seed==sizeof old,"reference byte layout");
    for(unsigned count=0;count<=256u;count++)for(unsigned variant=0;variant<8u;variant++){
        wf_jo_cache_init(&seed);
        /* Poison unconsumed tails; neither continuation may clear/copy them. */
        memset(seed.frame,0xa5,sizeof seed.frame);memset(seed.repeat,0x5a,sizeof seed.repeat);
        assert(wf_jo_cache_begin(&seed,count));unsigned first=UINT32_MAX;
        for(unsigned i=0;i<count;i++)if((i+variant)%4u){
            unsigned slot;int result=wf_jo_cache_resolve(&seed,i,(uint16_t)(i+100u),variant&1u,0,0,&slot);
            assert(result==WF_JO_CACHE_MISS);
            assert(wf_jo_cache_filled(&seed,i,(uint16_t)(i+100u),variant&1u,(uint8_t)(i%5u),(uint8_t)(i%12u)));
            if(first==UINT32_MAX)first=i;
        }
        uint32_t tag=variant<4u?variant:UINT32_MAX-(variant-4u);
        compare(tag,1);
        if(first!=UINT32_MAX){
            wf_jo_cache_clear(seed.ready,first);compare(tag,0);
        }
        seed.building=0;compare(tag,0);
    }
    printf("PASS publication full-state: %u cases, %u checked rejections\n",cases,invalid);
}
