/* Differential driver for the legacy (>170-slot) cache path. Both binaries
 * include the same driver and an independently pinned production source.
 * Compare every public result, chosen slot, mapping, counter and callback;
 * stale ages of invalid slots are deliberately not an acceptance criterion. */
#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef WF_FALLBACK_ASSETS_SOURCE
#error "Provide an exact production source using WF_FALLBACK_ASSETS_SOURCE"
#endif
#include WF_FALLBACK_ASSETS_SOURCE

static uint8_t *fpack,*from,*fraw,*fring;
static size_t npack,nrom,nraw,nring;
static uint8_t memory[256u*4096u+64u],alternate[256u*4096u+64u];
static uint8_t scene_memory[282624u],*owned=memory;
static unsigned capacity,hook_calls,hook_mode;
static uint64_t rows,reads,cases;
static const char *scenario;
static void *loan;
static unsigned loan_slots;

static uint64_t digest(const void *data,size_t n)
{
    uint64_t h=UINT64_C(1469598103934665603);const uint8_t *p=data;
    while(n--){h^=*p++;h*=UINT64_C(1099511628211);}return h;
}
static uint8_t *load_fixture(const char *path,size_t *bytes)
{
    FILE *f=fopen(path,"rb");assert(f);assert(!fseek(f,0,SEEK_END));
    long n=ftell(f);assert(n>0);assert(!fseek(f,0,SEEK_SET));
    uint8_t *p=malloc((size_t)n);assert(p);assert(fread(p,1,(size_t)n,f)==(size_t)n);
    assert(!fclose(f));*bytes=(size_t)n;return p;
}
static void guards(void)
{
    for(unsigned i=0;i<32u;i++)assert(owned[i]==0xa5&&owned[32u+capacity*4096u+i]==0xa5);
    if(loan)for(unsigned i=loan_slots*4096u;i<capacity*4096u;i++)assert(owned[32u+i]==0x6d);
    if(wf_jo_assets_ready()){
        for(unsigned i=0;i<cache_max_slots;i++)if(slot_block[i]!=WF_NO_SLOT){
            assert(i<cache_slots&&slot_block[i]<WF_BLOCKS&&block_slot[slot_block[i]]==i);
        }
        for(unsigned i=0;i<WF_BLOCKS;i++)if(block_slot[i]!=WF_NO_SLOT)
            assert(block_slot[i]<cache_slots&&slot_block[block_slot[i]]==i);
    }
}
static void record(const char *op,unsigned argument,int status,const uint8_t *p,size_t n)
{
    wf_jo_asset_stats_t s;wf_jo_assets_get_stats(&s);guards();
    intptr_t where=-1;
    if(p){assert(cache_data&&p>=cache_data&&p+n<=cache_data+cache_slots*4096u);where=p-cache_data;}
    printf("%"PRIu64",%u,%s,%s,%u,%d,%"PRIdPTR",%016"PRIx64
           ",%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%016"PRIx64",%016"PRIx64"\n",
           ++rows,capacity,scenario,op,argument,status,where,p?digest(p,n):0,
           s.block_hits,s.block_misses,s.decode_failures,s.cd_reads,s.cd_bytes,
           s.scene_block_decodes,s.scene_reads,hook_calls,cache_clock,cache_slots,
           !!wf_jo_assets_ready(),!!wf_jo_assets_scenes_ready(),
           digest(slot_block,sizeof slot_block),digest(block_slot,sizeof block_slot));
}
static int tile(unsigned block,unsigned within,int expected)
{
    const uint8_t *pixels=(const uint8_t *)(uintptr_t)1;
    int rc=wf_jo_assets_tile_read((uint16_t)(block*32u+within),&pixels);
    if(rc!=expected)fprintf(stderr,"unexpected status: %s capacity=%u block=%u rc=%d expected=%d\n",scenario,capacity,block,rc,expected);
    assert(rc==expected);
    if(!rc)assert(pixels&&!memcmp(pixels,fraw+block*4096u+within*128u,128u));
    else assert(!pixels);
    record("tile",block*32u+within,rc,pixels,pixels?128u:0u);++reads;return rc;
}
static void partial(unsigned offset,unsigned bytes,int expected)
{
    uint8_t out[10000];assert(bytes<=sizeof out);memset(out,0x95,sizeof out);
    int rc=wf_jo_assets_file_read("R18RING.BIN",offset,out,bytes);assert(rc==expected);
    if(!rc||rc==WF_JO_ASSET_SERVICE)assert(!memcmp(out,fring+offset,bytes));
    for(unsigned i=bytes;i<sizeof out;i++)assert(out[i]==0x95);
    record("scene",offset,rc,NULL,0);
    printf("bytes,%016"PRIx64"\n",digest(out,sizeof out));
}
static int callback(void *user)
{
    assert(user==&hook_mode);++hook_calls;record("callback",hook_mode,0,NULL,0);
    switch(hook_mode){
    case 1:return -1;
    case 2:tile(600u,3u,0);break;
    case 3:partial(4096u,17u,0);break;
    case 4:case 5:{
        loan=NULL;owned=alternate;memset(owned,0xa5,sizeof alternate);
        int rc=wf_jo_assets_bind(from,nrom,fpack,npack-(hook_mode==5u),owned+32u,capacity*4096u);
        assert(hook_mode==4u?!rc:rc!=0);record("callback-bind",hook_mode,rc,NULL,0);break;
    }
    case 6:{
        void *out=(void *)(uintptr_t)1;
        int rc=wf_jo_assets_borrow_sprite_cache_tail(4096u,&out);
        assert(rc==WF_JO_ASSET_ARGUMENT&&!out);record("callback-loan",0,rc,NULL,0);break;
    }
    default:break;
    }
    return 0;
}
static void hook(unsigned mode)
{hook_mode=mode;wf_jo_assets_set_service_hook(callback,&hook_mode);}
static void begin(unsigned count,const char *name,const uint8_t *pack)
{
    scenario=name;capacity=count;loan=NULL;owned=memory;hook_calls=0;
    memset(memory,0xa5,sizeof memory);++cases;
    assert(!wf_jo_assets_bind(from,nrom,pack,npack,memory+32u,count*4096u));
    assert(wf_jo_assets_sprite_cache_active_bytes()==count*4096u);
    record("bind",count,0,NULL,0);
}
static void scenes_ready(void)
{assert(!wf_jo_assets_preload_scenes(scene_memory,sizeof scene_memory));record("preload",0,0,NULL,0);}
static void fill(void)
{for(unsigned i=0;i<capacity;i++)tile(i,i%32u,0);}
static void demand(unsigned epoch,unsigned empty)
{
    uint16_t source[]={12800u,12832u,19200u};uint8_t touched[3],prepared[3]={0,1,0};
    memset(touched,(int)epoch,sizeof touched);
    wf_jo_assets_demand_frame(empty?NULL:source,empty?NULL:touched,epoch,empty?0u:3u,empty?NULL:prepared);
    record("demand",epoch,0,NULL,0);
}
static void failed_service_after_scene(unsigned n)
{
    begin(n,"scene-failed-service",fpack);scenes_ready();fill();
    tile(0,0,0);tile(1,0,0);partial(4096u,17u,0);
    hook(1);tile(400,0,WF_JO_ASSET_SERVICE);wf_jo_assets_set_service_hook(NULL,NULL);
    tile(401,0,0);tile(402,0,0);hook(0);tile(400,0,0);
}
static void ordinary_and_wrap(unsigned n)
{
    begin(n,"ordinary-and-wrap",fpack);hook(0);fill();
    for(unsigned i=0;i<4u*n;i++){
        if(!(i%37u))demand(i%255u+1u,i%74u==0);
        tile((i*97u+i/9u)%(n+73u),i%32u,0);
    }
    /* The injected transition is the only state write outside public APIs.
     * Resident ages remain real, so the original unsigned wrap path runs. */
    cache_clock=UINT32_MAX-3u;record("force-clock",0,0,NULL,0);
    for(unsigned i=0;i<n+20u;i++)tile((i*17u+13u)%(n+31u),i%32u,0);
    wf_jo_assets_demand_end();record("demand-end",0,0,NULL,0);
}
static void malformed(unsigned n)
{
    uint8_t *bad=malloc(npack);assert(bad);memcpy(bad,fpack,npack);
    unsigned end=read32(bad+32u+401u*4u);bad[end-1u]^=1u;
    begin(n,"malformed-stream",bad);hook(0);fill();
    tile(400,0,WF_JO_ASSET_DECODE);tile(400,0,WF_JO_ASSET_DECODE);
    tile(401,0,0);tile(0,0,0);tile(402,0,0);
    assert(!wf_jo_assets_bind(from,nrom,fpack,npack,owned+32u,n*4096u));
    record("recovery-bind",0,0,NULL,0);tile(400,0,0);
    int rc=wf_jo_assets_bind(from,nrom,fpack,npack-1u,owned+32u,n*4096u);
    assert(rc!=0);record("failed-bind",0,rc,NULL,0);tile(400,0,WF_JO_ASSET_UNLOADED);
    assert(!wf_jo_assets_bind(from,nrom,fpack,npack,owned+32u,n*4096u));
    record("recovery-bind",1,0,NULL,0);tile(401,0,0);free(bad);
}
static void loans(unsigned n)
{
    begin(n,"loans-and-return",fpack);scenes_ready();hook(0);fill();
    const unsigned active[]={170u,138u,2u};
    for(unsigned k=0;k<3u;k++){
        unsigned left=active[k];void *out=NULL;
        assert(!wf_jo_assets_borrow_sprite_cache_tail((n-left)*4096u,&out));
        assert(out==owned+32u+left*4096u);loan=out;loan_slots=left;
        memset(loan,0x6d,(n-left)*4096u);record("loan",left,0,NULL,0);
        assert(wf_jo_assets_sprite_cache_active_bytes()==left*4096u);
        void *other=(void *)(uintptr_t)1;
        int rc=wf_jo_assets_borrow_sprite_cache_tail(4096u,&other);
        assert(rc==WF_JO_ASSET_ARGUMENT&&!other);record("nested-loan",0,rc,NULL,0);
        for(unsigned i=0;i<left+17u;i++)tile((500u+i)%2048u,i%32u,0);
        partial(4096u,17u,0);demand(255u,0);
        tile(800,1u,0);tile(801,31u,0);tile(802,0u,0);
        rc=wf_jo_assets_return_sprite_cache_tail((uint8_t *)loan+1u);
        assert(rc==WF_JO_ASSET_ARGUMENT);record("wrong-return",0,rc,NULL,0);
        guards();assert(!wf_jo_assets_return_sprite_cache_tail(loan));loan=NULL;
        record("return",left,0,NULL,0);assert(wf_jo_assets_sprite_cache_active_bytes()==n*4096u);
        for(unsigned i=0;i<n-left+4u;i++)tile(900u+i,i%32u,0);
    }
    assert(!wf_jo_assets_borrow_sprite_cache_tail((n-138u)*4096u,&loan));
    void *old=loan;loan=NULL;
    int rc=wf_jo_assets_bind(from,nrom,fpack,npack-1u,owned+32u,n*4096u);
    assert(rc!=0);record("loan-failed-bind",0,rc,NULL,0);
    rc=wf_jo_assets_return_sprite_cache_tail(old);assert(rc==WF_JO_ASSET_UNLOADED);
    record("stale-return",0,rc,NULL,0);
    assert(!wf_jo_assets_bind(from,nrom,fpack,npack,owned+32u,n*4096u));
    rc=wf_jo_assets_return_sprite_cache_tail(old);assert(rc==WF_JO_ASSET_ARGUMENT);
    record("rebound-return",0,rc,NULL,0);tile(900,0,0);
}
static void reentry(unsigned n)
{
    for(unsigned mode=2u;mode<=6u;mode++){
        begin(n,"defensive-reentry",fpack);scenes_ready();fill();hook(mode);
        tile(400,0,(mode>=2u&&mode<=5u)?WF_JO_ASSET_SERVICE:0);
        wf_jo_assets_set_service_hook(NULL,NULL);
        if(mode==5u){
            assert(!wf_jo_assets_bind(from,nrom,fpack,npack,owned+32u,n*4096u));
            record("reentry-recover",mode,0,NULL,0);
        }
        tile(400,0,0);tile(600,3u,0);tile(401,0,0);
    }
}
int main(int argc,char **argv)
{
    assert(argc==4);fpack=load_fixture(argv[1],&npack);from=load_fixture(argv[2],&nrom);
    fraw=load_fixture(argv[3],&nraw);fring=load_fixture("R18RING.BIN",&nring);
    assert(nraw==WF_JO_SPRITE_BYTES&&nrom==WF_JO_ROM_BYTES&&read32(fpack+8u)==4096u);
    assert(nring==114918u);
    const unsigned sizes[]={171u,256u};
    for(unsigned i=0;i<2u;i++){
        unsigned n=sizes[i];failed_service_after_scene(n);ordinary_and_wrap(n);
        malformed(n);loans(n);reentry(n);
    }
    wf_jo_assets_demand_end();free(fpack);free(from);free(fraw);free(fring);
    fprintf(stderr,"PASS driver: %"PRIu64" scenarios, %"PRIu64" reads, %"PRIu64" state records\n",cases,reads,rows);
    return 0;
}
