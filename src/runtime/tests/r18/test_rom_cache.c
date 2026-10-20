#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "wf_r18_present.h"

static uint8_t rom[524288], cache[524288], tile_stage[128];
static unsigned reads;
static int read_file(const char *name, uint32_t offset, void *dst, size_t bytes)
{
    if (strcmp(name,"ROM68K.BIN") || (bytes != 1024u && bytes != sizeof rom) ||
        offset > sizeof rom-bytes) return -1;
    ++reads;
    memcpy(dst,rom+offset,bytes);
    return 0;
}
int main(void)
{
    static const unsigned pages[] = {81u,209u,337u,81u,209u,337u};
    for (unsigned i=0; i<sizeof rom; ++i) rom[i]=(uint8_t)(i*37u+11u);
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
    puts("PASS RUNTIME-18 ROM pages stay resident across address collisions");
    return 0;
}
