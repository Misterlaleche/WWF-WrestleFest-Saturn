#include "wf_jo_assets.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint32_t be32(const uint8_t *p) {
    return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 |
           (uint32_t)p[2] << 8 | p[3];
}
static void put32(uint8_t *p, uint32_t v) {
    p[0]=(uint8_t)(v>>24);p[1]=(uint8_t)(v>>16);
    p[2]=(uint8_t)(v>>8);p[3]=(uint8_t)v;
}
static uint32_t checksum(const uint8_t *p, size_t n) {
    uint32_t a=1,b=0;
    while(n--){a=(a+*p++)%65521u;b=(b+a)%65521u;}
    return b<<16|a;
}
static uint8_t *load(const char *path, size_t *bytes) {
    FILE *f=fopen(path,"rb");assert(f);
    assert(fseek(f,0,SEEK_END)==0);long size=ftell(f);assert(size>0);
    assert(fseek(f,0,SEEK_SET)==0);
    uint8_t *p=malloc((size_t)size);assert(p);
    assert(fread(p,1,(size_t)size,f)==(size_t)size);fclose(f);
    *bytes=(size_t)size;return p;
}
static int count_service(void *user) {
    unsigned *calls=user;wf_jo_asset_stats_t stats;
    wf_jo_assets_get_stats(&stats);++*calls;
    assert(stats.block_misses>=*calls);return 0;
}
static int fail_service(void *user) {(void)user;return -1;}
typedef struct scene_service_check {
    const uint8_t *actual,*expected;size_t bytes;unsigned calls,fail;
} scene_service_check_t;
static int scene_service(void *user) {
    scene_service_check_t *check=user;
    assert(memcmp(check->actual,check->expected,check->bytes)==0);
    ++check->calls;return check->fail?-1:0;
}
static void cache_loan_edges(const uint8_t *rom,size_t rn,const uint8_t *pack,size_t pn,
                             const uint8_t *raw,uint8_t *scene_arena) {
    const size_t full=196608u,loan_bytes=172032u;
    unsigned block_bytes=be32(pack+8),tiles_per_block=block_bytes/128u;
    unsigned full_slots=(unsigned)(full/block_bytes),active_slots=(unsigned)((full-loan_bytes)/block_bytes);
    uint8_t *guard=malloc(full+64u);assert(guard);memset(guard,0xa5,full+64u);
    uint8_t *cache=guard+32u;void *loan=NULL,*other=NULL;const uint8_t *pixels;
    wf_jo_asset_stats_t before,after;
    assert(wf_jo_assets_bind(rom,rn,pack,pn,cache,full)==0);
    assert(wf_jo_assets_preload_scenes(scene_arena,282624u)==0);
    assert(wf_jo_assets_sprite_cache_active_bytes()==full);
    for(unsigned i=0;i<full_slots;i++)assert(wf_jo_assets_tile_read((uint16_t)(i*tiles_per_block),&pixels)==0);
    assert(wf_jo_assets_borrow_sprite_cache_tail(0,&loan)==WF_JO_ASSET_ARGUMENT);
    assert(wf_jo_assets_borrow_sprite_cache_tail(1,&loan)==WF_JO_ASSET_ARGUMENT);
    assert(wf_jo_assets_borrow_sprite_cache_tail(loan_bytes,NULL)==WF_JO_ASSET_ARGUMENT);
    assert(wf_jo_assets_borrow_sprite_cache_tail(full,&loan)==WF_JO_ASSET_CAPACITY);
    assert(wf_jo_assets_borrow_sprite_cache_tail(full-8192u+block_bytes,&loan)==WF_JO_ASSET_CAPACITY);
    assert(wf_jo_assets_borrow_sprite_cache_tail((SIZE_MAX/block_bytes)*block_bytes,&loan)==WF_JO_ASSET_CAPACITY);
    assert(wf_jo_assets_sprite_cache_active_bytes()==full);
    assert(wf_jo_assets_borrow_sprite_cache_tail(loan_bytes,&loan)==0&&loan==cache+full-loan_bytes);
    assert(wf_jo_assets_sprite_cache_active_bytes()==full-loan_bytes);
    assert(wf_jo_assets_borrow_sprite_cache_tail(block_bytes,&other)==WF_JO_ASSET_ARGUMENT&&other==NULL);
    assert(wf_jo_assets_return_sprite_cache_tail((uint8_t *)loan+1u)==WF_JO_ASSET_ARGUMENT);
    /* The whole lent span still belongs to the original cache allocation. */
    assert(wf_jo_assets_preload_scenes(loan,282624u)==WF_JO_ASSET_ARGUMENT);
    wf_jo_assets_get_stats(&before);
    for(unsigned i=0;i<active_slots;i++) {
        assert(wf_jo_assets_tile_read((uint16_t)(i*tiles_per_block),&pixels)==0);
        assert(memcmp(pixels,raw+i*block_bytes,128u)==0);
    }
    wf_jo_assets_get_stats(&after);assert(after.block_misses==before.block_misses);
    memset(loan,0x6d,loan_bytes);
    for(unsigned i=active_slots;i<full_slots+200u;i++) {
        assert(wf_jo_assets_tile_read((uint16_t)(i*tiles_per_block),&pixels)==0);
        assert(pixels>=cache&&pixels+128u<=cache+full-loan_bytes);
        assert(memcmp(pixels,raw+i*block_bytes,128u)==0);
    }
    wf_jo_assets_get_stats(&after);assert(after.block_misses-before.block_misses==full_slots+200u-active_slots);
    for(size_t i=0;i<loan_bytes;i++)assert(((uint8_t *)loan)[i]==0x6d);
    unsigned hot=(full_slots+199u)*tiles_per_block;
    assert(wf_jo_assets_return_sprite_cache_tail(loan)==0);
    assert(wf_jo_assets_sprite_cache_active_bytes()==full);
    assert(wf_jo_assets_return_sprite_cache_tail(loan)==WF_JO_ASSET_ARGUMENT);
    wf_jo_assets_get_stats(&before);
    assert(wf_jo_assets_tile_read((uint16_t)hot,&pixels)==0);
    wf_jo_assets_get_stats(&after);assert(after.block_misses==before.block_misses);
    for(unsigned i=0;i<full_slots-active_slots;i++) {
        assert(wf_jo_assets_tile_read((uint16_t)((500u+i)*tiles_per_block),&pixels)==0);
        assert(memcmp(pixels,raw+(500u+i)*block_bytes,128u)==0);
    }
    /* Exactly 8 KiB remains: partial scenes must use only its one8K/two4K
     * slots, invalidate both mappings, and never overwrite the live loan. */
    assert(wf_jo_assets_borrow_sprite_cache_tail(full-8192u,&loan)==0);
    memset(loan,0x37,full-8192u);
    unsigned minimum_slots=8192u/block_bytes;
    for(unsigned i=0;i<minimum_slots;i++)assert(wf_jo_assets_tile_read((uint16_t)(i*tiles_per_block),&pixels)==0);
    size_t scene_bytes;uint8_t *expected=load("R18RING.BIN",&scene_bytes);uint8_t out[17];
    assert(wf_jo_assets_file_read("R18RING.BIN",4096u,out,sizeof out)==0);
    assert(memcmp(out,expected+4096u,sizeof out)==0);free(expected);
    wf_jo_assets_get_stats(&before);
    for(unsigned i=0;i<minimum_slots;i++) {
        assert(wf_jo_assets_tile_read((uint16_t)(i*tiles_per_block),&pixels)==0);
        assert(memcmp(pixels,raw+i*block_bytes,128u)==0);
    }
    wf_jo_assets_get_stats(&after);assert(after.block_misses-before.block_misses==minimum_slots);
    for(size_t i=0;i<full-8192u;i++)assert(((uint8_t *)loan)[i]==0x37);
    for(unsigned i=0;i<32u;i++)assert(guard[i]==0xa5&&guard[full+32u+i]==0xa5);
    assert(wf_jo_assets_bind(rom,rn,pack,pn-1u,cache,full)!=0);
    assert(wf_jo_assets_sprite_cache_active_bytes()==0u);
    assert(wf_jo_assets_return_sprite_cache_tail(loan)==WF_JO_ASSET_UNLOADED);
    assert(wf_jo_assets_borrow_sprite_cache_tail(block_bytes,&other)==WF_JO_ASSET_UNLOADED);
    assert(wf_jo_assets_bind(rom,rn,pack,pn,cache,full)==0);
    assert(wf_jo_assets_return_sprite_cache_tail(loan)==WF_JO_ASSET_ARGUMENT);
    assert(wf_jo_assets_sprite_cache_active_bytes()==full);
    free(guard);
    puts("PASS: cache-tail loan, retained hits, evictions, guards, scene scratch, return and rebind lifetime");
}

