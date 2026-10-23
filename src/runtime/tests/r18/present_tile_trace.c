/* HOST-only observation of the original tile reader passed to VDP1.
 * The real upload/cache/reader run unchanged; only successful first demands
 * are logged, including hits in HOTSPR, to size a measured resident set. */
#include "wf_r18_present.h"
#include <stdio.h>
#include <stdlib.h>
static wf_status_t trace_tile_read(uint16_t,const uint8_t **);
#define wf_saturn_yaul_upload_rom_tile(b,s,t,r) wf_saturn_yaul_upload_rom_tile(b,s,t,trace_tile_read)
#include "../../src/wf_r18_present.c"
#undef wf_saturn_yaul_upload_rom_tile
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
