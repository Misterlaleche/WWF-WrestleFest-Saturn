/* Original-runtime snapshot transport only; prepass behavior is production code. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wf_actor_prepass.h"
#include "wf_gameplay_bindings.h"
#include "wf_r18_runtime_handoff.h"
static wf_runtime_t runtime;
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
    wf_gameplay_binding_context_t context={0};
    wf_status_t status=wf_phase_actor_prepass_f18a(&runtime,0xf18au,&context);
    printf("status=%u owner=%x dependency=%x\n",status,context.last_blocked_phase,context.last_blocked_dependency);
    assert(wf_r18_runtime_export(&runtime,raw,sizeof raw)==WF_OK);
    file=fopen(argv[2],"wb");assert(file);
    assert(fwrite(raw,1,sizeof raw,file)==sizeof raw);
    for(unsigned i=0;i<8;i++)for(int shift=24;shift>=0;shift-=8)assert(fputc((int)((runtime.m68k_regs.d[i]>>shift)&255u),file)!=EOF);
    assert(!fclose(file));
    return status==WF_OK?0:2;
}
