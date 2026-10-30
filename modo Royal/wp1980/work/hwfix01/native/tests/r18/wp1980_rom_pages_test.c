#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "wf_r18_present.h"
#include "wp1980_royal_rom_pages.h"

static uint8_t source[0x80000],cache[WF_SATURN_LWRAM_ROM_BYTES],tiles[256],resident[512];
static unsigned reads;
static int read_file(const char *name,uint32_t at,void *out,size_t bytes)
{
    if(strcmp(name,"ROM68K.BIN")||at>sizeof source||bytes>sizeof source-at)return -1;
    ++reads;memcpy(out,source+at,bytes);return 0;
}
int main(int argc,char **argv)
{
    assert(argc==2);FILE *file=fopen(argv[1],"rb");assert(file);
    assert(fread(source,1,sizeof source,file)==sizeof source);assert(fgetc(file)==EOF);fclose(file);
    wf_r18_present_init(read_file,cache,sizeof cache,tiles,sizeof tiles);
    assert(wf_r18_rom_pages_preload(wp1980_rom_main_pages,sizeof wp1980_rom_main_pages/sizeof *wp1980_rom_main_pages)==WF_OK);
    assert(wf_r18_rom_extra_pages_preload(wp1980_rom_fast_pages,sizeof wp1980_rom_fast_pages/sizeof *wp1980_rom_fast_pages)==WF_OK);
    for(unsigned i=0;i<sizeof wp1980_rom_main_pages/sizeof *wp1980_rom_main_pages;i++)resident[wp1980_rom_main_pages[i]]=1u;
    for(unsigned i=0;i<sizeof wp1980_rom_fast_pages/sizeof *wp1980_rom_fast_pages;i++)resident[wp1980_rom_fast_pages[i]]=1u;
    unsigned loaded_reads=reads,pages=0u,checks=0u;uint8_t bytes[1024];
    for(unsigned page=0;page<512u;page++)if(resident[page]){
        unsigned at=page*1024u;
        assert(wf_r18_rom_read(NULL,at,bytes,sizeof bytes)==WF_OK);
        assert(!memcmp(bytes,source+at,sizeof bytes));++pages;++checks;
        static const unsigned offsets[]={0u,1u,29u,30u,31u,32u,1020u};
        for(unsigned i=0;i<sizeof offsets/sizeof offsets[0];i++)for(unsigned width=1u;width<=4u;width*=2u){
            assert(wf_r18_rom_read(NULL,at+offsets[i],bytes,width)==WF_OK);
            assert(!memcmp(bytes,source+at+offsets[i],width));++checks;
        }
        if(page<511u&&resident[page+1u]){
            assert(wf_r18_rom_read(NULL,at+1022u,bytes,4u)==WF_OK);
            assert(!memcmp(bytes,source+at+1022u,4u));++checks;
        }
    }
    assert(reads==loaded_reads);
    unsigned cold=0u;while(resident[cold])++cold;
    assert(wf_r18_rom_read(NULL,cold*1024u,bytes,sizeof bytes)==WF_OK);
    assert(!memcmp(bytes,source+cold*1024u,sizeof bytes));assert(reads==loaded_reads+1u);
    assert(wf_r18_rom_read(NULL,cold*1024u,bytes,4u)==WF_OK);assert(reads==loaded_reads+1u);
    printf("PASS: %u original pages byte-exact; %u full-page/unaligned/line-boundary checks; resident CD reads0; cold fallback retained\n",pages,checks);
    return 0;
}
