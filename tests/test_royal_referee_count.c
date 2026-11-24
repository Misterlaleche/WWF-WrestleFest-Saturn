/* Observed pre-referee snapshot transport; logic, number tiles and cleanup are production code. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wf_r17_referee.h"
#include "wf_r18_referee.h"
#include "wf_r18_runtime_handoff.h"
static wf_runtime_t runtime;
static wf_r18_video_t video;
static uint8_t rom[0x80000];
static uint16_t sounds[16];static unsigned sound_count;
static wf_status_t read_rom(void *user,uint32_t at,void *data,size_t bytes)
{(void)user;if(at>sizeof rom||bytes>sizeof rom-at)return WF_BLOCKED;memcpy(data,rom+at,bytes);return WF_OK;}
static wf_status_t audio(wf_runtime_t *rt,uint16_t word,void *user)
{(void)rt;(void)user;assert(sound_count<16);sounds[sound_count++]=word;return WF_OK;}
int main(int argc,char **argv)
{
    assert(argc==12);
    uint8_t raw[0x4000],before[0x4000];
    FILE *file=fopen(argv[1],"rb");assert(file);
    assert(fread(raw,1,sizeof raw,file)==sizeof raw);assert(fgetc(file)==EOF);assert(!fclose(file));
    wf_r17_m68k_shadow_t registers={.valid=1};
    for(unsigned i=0;i<8;i++)registers.d[i]=(uint32_t)strtoul(argv[4+i],NULL,0);
    assert(wf_r18_runtime_import(&runtime,raw,sizeof raw,&registers)==WF_OK);
    memcpy(before,raw,sizeof raw);
    assert(wf_r18_runtime_export(&runtime,before,sizeof before)==WF_OK);
    assert(!memcmp(raw,before,sizeof raw));
    file=fopen(argv[3],"rb");assert(file);assert(fread(rom,1,sizeof rom,file)==sizeof rom);assert(!fclose(file));
    video.read_rom=read_rom;
    wf_r17_referee_ctx_t context={.display_2067c=wf_r18_referee_display_2067c,.clear_206fe=wf_r18_referee_clear_206fe,.audio_2052=audio,.user=&video};
    wf_status_t status=wf_r17_referee_logic_1f914(&runtime,&runtime.referee,&context);
    printf("status=%u dependency=%x\n",status,context.blocked_dependency_rom);
    assert(wf_r18_runtime_export(&runtime,raw,sizeof raw)==WF_OK);
    file=fopen(argv[2],"wb");assert(file);
    assert(fwrite(raw,1,sizeof raw,file)==sizeof raw);
    for(unsigned i=0;i<8;i++)for(int shift=24;shift>=0;shift-=8)assert(fputc((int)((runtime.m68k_regs.d[i]>>shift)&255u),file)!=EOF);
    assert(fwrite(video.fg0,1,sizeof video.fg0,file)==sizeof video.fg0);
    for(unsigned i=0;i<sound_count;i++){assert(fputc(sounds[i]>>8,file)!=EOF);assert(fputc(sounds[i]&255u,file)!=EOF);}
    assert(!fclose(file));
    return status==WF_OK?0:2;
}
