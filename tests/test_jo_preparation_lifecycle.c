#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "wf_jo_assets.h"
#include "wf_jo_video.h"
#include "wf_r18_present.h"
#include "wf_saturn_yaul_backend.h"
#ifdef WF_TEST_SCALAR_CONSUME
int __wrap_wf_jo_backend_consume_prepared_rom_tiles(wf_saturn_yaul_backend_t *backend)
{(void)backend;return 0;}
#endif

extern void wf_jo_test_generation(uint32_t);
extern uint32_t wf_jo_test_binding_generation(void);
extern uint32_t wf_jo_test_demand_generation(void);
#ifdef WF_JO_ASSETS_TEST_WORKER
extern unsigned wf_test_assets_worker_submitted,wf_test_assets_worker_joined;
#endif
extern wf_status_t wf_jo_test_records(uint16_t,unsigned);
extern void wf_jo_test_stage_capacity(size_t);
extern wf_status_t __real_wf_jo_video_texture_protect(unsigned,uint16_t,wf_jo_texture_ref_t *,unsigned *);
static unsigned protects;
wf_status_t __wrap_wf_jo_video_texture_protect(unsigned logical,uint16_t source,wf_jo_texture_ref_t *ref,unsigned *hit)
{++protects;return __real_wf_jo_video_texture_protect(logical,source,ref,hit);}
static void *read_file(const char *path,size_t *size)
{
    FILE *f=fopen(path,"rb");assert(f);assert(!fseek(f,0,SEEK_END));
    long n=ftell(f);assert(n>0);rewind(f);void *p=malloc((size_t)n);assert(p);
    assert(fread(p,1,(size_t)n,f)==(size_t)n);assert(!fclose(f));*size=(size_t)n;return p;
}
static void submit(uint16_t first,unsigned count,unsigned reuse)
{
    uint32_t demand_before=wf_jo_test_demand_generation();
    unsigned before=protects;assert(wf_jo_test_records(first,count)==WF_OK);
    assert(reuse?protects==before:protects>before);
#ifdef WF_JO_ASSETS_TEST_WORKER
    /* demand_frame, post-consumption, and the existing common done label. */
    assert((uint32_t)(wf_jo_test_demand_generation()-demand_before)==3u);
#else
    assert(!demand_before&&!wf_jo_test_demand_generation());
#endif
}
int main(int argc,char **argv)
{
    assert(argc==3);size_t rn,pn;void *rom=read_file(argv[1],&rn),*pack=read_file(argv[2],&pn);
    uint8_t *cache=malloc(16384);assert(cache);
    assert(!wf_jo_assets_preparation_generation());
    assert(wf_jo_assets_bind(rom,rn,pack,pn,cache,16384)==WF_JO_ASSET_OK);
    uint32_t token=wf_jo_assets_preparation_generation();assert(token==1);
    const uint8_t *tile;assert(wf_jo_assets_tile_read(49,&tile)==WF_JO_ASSET_OK);
    assert(wf_jo_assets_preparation_generation()==token);
    void *loan=NULL;assert(wf_jo_assets_borrow_sprite_cache_tail(8192,&loan)==WF_JO_ASSET_OK);
    assert(wf_jo_assets_return_sprite_cache_tail(loan)==WF_JO_ASSET_OK);
    assert(wf_jo_assets_preparation_generation()==token);
    assert(wf_jo_assets_bind(NULL,0,NULL,0,NULL,0)==WF_JO_ASSET_ARGUMENT);
    assert(wf_jo_assets_preparation_generation()==token+1&&!wf_jo_assets_ready());
    assert(wf_jo_assets_bind(rom,rn,pack,pn,cache,16384)==WF_JO_ASSET_OK);
    assert(wf_jo_assets_preparation_generation()==token+2);

    wf_r18_present_init(NULL,NULL,0,NULL,0);
    submit(1000,256,0);submit(1000,1,0);submit(1000,1,1);
    void *backup=wf_r18_present_backup_workspace_begin(128);assert(backup);
    for(unsigned i=0;i<128;i++)((uint8_t *)backup)[i]=0xcc;
    wf_r18_present_backup_workspace_end();submit(1000,1,0);submit(1000,1,1);
    /* Finish replaces logical slot0 while begin is blocked. Next pending=0
     * and active/count match the old snapshot: only the cleared completion
     * proof prevents accidentally resurrecting source1000 for source60000. */
    wf_jo_test_stage_capacity(0);assert(wf_jo_test_records(60000,1)==WF_BLOCKED);
    wf_jo_test_stage_capacity(WF_JO_TEXTURE_STAGE_BYTES);submit(60000,1,0);submit(60000,1,1);
    /* An earlier failure leaves unresolved temporary IDs in the tile plan. */
    assert(wf_jo_test_records(40000,257)==WF_BLOCKED);submit(60000,1,0);submit(60000,1,1);
    wf_r18_present_init(NULL,NULL,0,NULL,0);submit(1000,1,0);submit(1000,1,1);
    assert(wf_jo_assets_bind(rom,rn,pack,pn,cache,16384)==WF_JO_ASSET_OK);
    submit(1000,1,0);submit(1000,1,1);

    /* The reuse token stops permanently at0, while the worker's established
     * generation still wraps normally. A later bind cannot revive an ABA. */
    wf_jo_test_generation(UINT32_MAX-1u);
    assert(wf_jo_assets_bind(NULL,0,NULL,0,NULL,0)==WF_JO_ASSET_ARGUMENT);
    assert(wf_jo_assets_preparation_generation()==UINT32_MAX);
    assert(wf_jo_assets_bind(NULL,0,NULL,0,NULL,0)==WF_JO_ASSET_ARGUMENT);
    assert(!wf_jo_assets_preparation_generation()&&!wf_jo_test_binding_generation());
    assert(wf_jo_assets_bind(rom,rn,pack,pn,cache,16384)==WF_JO_ASSET_OK);
    assert(!wf_jo_assets_preparation_generation()&&wf_jo_test_binding_generation()==1);
    submit(1000,1,0);submit(1000,1,0);
#ifdef WF_JO_ASSETS_TEST_WORKER
    assert(wf_test_assets_worker_submitted&&wf_test_assets_worker_submitted==wf_test_assets_worker_joined);
    puts("PASS private presenter worker: exactly three cleanup/generation increments per fresh or restored frame, balanced positive worker jobs");
#endif
    free(rom);free(pack);free(cache);
    puts("PASS preparation lifecycle: BUP, init, failed plans before/after finish, rebind, decode/loan stability, failed bind and permanent token-wrap disable");
    return 0;
}
