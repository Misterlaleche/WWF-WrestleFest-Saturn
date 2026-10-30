#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wp1980_royal_init.h"

static uint8_t image[WP1980_ROYINIT_BYTES],raw[16384];
static wf_runtime_t expected,actual,before;
static wf_r18_video_t video;
static wf_r18_world_ctx_t world;

static void read_file(const char *path,void *out,size_t bytes)
{
    FILE *file=fopen(path,"rb");assert(file);
    assert(fread(out,1,bytes,file)==bytes);assert(fgetc(file)==EOF);fclose(file);
}
int main(int argc,char **argv)
{
    assert(argc==3);char path[512],json[1024];
    read_file(argv[1],image,sizeof image);
    snprintf(path,sizeof path,"%s/entry_ram.bin",argv[2]);read_file(path,raw,sizeof raw);
    assert(!memcmp(image+WP1980_ROYINIT_RAM,raw,sizeof raw));
    snprintf(path,sizeof path,"%s/entry_registers.json",argv[2]);
    FILE *file=fopen(path,"r");assert(file);assert(fgets(json,sizeof json,file));fclose(file);
    wf_r17_m68k_shadow_t registers={.valid=1};
    for(unsigned i=0;i<8u;i++){
        char key[12];snprintf(key,sizeof key,"\"D%u\":",i);char *value=strstr(json,key);assert(value);
        registers.d[i]=(uint32_t)strtoul(value+strlen(key),NULL,10);
    }
    assert(wf_r18_runtime_handoff_f8c(&expected,raw,sizeof raw,&registers)==WF_OK);
    wp1980_royal_init_info_t info;
    assert(wp1980_royal_init_apply(&actual,&video,&world,image,sizeof image,&info)==WF_OK);
    assert(!memcmp(&expected,&actual,sizeof expected));
    assert(info.source_pc==0xf94u&&info.source_rng==0x6755f679u&&info.priority==0x7cu);
    assert(actual.g.rng_state==0xab3c1064u&&actual.frame_stage==3u&&actual.g.frame_counter==0u);
    assert(!memcmp(video.fg0,image+WP1980_ROYINIT_TEXT,8192u));
    assert(!memcmp(video.fg0+8192u,image+WP1980_ROYINIT_SPRITES,8192u));
    assert(!memcmp(video.palettes,image+WP1980_ROYINIT_PALETTE,sizeof video.palettes));
    assert(!memcmp(video.palette_used,raw+0x1600u,sizeof video.palette_used));
    assert(!memcmp(video.palette_map,raw+0x1610u,sizeof video.palette_map));
    read_file(argv[1],image,sizeof image);before=actual;
    image[WP1980_ROYINIT_RAM+0x5b0u]^=1u;
    assert(wp1980_royal_init_apply(&actual,&video,&world,image,sizeof image,&info)==WF_BLOCKED);
    assert(!memcmp(&actual,&before,sizeof actual));
    assert(wp1980_royal_init_apply(&actual,&video,&world,image,sizeof image-1u,&info)==WF_BAD_STATE);
    puts("PASS: packaged original initialization equals independent raw/D-register handoff for the entire runtime; corrupt/truncated data cannot mutate it");
    return 0;
}
