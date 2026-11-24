#include "wf_jo_scene_prepare.h"
#include "wf_jo_assets.h"
#include "zlib/zlib.h"
#include <limits.h>
#include <string.h>
#ifndef WF_JO_ASSETS_HOST
#include <jo/jo.h>
#endif

volatile wf_jo_scene_prepare_stats_t wf_jo_scene_prepare_stats;
static struct {
    z_stream z;
    uint8_t *storage;
    uint32_t bytes,produced;
    unsigned block,block_bytes,block_output;
    unsigned initialized,block_started,ready,state_used,window_used;
    char name[13];
} preparing;

static voidpf allocate(voidpf opaque,uInt count,uInt size)
{
    (void)opaque;
    if(!size||count>UINT_MAX/size)return Z_NULL;
    unsigned bytes=count*size;
    uint8_t *out;
    if(!preparing.state_used&&bytes<=8192u){
        preparing.state_used=1;out=preparing.storage+131072u;
    }else if(preparing.state_used&&!preparing.window_used&&bytes<=32768u){
        preparing.window_used=1;out=preparing.storage+131072u+8192u;
    }else return Z_NULL;
    memset(out,0,bytes);return out;
}
static void release(voidpf opaque,voidpf memory)
{
    (void)opaque;
    if(memory==preparing.storage+131072u)preparing.state_used=0;
    else if(memory==preparing.storage+131072u+8192u)preparing.window_used=0;
}
void wf_jo_scene_prepare_cancel(void)
{
    if(!preparing.storage)return;
    /* The inflater owns its state/window until inflateEnd completes. Only
     * then may ordinary sprite misses reuse these pages. */
    if(preparing.initialized)inflateEnd(&preparing.z);
    if(!preparing.ready)++wf_jo_scene_prepare_stats.cancelled;
    if(wf_jo_assets_return_sprite_cache_tail(preparing.storage))
        ++wf_jo_scene_prepare_stats.errors;
    memset(&preparing,0,sizeof preparing);
}
static int failed(int status)
{
    ++wf_jo_scene_prepare_stats.errors;
    wf_jo_scene_prepare_cancel();return status;
}
int wf_jo_scene_prepare_begin(const char *name)
{
    if(!name||strlen(name)>=sizeof preparing.name)return WF_JO_ASSET_ARGUMENT;
    const uint8_t *source;unsigned packed,decoded;uint32_t bytes;
    int status=wf_jo_assets_scene_block_view(name,0,&source,&packed,&decoded,&bytes);
    if(status)return status;
    if(!bytes||bytes>131072u)return WF_JO_ASSET_CAPACITY;
    wf_jo_scene_prepare_cancel();
    void *storage=NULL;
    status=wf_jo_assets_borrow_sprite_cache_tail(WF_JO_SCENE_PREPARE_BYTES,&storage);
    if(status)return status;
    preparing.storage=storage;preparing.bytes=bytes;
    /* The byte-oriented asset loader also accepts unaligned caller caches.
     * zlib state contains pointers/longs: never cast that valid byte lease
     * into a misaligned state on either SH-2 or the 64-bit host verifier. */
    if((uintptr_t)storage&7u)return failed(WF_JO_ASSET_ARGUMENT);
    memcpy(preparing.name,name,strlen(name)+1u);
    preparing.z.zalloc=allocate;preparing.z.zfree=release;
    if(inflateInit(&preparing.z)!=Z_OK)return failed(WF_JO_ASSET_DECODE);
    preparing.initialized=1;++wf_jo_scene_prepare_stats.begun;
    return WF_JO_ASSET_OK;
}
int wf_jo_scene_prepare_active(const char *name)
{return preparing.storage&&name&&!strcmp(name,preparing.name);}

int wf_jo_scene_prepare_step(void)
{
    if(!preparing.storage)return WF_JO_ASSET_UNLOADED;
    if(preparing.ready)return 1;
#if defined(JO_COMPILE_WITH_PROFILER_SUPPORT) && !defined(WF_JO_ASSETS_HOST)
    unsigned began=jo_get_ticks();
#endif
    if(!preparing.block_started){
        const uint8_t *source;unsigned packed,decoded;uint32_t bytes;
        int status=wf_jo_assets_scene_block_view(preparing.name,preparing.block,
            &source,&packed,&decoded,&bytes);
        if(status)return failed(status);
        if(bytes!=preparing.bytes||decoded>bytes-preparing.produced||
           inflateReset(&preparing.z)!=Z_OK)return failed(WF_JO_ASSET_DECODE);
        preparing.z.next_in=(Bytef *)(uintptr_t)source;preparing.z.avail_in=packed;
        preparing.block_bytes=decoded;preparing.block_output=0;preparing.block_started=1;
    }
    unsigned left=preparing.block_bytes-preparing.block_output;
    unsigned budget=left<WF_JO_SCENE_PREPARE_SLICE?left:WF_JO_SCENE_PREPARE_SLICE;
    preparing.z.next_out=preparing.storage+preparing.produced;preparing.z.avail_out=budget;
    unsigned input_before=preparing.z.avail_in;
    int result=inflate(&preparing.z,Z_NO_FLUSH);
    unsigned emitted=budget-preparing.z.avail_out;
    preparing.produced+=emitted;preparing.block_output+=emitted;
    ++wf_jo_scene_prepare_stats.steps;wf_jo_scene_prepare_stats.decoded_bytes+=emitted;
#if defined(JO_COMPILE_WITH_PROFILER_SUPPORT) && !defined(WF_JO_ASSETS_HOST)
    unsigned elapsed=jo_get_ticks()-began;
    wf_jo_scene_prepare_stats.total_ms+=elapsed;
    if(elapsed>wf_jo_scene_prepare_stats.max_step_ms)wf_jo_scene_prepare_stats.max_step_ms=elapsed;
#endif
    if(result==Z_STREAM_END){
        if(preparing.z.avail_in||preparing.block_output!=preparing.block_bytes||
           preparing.z.total_out!=preparing.block_bytes)return failed(WF_JO_ASSET_DECODE);
        preparing.block_started=0;++preparing.block;
        if(preparing.produced==preparing.bytes){
            inflateEnd(&preparing.z);preparing.initialized=0;preparing.ready=1;
            ++wf_jo_scene_prepare_stats.completed;return 1;
        }
        return 0;
    }
    if((result!=Z_OK&&result!=Z_BUF_ERROR)||
       (!emitted&&preparing.z.avail_in==input_before)||
       (!preparing.z.avail_in&&preparing.block_output<preparing.block_bytes))
        return failed(WF_JO_ASSET_DECODE);
    return 0;
}
int wf_jo_scene_prepare_view(const char *name,const void **blob,uint32_t *bytes)
{
    if(!blob||!bytes)return WF_JO_ASSET_ARGUMENT;
    *blob=NULL;*bytes=0;
    if(!preparing.ready||!wf_jo_scene_prepare_active(name))return 0;
    *blob=preparing.storage;*bytes=preparing.bytes;
    ++wf_jo_scene_prepare_stats.hits;return 1;
}
