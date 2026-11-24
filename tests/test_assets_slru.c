/* Real decoder/pixels plus an independent block-keyed two-queue model.
 * The production change that must fail this gate is an incorrect promotion,
 * demotion, physical invalidation, wrap, or stale decoded pointer. */
#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../port/wf_jo_assets.c"

#define TEST_PROTECTED UINT32_C(0x80000000)
typedef struct model {
    unsigned maximum,active,enabled,clock,batch;
    int slots[256],where[2048];
    unsigned cold[256],hot[256],cn,hn,last[2048],age[2048],empty_age[256];
    uint8_t protected[2048];
} model_t;
static model_t model;
static uint8_t *fixture_pack,*fixture_rom,*fixture_raw,*fixture_scene;
static size_t fixture_pack_size,fixture_rom_size,fixture_raw_size,fixture_scene_size;
static uint8_t storage[256u*4096u+64u],other_storage[256u*4096u+64u],scene_arena[282624u];
static uint8_t *active_storage=storage;
static unsigned current_capacity,loan_active;
static uint64_t requests,checks,scenario_count;
static void *active_loan;

static uint8_t *load_file(const char *path,size_t *bytes)
{
    FILE *file=fopen(path,"rb");assert(file);assert(!fseek(file,0,SEEK_END));
    long n=ftell(file);assert(n>0);assert(!fseek(file,0,SEEK_SET));
    uint8_t *p=malloc((size_t)n);assert(p);assert(fread(p,1,(size_t)n,file)==(size_t)n);
    assert(!fclose(file));*bytes=(size_t)n;return p;
}
static void queue_remove(unsigned *queue,unsigned *length,unsigned block)
{
    unsigned i=0;while(i<*length&&queue[i]!=block)++i;assert(i<*length);
    memmove(queue+i,queue+i+1,(*length-i-1u)*sizeof *queue);--*length;
}
static void model_reset(unsigned capacity)
{
    memset(&model,0,sizeof model);model.maximum=model.active=capacity;
    model.enabled=capacity+(capacity+1u)/2u<=256u;model.batch=model.enabled?1u:0u;
    for(unsigned i=0;i<256;i++)model.slots[i]=-1;
    for(unsigned i=0;i<2048;i++)model.where[i]=-1;
}
static void model_forget(unsigned slot)
{
    int block=model.slots[slot];if(block<0)return;
    /* Legacy scene/replacement invalidation retains a slot's former age.
     * Tail loan/return invalidation explicitly zeroes it at those call sites. */
    model.empty_age[slot]=model.enabled?0:model.age[block];
    if(model.protected[block])queue_remove(model.hot,&model.hn,(unsigned)block);
    else queue_remove(model.cold,&model.cn,(unsigned)block);
    model.protected[block]=0;model.age[block]=model.last[block]=0;
    model.where[block]=-1;model.slots[slot]=-1;
}
static void model_demote(void)
{
    unsigned limit=model.enabled&&model.active>16u?model.active-16u:0u;
    while(model.hn>limit){
        unsigned block=model.hot[0];queue_remove(model.hot,&model.hn,block);
        model.protected[block]=0;model.cold[model.cn++]=block;model.age[block]=model.clock;
        /* A resize can demote several entries in one metadata event. They
         * share its recency; the declared tie rule is physical slot order,
         * not the order in which the old protected queue was drained. */
        unsigned at=model.cn-1u;
        while(at&&model.age[model.cold[at-1u]]==model.clock&&
              model.where[model.cold[at-1u]]>model.where[block]){
            model.cold[at]=model.cold[at-1u];--at;
        }
        model.cold[at]=block;
    }
}
static void model_wrap(void)
{
    model.cn=model.hn=0;
    memset(model.age,0,sizeof model.age);memset(model.last,0,sizeof model.last);
    memset(model.empty_age,0,sizeof model.empty_age);
    memset(model.protected,0,sizeof model.protected);
    for(unsigned slot=0;slot<model.active;slot++)if(model.slots[slot]>=0)
        model.cold[model.cn++]=(unsigned)model.slots[slot];
    model.clock=1;
}
static unsigned model_read(unsigned block)
{
    unsigned hit=model.where[block]>=0;
    if(!hit){
        unsigned slot=0;while(slot<model.active&&model.slots[slot]>=0)++slot;
        if(slot==model.active){assert(model.cn);slot=(unsigned)model.where[model.cold[0]];}
        model_forget(slot);model.slots[slot]=(int)block;model.where[block]=(int)slot;
        model.cold[model.cn++]=block;
    }
    model.clock=(model.clock+1u)&(model.enabled?UINT32_C(0x7fffffff):UINT32_MAX);
    if(!model.clock)model_wrap();
    if(model.protected[block])queue_remove(model.hot,&model.hn,block);
    else queue_remove(model.cold,&model.cn,block);
    if(model.enabled&&hit&&model.last[block]!=model.batch)model.protected[block]=1;
    if(model.protected[block])model.hot[model.hn++]=block;
    else model.cold[model.cn++]=block;
    model.age[block]=model.clock;model.last[block]=model.enabled?model.batch:0;
    model_demote();return !hit;
}
static void check_state(void)
{
    assert(cache_slots==model.active&&cache_max_slots==model.maximum&&cache_clock==model.clock);
    unsigned resident=0,protected=0;
    for(unsigned slot=0;slot<model.maximum;slot++){
        int block=model.slots[slot];assert(slot_block[slot]==(block<0?WF_NO_SLOT:(unsigned)block));
        uint32_t expected=block<0?model.empty_age[slot]:model.age[block]|(model.protected[block]?TEST_PROTECTED:0u);
        assert(slot_age[slot]==expected);
        if(block>=0){assert(slot<model.active);++resident;protected+=model.protected[block];}
        if(model.enabled){
            uint32_t pair=slot_age[model.maximum+(slot>>1)];
            unsigned stamp=slot&1u?pair>>16:pair&65535u;
            assert(stamp==(block<0?0:model.last[block]));
        }
    }
    for(unsigned block=0;block<2048;block++)
        assert(block_slot[block]==(model.where[block]<0?WF_NO_SLOT:(unsigned)model.where[block]));
    assert(resident==model.cn+model.hn&&protected==model.hn);
    assert(model.hn<=(model.enabled&&model.active>16u?model.active-16u:0u));
#ifdef WF_CACHE_PROTECTED
    assert(cache_protected_count==model.hn);
    assert(cache_batch_epoch==(model.enabled?model.batch:0u));
#endif
    for(unsigned i=0;i<32;i++)assert(active_storage[i]==0xa5&&
        active_storage[32u+current_capacity*4096u+i]==0xa5);
    if(active_loan)for(unsigned i=loan_active*4096u;i<current_capacity*4096u;i++)
        assert(active_storage[32u+i]==0x6d);
    ++checks;
}
static void bind_case(unsigned capacity)
{
    memset(active_storage,0xa5,sizeof storage);current_capacity=capacity;
    active_loan=NULL;loan_active=0;
    assert(!wf_jo_assets_bind(fixture_rom,fixture_rom_size,fixture_pack,fixture_pack_size,
                             active_storage+32u,capacity*4096u));
    model_reset(capacity);check_state();++scenario_count;
}
static void model_batch(void)
{
    if(model.enabled){
        if(++model.batch>65535u){memset(model.last,0,sizeof model.last);model.batch=1u;}
    }
}
static void batch_without_check(void)
{wf_jo_assets_demand_frame(NULL,NULL,0,0,NULL);model_batch();}
static void batch(void){batch_without_check();check_state();}
static void read_tile(unsigned block,unsigned tile_in_block)
{
    assert(block<2048&&tile_in_block<32);unsigned missed=model_read(block);
    wf_jo_asset_stats_t before,after;wf_jo_assets_get_stats(&before);
    const uint8_t *pixels=(void *)(uintptr_t)1;
    unsigned tile=block*32u+tile_in_block;
    assert(wf_jo_assets_tile_read((uint16_t)tile,&pixels)==0);
    wf_jo_assets_get_stats(&after);
    assert(after.block_misses-before.block_misses==missed);
    assert(after.block_hits-before.block_hits==!missed);
    if(pixels!=active_storage+32u+(unsigned)model.where[block]*4096u+tile_in_block*128u){
        unsigned actual_slot=(unsigned)((size_t)(pixels-cache_data)/4096u);
        fprintf(stderr,"slot mismatch request=%"PRIu64" max=%u active=%u block=%u expected=%d actual=%zu clock=%u/%u cold=%u hot=%u\n",
            requests,model.maximum,model.active,block,model.where[block],
            (size_t)(pixels-cache_data)/4096u,model.clock,cache_clock,model.cn,model.hn);
        fprintf(stderr,"prior competing ages: expected slot %d had %u; actual slot %u had %u\n",
            model.where[block],slot_age[model.where[block]],actual_slot,
            actual_slot<model.active&&model.slots[actual_slot]>=0?model.age[model.slots[actual_slot]]:UINT32_MAX);
    }
    assert(pixels==active_storage+32u+(unsigned)model.where[block]*4096u+tile_in_block*128u);
    assert(!memcmp(pixels,fixture_raw+tile*128u,128u));
    assert(!after.decode_failures);++requests;check_state();
}
static void borrow(unsigned active)
{
    assert(active>=2&&active<current_capacity&&!active_loan);
    assert(!wf_jo_assets_borrow_sprite_cache_tail((current_capacity-active)*4096u,&active_loan));
    assert(active_loan==active_storage+32u+active*4096u);
    for(unsigned i=active;i<model.maximum;i++){model_forget(i);model.empty_age[i]=0;}
    model.active=active;model_demote();loan_active=active;
    memset(active_loan,0x6d,(current_capacity-active)*4096u);check_state();
}
static void give_back(void)
{
    assert(active_loan);void *old=active_loan;active_loan=NULL;
    assert(!wf_jo_assets_return_sprite_cache_tail(old));
    for(unsigned i=model.active;i<model.maximum;i++){model_forget(i);model.empty_age[i]=0;}
    model.active=model.maximum;check_state();
}
static void scene_scratch_case(void)
{
    assert(!wf_jo_assets_preload_scenes(scene_arena,sizeof scene_arena));
    uint8_t actual[17];assert(!wf_jo_assets_file_read("R18RING.BIN",4096u,actual,sizeof actual));
    assert(!memcmp(actual,fixture_scene+4096u,sizeof actual));
    model_forget(0);model_forget(1);check_state();
}
static void semantic_red_case(void)
{
    bind_case(138);
    const uint8_t *pixels;
    for(unsigned i=0;i<138;i++)for(unsigned repeat=0;repeat<2;repeat++){
        assert(!wf_jo_assets_tile_read((uint16_t)(i*32u+repeat),&pixels));
        assert(!memcmp(pixels,fixture_raw+i*4096u+repeat*128u,128u));
    }
    wf_jo_assets_demand_frame(NULL,NULL,0,0,NULL);
    for(unsigned i=0;i<122;i++)assert(!wf_jo_assets_tile_read((uint16_t)(i*32u),&pixels));
    for(unsigned i=500;i<517;i++)assert(!wf_jo_assets_tile_read((uint16_t)(i*32u),&pixels));
    wf_jo_asset_stats_t before,after;wf_jo_assets_get_stats(&before);
    assert(!wf_jo_assets_tile_read(7,&pixels));wf_jo_assets_get_stats(&after);
    assert(!memcmp(pixels,fixture_raw+7u*128u,128u));
    /* A second-batch block survives a one-pass scan of the probation area. */
    assert(after.block_misses==before.block_misses);
    assert(after.block_hits==before.block_hits+1u);
}
static void random_state_cases(void)
{
    const unsigned capacities[]={2,16,17,32,69,138,169,170,171,256};
    uint32_t rng=UINT32_C(0x51a7b37);
    for(unsigned scenario=0;scenario<sizeof capacities/sizeof capacities[0];scenario++){
        unsigned capacity=capacities[scenario];bind_case(capacity);
        for(unsigned step=0;step<3200;step++){
            rng=rng*1664525u+1013904223u;
            if(step%7u==0)batch();
            unsigned block=(rng>>8)%430u;
            if(step%3u)block=(step/7u)%180u;
            read_tile(block,rng&31u);
            if(step==900&&capacity>2)borrow(capacity/2u<2u?2u:capacity/2u);
            if(step==1300&&active_loan)give_back();
            if(step==1800)scene_scratch_case();
        }
        cache_clock=model.clock=model.enabled?UINT32_C(0x7ffffffe):UINT32_C(0xfffffffe);
        read_tile(900,3);read_tile(901,4);read_tile(902,5);
        if(model.enabled){
            while(model.batch!=65535u)batch_without_check();
            check_state();read_tile(902,6);batch();assert(model.batch==1u);
            read_tile(903,6);batch();read_tile(903,7);
        }
    }
}
static unsigned callback_kind,callback_calls;
static int hostile_service(void *user)
{
    (void)user;++callback_calls;
    if(callback_kind==1)return -1;
    if(callback_kind==2){
        assert(!wf_jo_assets_bind(fixture_rom,fixture_rom_size,fixture_pack,fixture_pack_size,
                                 other_storage+32u,138u*4096u));
        return 0;
    }
    if(callback_kind==3){
        const uint8_t *pixels;
        for(unsigned block=100;block<250;block++)
            assert(!wf_jo_assets_tile_read((uint16_t)(block*32u),&pixels));
    }
    return 0;
}
static void failure_cases(void)
{
    for(unsigned kind=1;kind<=3;kind++){
        active_storage=storage;bind_case(138);
        memset(other_storage,0xa5,sizeof other_storage);
        callback_kind=kind;callback_calls=0;
        const uint16_t plan[]={0,32,64};const uint8_t touched[]={1,1,1},prepared[32]={0};
        wf_jo_assets_demand_frame(plan,touched,1,3,prepared);
        wf_jo_assets_set_service_hook(hostile_service,NULL);
        const uint8_t *pixels=(void *)(uintptr_t)1;
        assert(wf_jo_assets_tile_read(0,&pixels)==WF_JO_ASSET_SERVICE&&pixels==NULL);
        assert(callback_calls==1);wf_jo_assets_demand_end();wf_jo_assets_set_service_hook(NULL,NULL);
        if(kind==2){active_storage=other_storage;model_reset(138);check_state();read_tile(0,0);}
        else {
            /* Callback recursion may deliberately change replacement order;
             * every surviving map still names exact canonical block bytes. */
            unsigned protected=0;
            for(unsigned slot=0;slot<cache_slots;slot++){
                unsigned b=slot_block[slot];
                if(b!=WF_NO_SLOT){assert(block_slot[b]==slot);
                    assert(!memcmp(cache_data+slot*4096u,fixture_raw+b*4096u,4096u));
                    protected+=!!(slot_age[slot]&TEST_PROTECTED);}
                else assert(!(slot_age[slot]&TEST_PROTECTED));
            }
#ifdef WF_CACHE_PROTECTED
            assert(protected==cache_protected_count);
#else
            (void)protected;
#endif
        }
    }
    active_storage=storage;bind_case(138);batch();read_tile(1,0);batch();read_tile(1,1);
    unsigned offset=read32(fixture_pack+WF_PACK_HEADER+1000u*4u);
    uint8_t original=fixture_pack[offset];fixture_pack[offset]^=255u;
    const uint8_t *pixels=(void *)(uintptr_t)1;
    wf_jo_asset_stats_t before,after;wf_jo_assets_get_stats(&before);
    assert(wf_jo_assets_tile_read((uint16_t)(1000u*32u),&pixels)==WF_JO_ASSET_DECODE&&pixels==NULL);
    wf_jo_assets_get_stats(&after);assert(after.decode_failures==before.decode_failures+1u);
    assert(block_slot[1000]==WF_NO_SLOT&&block_slot[1]!=WF_NO_SLOT);
    fixture_pack[offset]=original;
    /* A fresh bind clears every failed-slot policy and ownership trace. */
    bind_case(138);read_tile(1000,0);
    assert(wf_jo_assets_bind(NULL,0,NULL,0,NULL,0)==WF_JO_ASSET_ARGUMENT);
    assert(!wf_jo_assets_ready());pixels=(void *)(uintptr_t)1;
    assert(wf_jo_assets_tile_read(0,&pixels)==WF_JO_ASSET_UNLOADED&&pixels==NULL);
    bind_case(171);batch();read_tile(1,0);batch();read_tile(1,1);
}
static void paired_case(void)
{
    bind_case(138);
    const uint16_t plan[]={320,641,322,963,644,1285};
    const uint8_t touched[]={1,1,1,1,1,1},prepared[32]={0};
#ifdef WF_ASSETS_WORKER
    uint32_t generation=demand_generation;
    extern unsigned wf_test_assets_worker_submitted,wf_test_assets_worker_joined;
    unsigned submitted=wf_test_assets_worker_submitted;
#endif
    wf_jo_assets_demand_frame(plan,touched,1,6,prepared);model_batch();check_state();
    for(unsigned i=0;i<6;i++)read_tile(plan[i]/32u,plan[i]&31u);
    wf_jo_assets_demand_end();check_state();
#ifdef WF_ASSETS_WORKER
    assert(demand_generation==generation+2u&&!worker_pending&&!worker_ready&&!demand_active);
    assert(wf_test_assets_worker_submitted>submitted);
    assert(wf_test_assets_worker_submitted==wf_test_assets_worker_joined);
#endif
}
int main(int argc,char **argv)
{
    assert(argc==4);fixture_pack=load_file(argv[1],&fixture_pack_size);
    fixture_rom=load_file(argv[2],&fixture_rom_size);fixture_raw=load_file(argv[3],&fixture_raw_size);
    fixture_scene=load_file("R18RING.BIN",&fixture_scene_size);
    assert(fixture_raw_size==8388608u&&fixture_scene_size>4113u&&read32(fixture_pack+8)==4096u);
    semantic_red_case();random_state_cases();failure_cases();paired_case();wf_jo_assets_demand_end();
    printf("PASS SLRU canonical requests=%"PRIu64" state_checks=%"PRIu64" scenarios=%"PRIu64"\n",
           requests,checks,scenario_count);
    free(fixture_pack);free(fixture_rom);free(fixture_raw);free(fixture_scene);return 0;
}
