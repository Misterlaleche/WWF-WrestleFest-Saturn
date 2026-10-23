#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "wf_r18_present.h"

static uint8_t rom[524288], cache[524288], tile_stage[128];
static unsigned reads;
wf_status_t wf_r18_rom_pages_preload(const uint16_t *,size_t);
static int read_file(const char *name, uint32_t offset, void *dst, size_t bytes)
{
    if (strcmp(name,"ROM68K.BIN") || bytes%1024u ||
        offset > sizeof rom-bytes) return -1;
    ++reads;
    memcpy(dst,rom+offset,bytes);
    return 0;
}
int main(void)
{
    static const unsigned pages[] = {81u,209u,337u,81u,209u,337u};
    /* Distinguish every page, including pages separated by 256KB. */
    for (unsigned i=0; i<sizeof rom; ++i)
        rom[i]=(uint8_t)(i*37u+11u+(i/1024u)*13u+(i/262144u)*71u);
    wf_r18_present_init(read_file,cache,sizeof cache,tile_stage,sizeof tile_stage);
    for (unsigned i=0; i<sizeof pages/sizeof pages[0]; ++i) {
        uint8_t got[4];
        uint32_t address=pages[i]*1024u+23u;
        if (wf_r18_rom_read(NULL,address,got,sizeof got) != WF_OK ||
            memcmp(got,rom+address,sizeof got) != 0) {
            fputs("ROM cache returned incorrect bytes\n",stderr);
            return 1;
        }
    }
    if (reads != 3u) {
        fprintf(stderr,"Three live ROM pages caused %u CD reads\n",reads);
        return 1;
    }
    wf_r18_present_init(read_file,cache,sizeof cache,tile_stage,sizeof tile_stage);
    reads=0;
    if (wf_r18_rom_preload() != WF_OK || reads != 1u) {
        fputs("Full ROM preload did not use one file read\n",stderr);
        return 1;
    }
    for (unsigned page=0; page<512u; ++page) {
        uint8_t got[5];
        uint32_t address=page*1024u+1001u;
        if (wf_r18_rom_read(NULL,address,got,sizeof got) != WF_OK ||
            memcmp(got,rom+address,sizeof got) != 0 || reads != 1u) {
            fputs("Preloaded ROM page differs or caused another file read\n",stderr);
            return 1;
        }
    }
    /* Sparse resident working set frees LWRAM for sprites; consecutive
     * source pages are read in batches without changing their ROM offsets. */
    static const uint16_t hot[]={1,2,3,63,130,131,511};
    wf_r18_present_init(read_file,cache,256u*1024u,tile_stage,sizeof tile_stage);reads=0;
    if(wf_r18_rom_pages_preload(hot,7)!=WF_OK||reads>4)return 1;
    unsigned initial=reads;
    for(unsigned i=0;i<7;i++){
        uint8_t got[17];uint32_t address=(uint32_t)hot[i]*1024u+3u;
        if(wf_r18_rom_read(NULL,address,got,sizeof got)!=WF_OK||memcmp(got,rom+address,sizeof got)||reads!=initial)return 1;
    }
    static const uint16_t bad[][2]={{2,1},{1,1},{511,512}};
    for(unsigned i=0;i<3;i++)if(wf_r18_rom_pages_preload(bad[i],2)!=WF_BLOCKED||reads!=initial)return 1;
    uint8_t got[5];if(wf_r18_rom_read(NULL,4096u,got,sizeof got)!=WF_OK||memcmp(got,rom+4096u,sizeof got)||reads!=initial+1u)return 1;
    puts("PASS RUNTIME-18 ROM pages: collisions, full/sparse preload, batched exact bytes, invalid IDs and real fallback");
    return 0;
}
