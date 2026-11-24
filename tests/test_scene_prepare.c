#include "wf_jo_assets.h"
#include "wf_jo_scene_prepare.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint8_t cart[WF_JO_CART_BYTES] __attribute__((aligned(32)));
static uint8_t cache[581632u+64u] __attribute__((aligned(32)));
static uint8_t arena[282624u] __attribute__((aligned(32)));
static uint8_t reference[WF_JO_SPRITE_BYTES];
static uint8_t expected[131072u];
static const char *const names[]={"R18RING.BIN","R18MENU.BIN","R18MAP1.BIN",
                                "R18MAP2.BIN","R18MAP4.BIN","R18MAP5.BIN","R18MAP6.BIN"};
static size_t read_reference(const char *folder,const char *name,void *out,size_t cap)
{
    char path[2048];int n=snprintf(path,sizeof path,"%s/%s",folder,name);
    assert(n>0&&(size_t)n<sizeof path);FILE *f=fopen(path,"rb");assert(f);
    size_t bytes=fread(out,1,cap,f);assert(feof(f)||fgetc(f)==EOF);assert(!ferror(f));fclose(f);return bytes;
}
static void sprite(unsigned tile)
{
    const uint8_t *pixels=NULL;
    assert(!wf_jo_assets_tile_read((uint16_t)tile,&pixels));
    assert(!memcmp(pixels,reference+(tile&65535u)*128u,128u));
}
int main(int argc,char **argv)
{
    assert(argc==2);assert(read_reference(argv[1],"SPRITES.PAK",reference,sizeof reference)==sizeof reference);
    memset(cache,0xa7,sizeof cache);
    assert(!wf_jo_assets_load(cart,sizeof cart,cache,565248u));
    assert(!wf_jo_assets_preload_scenes(arena,sizeof arena));
    size_t capacity=wf_jo_assets_sprite_cache_active_bytes();
    wf_jo_asset_stats_t before,after;wf_jo_assets_get_stats(&before);
    assert(wf_jo_scene_prepare_step()==WF_JO_ASSET_UNLOADED);
    assert(wf_jo_scene_prepare_begin("unknown")<0);
    unsigned steps=0;
    for(unsigned scene=0;scene<7u;scene++){
        size_t bytes=read_reference(argv[1],names[scene],expected,sizeof expected);
        assert(!wf_jo_scene_prepare_begin(names[scene]));
        assert(wf_jo_assets_sprite_cache_active_bytes()==capacity-WF_JO_SCENE_PREPARE_BYTES);
        const void *view=(void *)1;uint32_t view_bytes=1;
        assert(!wf_jo_scene_prepare_view(names[scene],&view,&view_bytes)&&!view&&!view_bytes);
        int status;
        do{
            uint32_t old=wf_jo_scene_prepare_stats.decoded_bytes;
            status=wf_jo_scene_prepare_step();assert(status>=0);
            assert(wf_jo_scene_prepare_stats.decoded_bytes-old<=WF_JO_SCENE_PREPARE_SLICE);
            /* Exercise the independent sprite inflater and force replacement
             * while the scene's private dictionary remains suspended. */
            for(unsigned i=0;i<37u;i++)sprite((steps*193u+i*977u)&65535u);
            assert(++steps<4096u);
        }while(!status);
        assert(wf_jo_scene_prepare_view(names[scene],&view,&view_bytes)==1);
        assert(view_bytes==bytes&&!memcmp(view,expected,bytes));
        assert(wf_jo_scene_prepare_step()==1);
        wf_jo_scene_prepare_cancel();
        assert(wf_jo_assets_sprite_cache_active_bytes()==capacity);
        assert(!wf_jo_scene_prepare_active(names[scene]));
        for(unsigned i=0;i<2048u;i++)sprite(i*32u);
    }
    /* Replacing a partial preparation and cancelling after dictionary use
     * must release every page and never expose a partial scene. */
    assert(!wf_jo_scene_prepare_begin(names[0]));
    assert(!wf_jo_scene_prepare_step());assert(!wf_jo_scene_prepare_step());
    assert(!wf_jo_scene_prepare_begin(names[1]));assert(!wf_jo_scene_prepare_step());
    wf_jo_scene_prepare_cancel();assert(wf_jo_assets_sprite_cache_active_bytes()==capacity);
    /* Corrupt the resident compressed block's Adler checksum. Only the
     * validated complete output may become available to a renderer. */
    const uint8_t *source;unsigned packed,decoded;uint32_t file_bytes;
    assert(!wf_jo_assets_scene_block_view(names[0],0,&source,&packed,&decoded,&file_bytes));
    uint8_t *checksum=(uint8_t *)(uintptr_t)(source+packed-1u);uint8_t saved=*checksum;*checksum^=1u;
    assert(!wf_jo_scene_prepare_begin(names[0]));int status=0;
    for(unsigned i=0;i<32u&&status==0;i++)status=wf_jo_scene_prepare_step();
    assert(status==WF_JO_ASSET_DECODE);*checksum=saved;
    const void *view;uint32_t bytes;
    assert(!wf_jo_scene_prepare_view(names[0],&view,&bytes));
    assert(wf_jo_assets_sprite_cache_active_bytes()==capacity);
    wf_jo_assets_get_stats(&after);assert(after.cd_reads==before.cd_reads&&after.cd_bytes==before.cd_bytes);
    for(unsigned i=565248u;i<sizeof cache;i++)assert(cache[i]==0xa7u);
    /* Legal byte caches need not be aligned enough for zlib state. Reject
     * that optional preparation, preserving the ordinary asset API. */
    assert(!wf_jo_assets_load(cart,sizeof cart,cache+1,565248u));
    assert(!wf_jo_assets_preload_scenes(arena,sizeof arena));
    assert(wf_jo_scene_prepare_begin(names[0])==WF_JO_ASSET_ARGUMENT);
    assert(wf_jo_assets_sprite_cache_active_bytes()==capacity);sprite(1234u);
    assert(!wf_jo_assets_load(cart,sizeof cart,cache,8192u));
    assert(!wf_jo_assets_preload_scenes(arena,sizeof arena));
    assert(wf_jo_scene_prepare_begin(names[0])==WF_JO_ASSET_CAPACITY);
    assert(wf_jo_assets_sprite_cache_active_bytes()==8192u);sprite(4321u);
    printf("PASS scene preparation: all7 scenes byte-identical in%u bounded slices; interleaved sprites, checksum failure, cancellation, alignment, lease recovery and zero CD reads\n",steps);
    return 0;
}
