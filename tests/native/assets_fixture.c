#include "assets_fixture.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <errno.h>
#include "wf_r18_present.h"
#include "wf_jo_audio.h"
/* General native fixtures do not instantiate SCSP. The transport fixture
 * overrides these with a live diagnostic and an observed service stub. */
__attribute__((weak)) volatile wf_jo_audio_diagnostic_t wf_jo_audio_diagnostic;
__attribute__((weak)) bool wf_jo_audio_service_cooperate(void){return true;}
extern uint64_t wf_jo_backend_frame_digest(void);
static unsigned char cart[WF_JO_CART_BYTES],cache[WF_JO_SPRITE_BLOCK*128u],scenes[724992];
static wf_jo_asset_stats_t start,last;
static unsigned max_misses,max_frame,frames,play,cache_bytes,block_bytes,cache_slots;
static FILE *trace;
static FILE *command_trace;
static FILE *tiles_trace;
static void tiles_word(unsigned value){fputc((int)(value>>8),tiles_trace);fputc((int)(value&255u),tiles_trace);}
void wf_host_assets_frame(unsigned frame){
    wf_jo_asset_stats_t now;wf_jo_assets_get_stats(&now);
    unsigned misses=now.block_misses-last.block_misses;
    if(trace)fprintf(trace,"%u,%u,%u,%u,%u\n",frame,now.block_hits-last.block_hits,misses,
        now.scene_block_decodes-last.scene_block_decodes,now.cd_reads-last.cd_reads);
    if(command_trace)fprintf(command_trace,"%u,%u,%016" PRIx64 ",%016" PRIx64 "\n",
        frame,wf_r18_present_commands(),wf_r18_present_frame_digest(),wf_jo_backend_frame_digest());
    if(tiles_trace){uint16_t ids[256];size_t count=wf_r18_present_frame_tiles(ids,256);
        tiles_word(frame>>16);tiles_word(frame);tiles_word((unsigned)count);
        for(size_t i=0;i<count;i++)tiles_word(ids[i]);}
    if(misses>max_misses){max_misses=misses;max_frame=frame;}
    last=now;frames=frame;
}
static void finish(void){
    wf_host_assets_frame(frames+1u);
    printf("JO_ASSETS slots=%u block_bytes=%u cache_bytes=%u actual_cache_slots=%u frames=%u block_hits=%u block_misses=%u max_misses_per_frame=%u max_frame=%u decode_failures=%u combat_backing_reads=%u combat_backing_bytes=%u\n",
      cache_slots,block_bytes,cache_bytes,cache_slots,frames,last.block_hits-start.block_hits,last.block_misses-start.block_misses,
      max_misses,max_frame,last.decode_failures-start.decode_failures,
      last.cd_reads-start.cd_reads,last.cd_bytes-start.cd_bytes);
    if(trace)fclose(trace);
    if(command_trace)fclose(command_trace);
    if(tiles_trace)fclose(tiles_trace);
    if(play){assert(last.decode_failures==start.decode_failures);assert(last.cd_reads==start.cd_reads);}
}
void wf_host_assets_init(void){
    /* CACHE_SLOTS remains a legacy count of8 KiB allocation units, regardless
     * of the pack's block size. Explicit CACHE_BYTES takes precedence. */
    const char *bytes=getenv("WF_JO_TEST_CACHE_BYTES"),*units=getenv("WF_JO_TEST_CACHE_SLOTS");
    const char *requested=bytes?bytes:units;
    cache_bytes=581632u;
    if(requested){
        char *end;errno=0;unsigned long value=strtoul(requested,&end,0);
        assert(!errno&&end!=requested&&!*end&&value);
        if(!bytes){assert(value<=128u);value*=8192u;}
        assert(value<=sizeof cache);cache_bytes=(unsigned)value;
    }
    assert(cache_bytes>=WF_JO_SPRITE_BLOCK&&cache_bytes<=sizeof cache);
    assert(wf_jo_assets_load(cart,sizeof cart,cache,cache_bytes)==0);
    /* The production loader already validated this resident WFSZ header. */
    const uint8_t *header=cart+WF_JO_ROM_BYTES;
    block_bytes=(uint32_t)header[8]<<24|(uint32_t)header[9]<<16|
                (uint32_t)header[10]<<8|header[11];
    assert(block_bytes==4096u||block_bytes==8192u);
    cache_slots=cache_bytes/block_bytes;
    if(cache_slots>WF_JO_CACHE_MAX_SLOTS)cache_slots=WF_JO_CACHE_MAX_SLOTS;
    assert(wf_jo_assets_preload_scenes(scenes,sizeof scenes)==0);
    assert(wf_jo_assets_scenes_ready());
    assert(!atexit(finish));
}
void wf_host_assets_play(void){
    wf_jo_assets_get_stats(&start);last=start;play=1u;
    const char *path=getenv("WF_JO_TEST_STATS");
    if(path){trace=fopen(path,"w");assert(trace);fputs("frame,block_hits,block_misses,scene_decodes,backing_reads\n",trace);}
    path=getenv("WF_JO_TEST_COMMANDS");
    if(path){command_trace=fopen(path,"w");assert(command_trace);fputs("frame,commands,canonical,pixels\n",command_trace);}
    path=getenv("WF_JO_TEST_TILES");
    if(path){tiles_trace=fopen(path,"wb");assert(tiles_trace);}
}
void wf_host_assets_dump(const void *data,size_t bytes){
    const char *path=getenv("WF_JO_TEST_ACTOR_DUMP");if(!path)return;
    FILE *file=fopen(path,"wb");assert(file);assert(fwrite(data,1,bytes,file)==bytes);assert(!fclose(file));
}
