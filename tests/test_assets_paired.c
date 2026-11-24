/* Shared corpus plus deterministic/deferred/pthread worker lifetime checks. */
#define main legacy_assets_main
#include "test_assets.c"
#undef main
#include <pthread.h>

static void (*pending_worker)(void);
static pthread_t main_thread,worker_thread;
static unsigned schedule_mode,worker_submits,worker_joins;
static uint64_t digest=UINT64_C(14695981039346656037);
static unsigned hook_calls,hook_action;
static const uint8_t *edge_rom,*edge_pack,*edge_raw;
static size_t edge_rn,edge_pn;
static uint8_t *edge_cache;
static size_t edge_cn;

static void hash_bytes(const void *data,size_t bytes) {
    const uint8_t *p=data;
    while(bytes--)digest=(digest^*p++)*UINT64_C(1099511628211);
}
static void *thread_worker(void *unused) {(void)unused;pending_worker();return NULL;}
void wf_jo_assets_test_submit(void (*callback)(void)) {
    assert(pthread_equal(pthread_self(),main_thread));assert(!pending_worker);
    pending_worker=callback;++worker_submits;
    if(schedule_mode==0)callback();
    if(schedule_mode==2)assert(pthread_create(&worker_thread,NULL,thread_worker,NULL)==0);
}
void wf_jo_assets_test_join(void) {
    assert(pthread_equal(pthread_self(),main_thread));assert(pending_worker);
    if(schedule_mode==1)pending_worker();
    if(schedule_mode==2)assert(pthread_join(worker_thread,NULL)==0);
    pending_worker=NULL;++worker_joins;
}
#ifdef WF_ASSETS_REFERENCE
void wf_jo_assets_demand_frame(const uint16_t *s,const uint8_t *t,unsigned e,unsigned n,const uint8_t *p)
{(void)s;(void)t;(void)e;(void)n;(void)p;}
void wf_jo_assets_demand_end(void) {}
#endif
static int paired_hook(void *unused) {
    (void)unused;assert(pthread_equal(pthread_self(),main_thread));assert(!pending_worker);
    wf_jo_asset_stats_t s;wf_jo_assets_get_stats(&s);hash_bytes(&s,sizeof s);++hook_calls;
    if(hook_action==1){
        const uint8_t *pixels=NULL;wf_jo_assets_demand_end();
        assert(wf_jo_assets_tile_read(64,&pixels)==0);
        assert(!memcmp(pixels,edge_raw+64u*128u,128u));
    }else if(hook_action==2){
        assert(wf_jo_assets_bind(edge_rom,edge_rn,edge_pack,edge_pn,edge_cache,edge_cn)==0);
    }
    return hook_action==3;
}
static void bind_edges(void) {
    wf_jo_assets_demand_end();assert(!pending_worker);
    memset(edge_cache,0xa5,edge_cn);hook_action=hook_calls=0;
    assert(wf_jo_assets_bind(edge_rom,edge_rn,edge_pack,edge_pn,edge_cache,edge_cn)==0);
    wf_jo_assets_set_service_hook(paired_hook,NULL);
}
static void plan(unsigned count) {
    uint16_t ids[3]={0,32,64};uint8_t touched[3]={1,1,1},prepared[32]={0};
    assert(count<=3);wf_jo_assets_demand_frame(ids,touched,1,count,prepared);
    memset(ids,0xff,sizeof ids);memset(touched,0,sizeof touched);memset(prepared,0xff,sizeof prepared);
}
static void workload(void) {
    bind_edges();uint32_t random=0x13579bdu;
    for(unsigned frame=0;frame<328u;++frame){
        uint16_t ids[256],wanted[256];uint8_t touched[256],prepared[32]={0};
        for(unsigned i=0;i<256u;++i){
            random=random*1664525u+1013904223u;
            ids[i]=wanted[i]=frame<256u?(uint16_t)(frame*256u+i):(uint16_t)(random>>8);
            touched[i]=1;
            if(frame>=320u){if(i%3u==0)touched[i]=0;if(i%5u==0)prepared[i>>3]|=(uint8_t)(1u<<(i&7u));}
        }
        uint8_t active[256];
        for(unsigned i=0;i<256u;++i)active[i]=(uint8_t)(touched[i]&&!(prepared[i>>3]&(1u<<(i&7u))));
        wf_jo_assets_demand_frame(ids,touched,1,256,prepared);
        memset(ids,0,sizeof ids);memset(touched,0,sizeof touched);memset(prepared,0xff,sizeof prepared);
        for(unsigned i=0;i<256u;++i)if(active[i]){
            const uint8_t *pixels=NULL;int status=wf_jo_assets_tile_read(wanted[i],&pixels);
            assert(status==0&&pixels>=edge_cache&&pixels+128u<=edge_cache+edge_cn);
            assert(!memcmp(pixels,edge_raw+(size_t)wanted[i]*128u,128u));
            uint32_t offset=(uint32_t)(pixels-edge_cache);hash_bytes(&offset,sizeof offset);hash_bytes(pixels,128u);
        }
        wf_jo_assets_demand_end();assert(!pending_worker);
        wf_jo_asset_stats_t stats;wf_jo_assets_get_stats(&stats);hash_bytes(&stats,sizeof stats);
    }
    printf("EQUIVALENCE %016llx callbacks=%u\n",(unsigned long long)digest,hook_calls);
#if !defined(WF_ASSETS_REFERENCE) && !defined(WF_ASSETS_SERIAL_FALLBACK)
    assert(worker_submits>1000u&&worker_joins==worker_submits);
#endif
}
static void lifetime_edges(void) {
    const uint8_t *pixels=NULL;wf_jo_asset_stats_t stats;bind_edges();plan(2);
    assert(wf_jo_assets_tile_read(0,&pixels)==0&&!memcmp(pixels,edge_raw,128));
    assert(!pending_worker&&hook_calls==1);wf_jo_assets_get_stats(&stats);
    assert(stats.block_misses==1&&stats.block_hits==0); /* Private future block is not published. */
    wf_jo_assets_demand_end();assert(wf_jo_assets_tile_read(32,&pixels)==0);
    wf_jo_assets_get_stats(&stats);assert(stats.block_misses==2&&stats.block_hits==0);
    bind_edges();plan(3);hook_action=1;
    assert(wf_jo_assets_tile_read(0,&pixels)==0&&!memcmp(pixels,edge_raw,128));
    assert(hook_calls==1&&!pending_worker);wf_jo_assets_get_stats(&stats);assert(stats.block_misses==2);
#ifndef WF_ASSETS_REFERENCE
    bind_edges();plan(3);hook_action=2;
    assert(wf_jo_assets_tile_read(0,&pixels)==WF_JO_ASSET_SERVICE&&pixels==NULL);
    assert(!pending_worker);
#endif
    bind_edges();plan(3);hook_action=3;
    assert(wf_jo_assets_tile_read(0,&pixels)==WF_JO_ASSET_SERVICE);
    wf_jo_assets_demand_end();assert(!pending_worker);wf_jo_assets_set_service_hook(NULL,NULL);
    assert(wf_jo_assets_tile_read(0,&pixels)==0&&!memcmp(pixels,edge_raw,128));
    bind_edges();plan(2);assert(wf_jo_assets_tile_read(0,&pixels)==0);
    void *loan=NULL;assert(wf_jo_assets_borrow_sprite_cache_tail(4096,&loan)==0);
    assert(!pending_worker);assert(wf_jo_assets_return_sprite_cache_tail(loan)==0);
    assert(wf_jo_assets_bind(NULL,0,NULL,0,NULL,0)!=0&&!pending_worker);
    bind_edges();
    /* Both the current and future stream can fail independently. */
    for(unsigned broken=0;broken<2u;++broken){
        uint8_t *bad=malloc(edge_pn);assert(bad);memcpy(bad,edge_pack,edge_pn);
        bad[be32(bad+32u+(broken+1u)*4u)-1u]^=1u;
        assert(wf_jo_assets_bind(edge_rom,edge_rn,bad,edge_pn,edge_cache,edge_cn)==0);
        hook_calls=0;wf_jo_assets_set_service_hook(paired_hook,NULL);plan(2);
        pixels=NULL;int status=wf_jo_assets_tile_read(0,&pixels);
        if(broken){assert(status==0&&hook_calls==1);wf_jo_assets_get_stats(&stats);assert(stats.decode_failures==0);}
        else assert(status==WF_JO_ASSET_DECODE&&pixels==NULL&&hook_calls==0);
        assert(!pending_worker);
        if(broken){pixels=NULL;assert(wf_jo_assets_tile_read(32,&pixels)==WF_JO_ASSET_DECODE&&pixels==NULL);}
        wf_jo_assets_get_stats(&stats);assert(stats.decode_failures==1);
        wf_jo_assets_demand_end();assert(wf_jo_assets_bind(edge_rom,edge_rn,edge_pack,edge_pn,edge_cache,edge_cn)==0);
        free(bad);
    }
    puts("PASS paired lifetime: source copies, hooks, cancellation, reentry, rebind, loans and independent corrupt streams");
}
static void length_edges(void) {
    const unsigned lengths[]={0,4095,4096,4097,8192,8193,4096,4096,4096};
    for(unsigned test=0;test<sizeof lengths/sizeof lengths[0];++test){
        unsigned n=lengths[test],stream_bytes=n+11u;uint8_t *stream=malloc(stream_bytes+1u);assert(stream);
        stream[0]=0x78;stream[1]=0x01;stream[2]=1;
        stream[3]=(uint8_t)n;stream[4]=(uint8_t)(n>>8);stream[5]=(uint8_t)~n;stream[6]=(uint8_t)(~n>>8);
        memset(stream+7,0x5a,n);put32(stream+7+n,checksum(stream+7,n));
        if(test==6)--stream_bytes;
        if(test==7)stream[stream_bytes++]=0;
        if(test==8)stream[stream_bytes-1u]^=1;
        uint32_t first=be32(edge_pack+32),end=be32(edge_pack+36),blocks=be32(edge_pack+16);
        size_t pn=edge_pn-(end-first)+stream_bytes;uint8_t *pack=malloc(pn);assert(pack);
        memcpy(pack,edge_pack,first);memcpy(pack+first,stream,stream_bytes);
        memcpy(pack+first+stream_bytes,edge_pack+end,edge_pn-end);
        for(unsigned i=1;i<=blocks;++i)put32(pack+32u+i*4u,be32(edge_pack+32u+i*4u)+stream_bytes-(end-first));
        put32(pack+24,(uint32_t)pn);put32(pack+28,checksum(pack+32,first-32));
        uint8_t guard[8192+64];memset(guard,0xa5,sizeof guard);
        assert(wf_jo_assets_bind(edge_rom,edge_rn,pack,pn,guard+32,8192)==0);
        unsigned calls=0;wf_jo_assets_set_service_hook(count_service,&calls);
        for(unsigned repeat=0;repeat<2;++repeat){
            const uint8_t *pixels=NULL;int status=wf_jo_assets_tile_read(0,&pixels);
            if(test==2){assert(status==0&&pixels);for(unsigned i=0;i<128;++i)assert(pixels[i]==0x5a);}
            else{assert(status==WF_JO_ASSET_DECODE&&pixels==NULL);for(unsigned i=0;i<8192;++i)assert(guard[32+i]==0xa5);}
        }
        assert(calls==(test==2));
        for(unsigned i=0;i<32;++i)assert(guard[i]==0xa5&&guard[8192+32+i]==0xa5);
        wf_jo_assets_demand_end();free(stream);free(pack);
    }
    puts("PASS exact output lengths: short, exact, extra, full staging, overflow, truncated/trailing input and checksum guards");
}
int main(int argc,char **argv) {
    assert(argc==5);schedule_mode=(unsigned)atoi(argv[4]);assert(schedule_mode<3);
    main_thread=pthread_self();assert(legacy_assets_main(4,argv)==0);
    edge_pack=load(argv[1],&edge_pn);edge_rom=load(argv[2],&edge_rn);size_t raw_bytes;
    edge_raw=load(argv[3],&raw_bytes);assert(raw_bytes==WF_JO_SPRITE_BYTES&&be32(edge_pack+8)==4096);
    edge_cn=138u*4096u;uint8_t *guard=malloc(edge_cn+64);assert(guard);memset(guard,0xa5,edge_cn+64);edge_cache=guard+32;
    workload();lifetime_edges();length_edges();wf_jo_assets_demand_end();
    for(unsigned i=0;i<32;++i)assert(guard[i]==0xa5&&guard[edge_cn+32+i]==0xa5);
    assert(worker_submits==worker_joins&&!pending_worker);
    printf("PASS worker scheduling mode%u: %u launches/%u joins\n",schedule_mode,worker_submits,worker_joins);
    free(guard);free((void *)edge_pack);free((void *)edge_rom);free((void *)edge_raw);return 0;
}
