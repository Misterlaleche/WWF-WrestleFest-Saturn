/* Observe the natural celebration input phase, or run the production adapter
 * against its preserved complete RAM snapshot. No match outcome is injected. */
#ifdef WF_CAPTURE_ROYAL_INPUT
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "wf_gameplay_bindings.h"
#include "wf_r18_runtime_handoff.h"
static wf_phase_fn original_input_phase;
static wf_status_t observe_input_phase(wf_runtime_t *rt,uint32_t address,void *user)
{
    static unsigned captured;
    const char *directory=getenv("WF_CAPTURE_ROYAL_INPUT");
    if(!captured&&directory&&rt->g.frame_counter==13716u){
        wf_gameplay_binding_context_t *context=user;
        assert(rt->g.global_16c4==4u&&wf_be16(&rt->actors[4],WF_A_SELECTOR)==0x8eu);
        uint8_t raw[0x4000]={0};char path[1024];
        assert(wf_r18_runtime_export(rt,raw,sizeof raw)==WF_OK);
        snprintf(path,sizeof path,"%s/arcade-ram.bin",directory);
        FILE *file=fopen(path,"wb");assert(file);
        assert(fwrite(raw,1,sizeof raw,file)==sizeof raw);assert(!fclose(file));
        snprintf(path,sizeof path,"%s/state.json",directory);file=fopen(path,"w");assert(file);
        fprintf(file,"{\"frame\":%u,\"d\":[",rt->g.frame_counter);
        for(unsigned i=0;i<8;i++)fprintf(file,"%s%u",i?",":"",rt->m68k_regs.d[i]);
        fputs("],\"hardware_low\":[",file);
        for(unsigned i=0;i<4;i++)fprintf(file,"%s%u",i?",":"",context->input_hw_low[i]);
        fprintf(file,"]}\n");assert(!fclose(file));captured=1;
    }
    return original_input_phase(rt,address,user);
}
size_t __real_wf_gameplay_bindings_build(wf_gameplay_binding_context_t *,wf_phase_binding_t *,size_t);
size_t __wrap_wf_gameplay_bindings_build(wf_gameplay_binding_context_t *context,wf_phase_binding_t *bindings,size_t capacity)
{
    size_t count=__real_wf_gameplay_bindings_build(context,bindings,capacity);
    for(size_t i=0;i<count;i++)if(bindings[i].rom_addr==0x514eu){
        original_input_phase=bindings[i].fn;bindings[i].fn=observe_input_phase;
    }
    return count;
}
#else
#define main unused_probe_main
#include "native/jo_main_probe.c"
#undef main

int main(int argc,char **argv)
{
    assert(argc==15);uint8_t raw[0x4000],before[0x4000];
    FILE *file=fopen(argv[1],"rb");assert(file);
    assert(fread(raw,1,sizeof raw,file)==sizeof raw);assert(fgetc(file)==EOF);assert(!fclose(file));
    wf_host_assets_init();
    wf_r17_m68k_shadow_t registers={.valid=1};
    for(unsigned i=0;i<8;i++)registers.d[i]=(uint32_t)strtoul(argv[3+i],NULL,0);
    assert(wf_r18_runtime_import(&rt,raw,sizeof raw,&registers)==WF_OK);
    memcpy(before,raw,sizeof raw);assert(wf_r18_runtime_export(&rt,before,sizeof before)==WF_OK);
    assert(!memcmp(raw,before,sizeof raw));
    for(unsigned i=0;i<4;i++)gameplay.input_hw_low[i]=(uint8_t)strtoul(argv[11+i],NULL,0);
    gameplay.participant_input_word=input_word;
    wf_host_assets_play();
    wf_status_t status=wf_phase_input_real(&rt,0x514eu,&gameplay);
    printf("status=%u owner=%x dependency=%x\n",status,gameplay.last_blocked_phase,gameplay.last_blocked_dependency);
    assert(wf_r18_runtime_export(&rt,raw,sizeof raw)==WF_OK);
    file=fopen(argv[2],"wb");assert(file);assert(fwrite(raw,1,sizeof raw,file)==sizeof raw);
    for(unsigned i=0;i<8;i++)for(int shift=24;shift>=0;shift-=8)assert(fputc((int)((rt.m68k_regs.d[i]>>shift)&255u),file)!=EOF);
    assert(!fclose(file));return status==WF_OK?0:2;
}
#endif
