/* HOST-only observation of actual successful tile and ROM-page demands.
 * The real upload/cache/readers run unchanged. Preloads are not demands;
 * first reads include HOTSPR and immutable-word-cache hits. */
#include "wf_r18_present.h"
#include <stdio.h>
#include <stdlib.h>
static wf_status_t trace_tile_read(uint16_t,const uint8_t **);
static wf_status_t trace_metasprite_emit(wf_r18_metasprite_ctx_t *,uint16_t,uint16_t,uint16_t,uint16_t,uint16_t);
#define wf_r18_rom_read untraced_rom_read
#define wf_r18_metasprite_emit trace_metasprite_emit
#define wf_saturn_yaul_upload_rom_tile(b,s,t,r) wf_saturn_yaul_upload_rom_tile(b,s,t,trace_tile_read)
#include "../../src/wf_r18_present.c"
#undef wf_saturn_yaul_upload_rom_tile
#undef wf_r18_metasprite_emit
#undef wf_r18_rom_read
wf_status_t wf_r18_rom_read(void *user,uint32_t address,void *out,size_t bytes)
{
    wf_status_t status=untraced_rom_read(user,address,out,bytes);
    const char *path=getenv("WF_PROBE_ROM_DEMAND");
    static FILE *log;
    static uint8_t seen[512u/8u];
    if(status==WF_OK&&bytes&&path){
        for(uint32_t page=address/1024u;page<=(address+bytes-1u)/1024u;page++){
            unsigned mask=1u<<(page%8u);
            if(seen[page/8u]&mask)continue;
            if(!log){log=fopen(path,"w");if(!log)exit(2);setvbuf(log,NULL,_IOLBF,0);}
            seen[page/8u]|=(uint8_t)mask;fprintf(log,"%u\n",page);
        }
    }
    return status;
}
static wf_status_t trace_metasprite_emit(wf_r18_metasprite_ctx_t *parser,uint16_t set,uint16_t pose,uint16_t x,uint16_t y,uint16_t partner)
{
    /* The included presenter binds its internal reader directly. Route that
     * callback through the same observer as video/world/display callers. */
    parser->read_rom=wf_r18_rom_read;
    return wf_r18_metasprite_emit(parser,set,pose,x,y,partner);
}
static wf_status_t trace_tile_read(uint16_t tile,const uint8_t **out)
{
    wf_status_t status=wf_r18_tile_read(tile,out);
    const char *path=getenv("WF_PROBE_TILE_DEMAND");
    static FILE *log;
    static uint8_t seen[65536u/8u];
    if(status==WF_OK&&path&&!(seen[tile/8u]&(1u<<(tile%8u)))){
        if(!log){log=fopen(path,"w");if(!log)exit(2);setvbuf(log,NULL,_IOLBF,0);}
        seen[tile/8u]|=(uint8_t)(1u<<(tile%8u));fprintf(log,"%u\n",tile);
    }
    return status;
}
