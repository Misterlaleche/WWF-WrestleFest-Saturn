/* Focused private-plan boundary gate. The unchanged real-asset paired gate
 * separately exercises actual decoder/worker/service lifetimes. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#ifndef WF_DEMAND_IMPLEMENTATION
#define WF_DEMAND_IMPLEMENTATION "../port/wf_jo_assets.c"
#endif
#include WF_DEMAND_IMPLEMENTATION

static unsigned joins,cases,old_generation,join_epoch;
static uint16_t *join_sources;
static uint8_t *join_touched,*join_prepared;
static uint64_t digest=UINT64_C(14695981039346656037);
static void hash_word(uint32_t value)
{for(unsigned i=0;i<4;i++){digest=(digest^(value&255u))*UINT64_C(1099511628211);value>>=8;}}
void wf_jo_assets_test_submit(void (*callback)(void))
{(void)callback;assert(!"Demand formation must not launch a decoder");}
void wf_jo_assets_test_join(void)
{
    assert(worker_pending&&demand_count==256u&&demand_cursor==17u&&demand_active);
    assert(demand_generation==old_generation);++joins;
    if(join_sources){join_sources[0]=0xbeefu;join_touched[0]=(uint8_t)join_epoch;join_prepared[0]&=254u;}
    worker.completed_generation=worker_expected;
}
static void check(unsigned count,unsigned epoch,unsigned pattern,unsigned guard)
{
    unsigned allocation=count<=256u?count:256u,bytes=(allocation+7u)/8u;
    uint16_t *sources=malloc((allocation?allocation:1u)*sizeof *sources);assert(sources);
    uint8_t *touched=malloc(allocation?allocation:1u),*prepared=malloc(bytes?bytes:1u);
    assert(touched&&prepared);uint32_t random=0x9132u+cases;
    for(unsigned i=0;i<allocation;i++){
        sources[i]=(uint16_t)(i*129u+cases*7u);
        touched[i]=(uint8_t)(pattern<2u||((i+cases)%5u)?epoch:(epoch^0x55u));
    }
    for(unsigned i=0;i<bytes;i++){
        random=random*1664525u+1013904223u;
        const uint8_t fixed[6]={0,255,85,170,1,128};
        prepared[i]=pattern<6u?fixed[pattern]:(uint8_t)(random>>16);
    }
    for(unsigned i=0;i<256u;i++)demand_tiles[i]=(uint16_t)(0x7000u+i);
    static uint8_t placeholder;rom_image=sprite_pack=&placeholder;
    cache_slots=138u;sprite_block_bytes=4096u;servicing=0;
    demand_count=256u;demand_cursor=17u;demand_active=1u;
    demand_generation=old_generation=cases%37u?cases+1u:UINT32_MAX;
    worker_pending=cases&1u;worker_ready=1u;worker_expected=53u;
    worker_binding=binding_generation=97u;worker_plan=demand_generation;
    worker.completed_generation=0;worker_serial=101u;
    join_sources=(worker_pending&&allocation&&pattern==7u&&!guard)?sources:NULL;
    join_touched=touched;join_prepared=prepared;join_epoch=epoch;
    const uint16_t *s=sources;const uint8_t *t=touched,*p=prepared;
    switch(guard){
        case 1:s=NULL;break;case 2:t=NULL;break;case 3:p=NULL;break;
        case 4:epoch=0;break;case 5:epoch=256;break;case 6:count=257;break;
        case 7:servicing=1;break;case 8:rom_image=NULL;break;
        case 9:sprite_pack=NULL;break;case 10:cache_slots=0;break;
        case 11:sprite_block_bytes=8192;break;
    }
    unsigned expected_joins=joins+worker_pending;
    wf_jo_assets_demand_frame(s,t,epoch,count,p);
    assert(joins==expected_joins&&!worker_pending&&!worker_ready);
    assert(demand_generation==(uint32_t)(old_generation+1u)&&!demand_cursor);
    assert(binding_generation==97u&&worker_serial==101u&&worker_expected==53u);
    unsigned wanted=0;
    if(!guard)for(unsigned i=0;i<count;i++)
        if(touched[i]==epoch&&!(prepared[i/8u]&(1u<<(i%8u))))
            assert(demand_tiles[wanted++]==sources[i]);
    assert(demand_count==wanted&&demand_active==(wanted!=0));
    for(unsigned i=wanted;i<256u;i++)assert(demand_tiles[i]==0x7000u+i);
    hash_word(demand_generation);hash_word(demand_count);hash_word(demand_active);hash_word(joins);
    for(unsigned i=0;i<256u;i++)hash_word(demand_tiles[i]);
    /* Inputs are copied, and cleanup does not clear/consume a stale tail. */
    memset(sources,0,(allocation?allocation:1u)*sizeof *sources);
    memset(touched,0,allocation?allocation:1u);memset(prepared,255,bytes?bytes:1u);
    wf_jo_assets_demand_end();assert(!demand_count&&!demand_cursor&&!demand_active);
    assert(!worker_pending&&!worker_ready&&joins==expected_joins);
    free(sources);free(touched);free(prepared);++cases;
}
int main(void)
{
    const unsigned epochs[3]={1u,127u,255u};
    for(unsigned n=0;n<=256u;n++)for(unsigned p=0;p<8u;p++)
        for(unsigned e=0;e<3u;e++)check(n,epochs[e],p,0);
    for(unsigned guard=1;guard<=11;guard++)for(unsigned count=0;count<=256;count+=128)
        check(count,1u,6u,guard);
    printf("PASS demand boundaries cases=%u joins=%u digest=%016llx\n",
           cases,joins,(unsigned long long)digest);return 0;
}