static void scene_view_edges(uint8_t *scene_arena) {
    const char *names[]={"R18RING.BIN","R18MENU.BIN","R18MAP1.BIN","R18MAP2.BIN","R18MAP4.BIN","R18MAP5.BIN","R18MAP6.BIN"};
    const uint8_t *source=NULL;unsigned packed=0,decoded=0;uint32_t file_bytes=0;
    assert(wf_jo_assets_scene_block_view(names[0],0,&source,&packed,&decoded,&file_bytes)==WF_JO_ASSET_UNLOADED);
    assert(wf_jo_assets_preload_scenes(scene_arena,282624u)==0);
    wf_jo_asset_stats_t before,after;wf_jo_assets_get_stats(&before);
    for(unsigned i=0;i<7u;i++) {
        size_t expected_bytes;uint8_t *expected=load(names[i],&expected_bytes);free(expected);
        unsigned first=be32(scene_arena+32u+i*16u+4u);
        unsigned count=(unsigned)((expected_bytes+8191u)/8192u);
        for(unsigned b=0;b<count;b++) {
            assert(wf_jo_assets_scene_block_view(names[i],b,&source,&packed,&decoded,&file_bytes)==0);
            unsigned start=be32(scene_arena+144u+(first+b)*8u);
            unsigned end=be32(scene_arena+148u+(first+b)*8u);
            unsigned remaining=(unsigned)expected_bytes-b*8192u;
            assert(source==scene_arena+start&&packed==end-start);
            assert(decoded==(remaining<8192u?remaining:8192u)&&file_bytes==expected_bytes);
        }
        assert(wf_jo_assets_scene_block_view(names[i],count,&source,&packed,&decoded,&file_bytes)==WF_JO_ASSET_ARGUMENT);
        assert(!source&&!packed&&!decoded&&!file_bytes);
        assert(wf_jo_assets_scene_block_view(names[i],UINT32_MAX,&source,&packed,&decoded,&file_bytes)==WF_JO_ASSET_ARGUMENT);
    }
    assert(wf_jo_assets_scene_block_view(NULL,0,&source,&packed,&decoded,&file_bytes)==WF_JO_ASSET_ARGUMENT);
    assert(wf_jo_assets_scene_block_view("../R18RING.BIN",0,&source,&packed,&decoded,&file_bytes)==WF_JO_ASSET_ARGUMENT);
    assert(wf_jo_assets_scene_block_view("R18RING.BIN",0,NULL,&packed,&decoded,&file_bytes)==WF_JO_ASSET_ARGUMENT);
    assert(wf_jo_assets_scene_block_view("R18RING.BIN",0,&source,NULL,&decoded,&file_bytes)==WF_JO_ASSET_ARGUMENT);
    assert(wf_jo_assets_scene_block_view("R18RING.BIN",0,&source,&packed,NULL,&file_bytes)==WF_JO_ASSET_ARGUMENT);
    assert(wf_jo_assets_scene_block_view("R18RING.BIN",0,&source,&packed,&decoded,NULL)==WF_JO_ASSET_ARGUMENT);
    /* Resident storage is contractually immutable; range checks still reject
     * a subsequently damaged referenced offset without publishing a view. */
    uint32_t first_start=be32(scene_arena+144u),first_end=be32(scene_arena+148u);
    put32(scene_arena+144u,0u);
    assert(wf_jo_assets_scene_block_view(names[0],0,&source,&packed,&decoded,&file_bytes)==WF_JO_ASSET_FORMAT);
    put32(scene_arena+144u,first_start);put32(scene_arena+148u,first_start);
    assert(wf_jo_assets_scene_block_view(names[0],0,&source,&packed,&decoded,&file_bytes)==WF_JO_ASSET_FORMAT);
    put32(scene_arena+148u,UINT32_MAX);
    assert(wf_jo_assets_scene_block_view(names[0],0,&source,&packed,&decoded,&file_bytes)==WF_JO_ASSET_FORMAT);
    assert(!source&&!packed&&!decoded&&!file_bytes);put32(scene_arena+148u,first_end);
    wf_jo_assets_get_stats(&after);assert(memcmp(&before,&after,sizeof before)==0);
    puts("PASS: all resident scene block views, final lengths, index/name/null bounds and no decode/disc activity");
}
static void write_scene_pack(const uint8_t *data,size_t bytes) {
    FILE *f=fopen("SCENES.WFP.test-new","wb");assert(f);
    assert(fwrite(data,1,bytes,f)==bytes);assert(!fclose(f));
    assert(!rename("SCENES.WFP.test-new","SCENES.WFP"));
}
static void scene_metadata_edges(uint8_t *arena) {
    size_t bytes;uint8_t *original=load("SCENES.WFP",&bytes);
    uint8_t *bad=malloc(bytes);assert(bad);
    unsigned index=32u+7u*16u,blocks=be32(original+16u),data_at=be32(original+20u);
    unsigned duplicate=blocks;
    for(unsigned i=1;i<blocks&&duplicate==blocks;i++)for(unsigned j=0;j<i;j++)
        if(!memcmp(original+index+i*8u,original+index+j*8u,8u)){duplicate=i;break;}
    assert(duplicate<blocks); /* A real shared stream, not a synthetic fixture. */
    for(unsigned test=0;test<7u;test++) {
        memcpy(bad,original,bytes);
        switch(test) {
        case 0: put32(bad+4u,1u);break; /* Old metadata cannot address seven worlds. */
        case 1: put32(bad+32u,be32(bad+32u)-1u);break;
        case 2: put32(bad+index,0u);break;
        case 3: put32(bad+index+4u,(uint32_t)bytes+1u);break;
        case 4: /* A partial overlap is not an exact alias. */
            put32(bad+index+duplicate*8u+4u,be32(bad+index+duplicate*8u+4u)-1u);break;
        case 5: /* Skipping the first physical stream leaves an invalid gap. */
            memcpy(bad+index,bad+index+8u,8u);break;
        case 6: put32(bad+16u,blocks+1u);break;
        }
        put32(bad+28u,checksum(bad+32u,data_at-32u));
        write_scene_pack(bad,bytes);
        assert(wf_jo_assets_preload_scenes(arena,282624u)==WF_JO_ASSET_FORMAT);
        assert(!wf_jo_assets_scenes_ready());
    }
    memcpy(bad,original,bytes);bad[28u]^=1u;write_scene_pack(bad,bytes);
    assert(wf_jo_assets_preload_scenes(arena,282624u)==WF_JO_ASSET_FORMAT);
    write_scene_pack(original,bytes);
    assert(wf_jo_assets_preload_scenes(arena,282624u)==0);
    free(bad);free(original);
    puts("PASS: shared scene index rejects version, directory, range, partial aliases, gaps and checksum corruption");
}
static void cache_geometry_edges(const uint8_t *rom,size_t rn,const uint8_t *pack,size_t pn,
                                  const uint8_t *raw,uint8_t *scene_arena) {
    unsigned block_bytes=be32(pack+8),tiles_per_block=block_bytes/128u;
    /* Minimum scratch lease must fit one8 KiB or two4 KiB slots. Partial
     * scene reads borrow all of them and must invalidate every reverse map. */
    uint8_t minimum[8192u+64u];memset(minimum,0xa5,sizeof minimum);
    assert(wf_jo_assets_bind(rom,rn,pack,pn,minimum+32u,8192u)==0);
    assert(wf_jo_assets_preload_scenes(scene_arena,282624u)==0);
    const uint8_t *pixels;unsigned slots=8192u/block_bytes;
    for(unsigned i=0;i<slots;i++)assert(wf_jo_assets_tile_read((uint16_t)(i*tiles_per_block),&pixels)==0);
    size_t scene_bytes;uint8_t *expected=load("R18RING.BIN",&scene_bytes);uint8_t out[17];
    scene_service_check_t hook={out,expected+4096u,sizeof out,0,0};
    wf_jo_assets_set_service_hook(scene_service,&hook);
    assert(wf_jo_assets_file_read("R18RING.BIN",4096u,out,sizeof out)==0&&hook.calls==1u);
    wf_jo_assets_set_service_hook(NULL,NULL);
    wf_jo_asset_stats_t before,after;wf_jo_assets_get_stats(&before);
    for(unsigned i=0;i<slots;i++){
        assert(wf_jo_assets_tile_read((uint16_t)(i*tiles_per_block),&pixels)==0);
        assert(memcmp(pixels,raw+i*block_bytes,128u)==0);
    }
    wf_jo_assets_get_stats(&after);assert(after.block_misses-before.block_misses==slots);
    /* A failed service reports failure after coherent copied output. */
    hook.calls=0;hook.fail=1;wf_jo_assets_set_service_hook(scene_service,&hook);
    assert(wf_jo_assets_file_read("R18RING.BIN",4096u,out,sizeof out)==WF_JO_ASSET_SERVICE&&hook.calls==1u);
    assert(wf_jo_assets_bind(rom,rn,pack,pn,minimum+32u,8192u)==0);
    assert(wf_jo_assets_tile_read(0,&pixels)==0); /* bind clears the failing hook */
    for(unsigned i=0;i<32u;i++)assert(minimum[i]==0xa5&&minimum[8192u+32u+i]==0xa5);
    free(expected);
    /* Exercise slot255 and the256-slot cap, not only a small cache. */
    size_t used=(size_t)WF_JO_CACHE_MAX_SLOTS*block_bytes;
    uint8_t *large=malloc(used+block_bytes+64u);assert(large);
    memset(large,0xa5,used+block_bytes+64u);
    assert(wf_jo_assets_bind(rom,rn,pack,pn,large+32u,used+block_bytes)==0);
    for(unsigned i=0;i<=WF_JO_CACHE_MAX_SLOTS;i++){
        assert(wf_jo_assets_tile_read((uint16_t)(i*tiles_per_block),&pixels)==0);
        assert(memcmp(pixels,raw+i*block_bytes,128u)==0);
    }
    wf_jo_assets_get_stats(&before);
    assert(wf_jo_assets_tile_read((uint16_t)tiles_per_block,&pixels)==0);
    wf_jo_assets_get_stats(&after);assert(after.block_misses==before.block_misses);
    assert(wf_jo_assets_tile_read(0,&pixels)==0&&memcmp(pixels,raw,128u)==0);
    wf_jo_assets_get_stats(&after);assert(after.block_misses==before.block_misses+1u);
    for(unsigned i=0;i<32u;i++)assert(large[i]==0xa5);
    for(size_t i=used+32u;i<used+block_bytes+64u;i++)assert(large[i]==0xa5);
    free(large);
    puts("PASS: minimum8 KiB scratch, all borrowed scene slots, service publication/failure and256-slot cache cap");
}
int main(int argc,char **argv) {
    assert(argc==4);
    size_t pack_bytes,rom_bytes,raw_bytes;
    uint8_t *pack=load(argv[1],&pack_bytes),*rom=load(argv[2],&rom_bytes);
    uint8_t *raw=load(argv[3],&raw_bytes);
    assert(raw_bytes==WF_JO_SPRITE_BYTES&&rom_bytes==WF_JO_ROM_BYTES);
    const size_t cache_bytes=8u*WF_JO_SPRITE_BLOCK;
    uint8_t *guarded=malloc(cache_bytes+64u);assert(guarded);
    memset(guarded,0xa5,cache_bytes+64u);uint8_t *cache=guarded+32u;
    assert(!wf_jo_assets_ready());
    assert(!wf_jo_assets_scenes_ready());
    assert(wf_jo_assets_bind(rom,rom_bytes,pack,pack_bytes,cache,cache_bytes)==0);
    assert(wf_jo_assets_ready());
    assert(!wf_jo_assets_scenes_ready());
    unsigned service_calls=0;
    wf_jo_assets_set_service_hook(count_service,&service_calls);
    /* Every original tile, including all rows/nibbles, must be unchanged. */
    for(uint32_t tile=0;tile<65536u;tile++) {
        const uint8_t *pixels=NULL;
        assert(wf_jo_assets_tile_read((uint16_t)tile,&pixels)==0);
        assert(pixels&&memcmp(pixels,raw+tile*128u,128u)==0);
    }
    wf_jo_asset_stats_t stats;wf_jo_assets_get_stats(&stats);
    assert(stats.block_misses==WF_JO_SPRITE_BYTES/be32(pack+8)&&stats.cd_reads==0);
    assert(service_calls==stats.block_misses); /* Hot reads never call the service hook. */
    wf_jo_assets_set_service_hook(NULL,NULL);
    /* A small cache permutation detects stale hits after eviction. */
    for(uint32_t i=0;i<8192u;i++) {
        uint16_t tile=(uint16_t)(i*40503u+317u);const uint8_t *pixels=NULL;
        assert(wf_jo_assets_tile_read(tile,&pixels)==0);
        assert(memcmp(pixels,raw+(uint32_t)tile*128u,128u)==0);
    }
    uint8_t out[257];memset(out,0x7c,sizeof out);
    assert(wf_jo_assets_file_read("SPRITES.PAK",8192u-47u,out,sizeof out)==0);
    assert(memcmp(out,raw+8192u-47u,sizeof out)==0);
    assert(wf_jo_assets_file_read("ROM68K.BIN",rom_bytes-1u,out,1)==0);
    assert(out[0]==rom[rom_bytes-1u]);
    assert(wf_jo_assets_file_read("ROM68K.BIN",rom_bytes,NULL,0)==0);
    assert(wf_jo_assets_file_read("ROM68K.BIN",rom_bytes,out,1)!=0);
    assert(wf_jo_assets_file_read("SPRITES.PAK",raw_bytes-128u,out,129)!=0);
    assert(wf_jo_assets_file_read("SPRITES.PAK",0,NULL,1)!=0);
    assert(wf_jo_assets_file_read("SPRITES.PAK",UINT32_MAX,out,1)!=0);
    assert(wf_jo_assets_tile_read(0,NULL)!=0);
    uint32_t size=0;
    assert(wf_jo_assets_file_size("SPRITES.PAK",&size)==0&&size==raw_bytes);
    assert(wf_jo_assets_file_size("ROM68K.BIN",&size)==0&&size==rom_bytes);
    assert(wf_jo_assets_file_size("MISSING.BIN",&size)!=0);
    assert(wf_jo_assets_file_size("ROM68K.BIN",NULL)!=0);
    for(unsigned i=0;i<32u;i++)assert(guarded[i]==0xa5&&guarded[32u+cache_bytes+i]==0xa5);
    /* Header/index corruption is rejected before a block can be addressed. */
    uint8_t *bad=malloc(pack_bytes);assert(bad);memcpy(bad,pack,pack_bytes);
    bad[0]^=1u;
    assert(wf_jo_assets_bind(rom,rom_bytes,bad,pack_bytes,cache,cache_bytes)!=0);
    assert(wf_jo_assets_last_detail()==WF_JO_ASSET_DETAIL_HEADER);
    assert(!wf_jo_assets_ready());
    assert(wf_jo_assets_rom_read(0,out,1)!=0);
    memcpy(bad,pack,pack_bytes);
    uint32_t data_at=be32(bad+20);
    put32(bad+36,(uint32_t)pack_bytes+1u);
    put32(bad+28,checksum(bad+32,data_at-32u));
    assert(wf_jo_assets_bind(rom,rom_bytes,bad,pack_bytes,cache,cache_bytes)!=0);
    assert(wf_jo_assets_last_detail()==WF_JO_ASSET_DETAIL_OFFSET);
    memcpy(bad,pack,pack_bytes);bad[28]^=1u;
    assert(wf_jo_assets_bind(rom,rom_bytes,bad,pack_bytes,cache,cache_bytes)!=0);
    assert(wf_jo_assets_last_detail()==WF_JO_ASSET_DETAIL_INDEX_CHECKSUM);
    assert(wf_jo_assets_bind(rom,rom_bytes,pack,pack_bytes-1u,cache,cache_bytes)!=0);
    assert(wf_jo_assets_bind(rom,rom_bytes-1u,pack,pack_bytes,cache,cache_bytes)!=0);
    assert(wf_jo_assets_bind(rom,rom_bytes,pack,pack_bytes,cache,8191u)!=0);
    /* The zlib Adler checksum catches payload corruption; no bad cache entry
     * may survive and be returned as a valid tile on a repeated call. */
    memcpy(bad,pack,pack_bytes);bad[be32(bad+36)-1u]^=1u;
    assert(wf_jo_assets_bind(rom,rom_bytes,bad,pack_bytes,cache,cache_bytes)==0);
    const uint8_t *pixels=NULL;
    assert(wf_jo_assets_tile_read(0,&pixels)==WF_JO_ASSET_DECODE);
    assert(wf_jo_assets_tile_read(0,&pixels)==WF_JO_ASSET_DECODE);
    assert(wf_jo_assets_bind(rom,rom_bytes,pack,pack_bytes,cache,cache_bytes)==0);
    wf_jo_assets_set_service_hook(fail_service,NULL);
    assert(wf_jo_assets_tile_read(0,&pixels)==WF_JO_ASSET_SERVICE);
    /* The block is coherent before the service runs; its failure cannot
     * publish incomplete pixels or poison a later cache hit. */
    wf_jo_assets_set_service_hook(NULL,NULL);
    assert(wf_jo_assets_tile_read(0,&pixels)==0&&memcmp(pixels,raw,128u)==0);
    for(unsigned i=0;i<32u;i++)assert(guarded[i]==0xa5&&guarded[32u+cache_bytes+i]==0xa5);
    printf("PASS: 65536 exact native tiles, 8192 eviction reads, cross-block reads, bounds/corruption and cache guards\n");
    /* The same loader used by Saturn is exercised against real host files.
     * Scenes and all sprite/ROM views must stop using the backing store once
     * preload ends, even across repeated unrelated scene reads. */
    uint8_t *cart=malloc(WF_JO_CART_BYTES+32u),*scene_guard=malloc(282624u+64u);
    assert(cart&&scene_guard);memset(cart,0xa5,WF_JO_CART_BYTES+32u);
    memset(scene_guard,0xa5,282624u+64u);uint8_t *scene_arena=scene_guard+32u;
    assert(wf_jo_assets_load(cart,1000000u,cache,cache_bytes)==WF_JO_ASSET_CAPACITY);
    assert(!wf_jo_assets_ready());
    assert(wf_jo_assets_load(cart,WF_JO_CART_BYTES,cache,cache_bytes)==0);
    size_t expected_used=WF_JO_ROM_BYTES+((pack_bytes+31u)&~(size_t)31u),remaining;
    assert(wf_jo_assets_cart_used()==expected_used);
    assert(wf_jo_assets_cart_tail(&remaining)==cart+expected_used);
    assert(remaining==WF_JO_CART_BYTES-expected_used&&remaining>=440710u);
    assert(wf_jo_assets_preload_scenes(scene_arena,100u)==WF_JO_ASSET_CAPACITY);
    assert(!wf_jo_assets_scenes_ready());
    assert(wf_jo_assets_preload_scenes(scene_arena,282624u)==0);
    assert(wf_jo_assets_scenes_ready());
    wf_jo_assets_get_stats(&stats);uint32_t reads_after_load=stats.cd_reads;
    assert(reads_after_load==3u);
    assert(wf_jo_assets_tile_read(65535u,&pixels)==0);
    /* Partial 8 KiB scene decoding invalidates both borrowed 4 KiB slots. */
    assert(wf_jo_assets_tile_read(32u,&pixels)==0);
    assert(wf_jo_assets_file_read("R18RING.BIN",4096u,out,17u)==0);
    assert(wf_jo_assets_tile_read(32u,&pixels)==0);
    assert(memcmp(pixels,raw+32u*128u,128u)==0);
    assert(wf_jo_assets_tile_read(65535u,&pixels)==0);
    wf_jo_assets_get_stats(&stats);uint32_t misses_before_scenes=stats.block_misses;
    const char *names[]={"R18RING.BIN","R18MENU.BIN","R18MAP1.BIN","R18MAP2.BIN","R18MAP4.BIN","R18MAP5.BIN","R18MAP6.BIN"};
    for(unsigned pass=0;pass<2u;pass++)for(unsigned i=0;i<7u;i++) {
        size_t actual_size;uint8_t *expected=load(names[i],&actual_size);
        uint8_t *actual=malloc(actual_size);assert(actual);
        assert(wf_jo_assets_file_size(names[i],&size)==0&&size==actual_size);
        assert(wf_jo_assets_file_read(names[i],0,actual,actual_size)==0);
        assert(memcmp(actual,expected,actual_size)==0);
        /* Whole-scene reads go directly into scene staging and must leave
         * even a one-slot hot sprite cache unchanged. */
        assert(wf_jo_assets_tile_read(65535u,&pixels)==0);
        assert(memcmp(pixels,raw+raw_bytes-128u,128u)==0);
        wf_jo_assets_get_stats(&stats);assert(stats.block_misses==misses_before_scenes);
        assert(wf_jo_assets_file_read(names[i],(uint32_t)actual_size-1u,out,2u)!=0);
        assert(wf_jo_assets_file_read(names[i],(uint32_t)actual_size,NULL,0)==0);
        free(actual);free(expected);
    }
    for(unsigned i=0;i<7u;i++) {
        size_t actual_size;uint8_t *expected=load(names[i],&actual_size);
        assert(wf_jo_assets_file_read(names[i],8192u-37u,out,sizeof out)==0);
        assert(memcmp(out,expected+8192u-37u,sizeof out)==0);
        assert(wf_jo_assets_file_read(names[i],(uint32_t)actual_size-sizeof out,out,sizeof out)==0);
        assert(memcmp(out,expected+actual_size-sizeof out,sizeof out)==0);
        free(expected);
    }
    /* Scene stream checksums are enforced even when the metadata was valid
     * at preload. No failed decode may be served as a successful cache hit. */
    unsigned first_end=be32(scene_arena+148u);scene_arena[first_end-1u]^=1u;
    assert(wf_jo_assets_file_read(names[0],0,out,sizeof out)==WF_JO_ASSET_DECODE);
    assert(wf_jo_assets_file_read(names[0],0,out,sizeof out)==WF_JO_ASSET_DECODE);
    scene_arena[first_end-1u]^=1u;
    assert(wf_jo_assets_file_read(names[0],0,out,sizeof out)==0);
    assert(wf_jo_assets_tile_read(65535u,&pixels)==0);
    assert(memcmp(pixels,raw+raw_bytes-128u,128u)==0);
    wf_jo_assets_get_stats(&stats);assert(stats.cd_reads==reads_after_load);
    assert(wf_jo_assets_bind(rom,rom_bytes,pack,pack_bytes,cache,cache_bytes)==0);
    assert(!wf_jo_assets_scenes_ready());
    for(unsigned i=0;i<32u;i++)assert(cart[WF_JO_CART_BYTES+i]==0xa5);
    for(unsigned i=0;i<32u;i++)assert(scene_guard[i]==0xa5&&scene_guard[282624u+32u+i]==0xa5);
    for(unsigned i=0;i<32u;i++)assert(guarded[i]==0xa5&&guarded[32u+cache_bytes+i]==0xa5);
    printf("PASS: checked loading, cartridge tail, seven exact compressed scenes, partial reads, corruption/guards and zero subsequent backing-store reads\n");
    scene_view_edges(scene_arena);
    scene_metadata_edges(scene_arena);
    cache_loan_edges(rom,rom_bytes,pack,pack_bytes,raw,scene_arena);
    cache_geometry_edges(rom,rom_bytes,pack,pack_bytes,raw,scene_arena);
    free(scene_guard);free(cart);free(bad);free(guarded);free(raw);free(rom);free(pack);return 0;
}
