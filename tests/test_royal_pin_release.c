/* Snapshot transport for the natural Royal pin-intervention release.
 * The optional observer only copies RAM/registers before the real callback. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wf_r17_actions_r11.h"
#include "wf_r17_dispatch_1c03e.h"
#include "wf_r18_runtime_handoff.h"

#ifdef WF_CAPTURE_ROYAL_PIN
wf_status_t __real_wf_r17_dispatch_1c03e_r11(wf_runtime_t *,unsigned,wf_r17_actions_r11_bridge_t *);
wf_status_t __wrap_wf_r17_dispatch_1c03e_r11(wf_runtime_t *rt,unsigned slot,wf_r17_actions_r11_bridge_t *bridge)
{
    static unsigned captured;
    const char *directory=getenv("WF_CAPTURE_ROYAL_PIN");
    if(!captured&&directory&&rt->g.frame_counter==9803u&&slot==5u){
        uint8_t raw[0x4000]={0};char path[1024];
        assert(wf_r18_runtime_export(rt,raw,sizeof raw)==WF_OK);
        snprintf(path,sizeof path,"%s/arcade-ram.bin",directory);
        FILE *file=fopen(path,"wb");assert(file);
        assert(fwrite(raw,1,sizeof raw,file)==sizeof raw);assert(!fclose(file));
        snprintf(path,sizeof path,"%s/state.json",directory);file=fopen(path,"w");assert(file);
        fprintf(file,"{\"frame\":%u,\"slot\":5,\"d\":[",rt->g.frame_counter);
        for(unsigned i=0;i<8;i++)fprintf(file,"%s%u",i?",":"",rt->m68k_regs.d[i]);
        fprintf(file,"]}\n");assert(!fclose(file));captured=1;
    }
    return __real_wf_r17_dispatch_1c03e_r11(rt,slot,bridge);
}
#else
static wf_runtime_t runtime;
int main(int argc,char **argv)
{
    assert(argc==12);uint8_t raw[0x4000],before[0x4000];
    FILE *file=fopen(argv[1],"rb");assert(file);
    assert(fread(raw,1,sizeof raw,file)==sizeof raw);assert(fgetc(file)==EOF);assert(!fclose(file));
    wf_r17_m68k_shadow_t registers={.valid=1};
    for(unsigned i=0;i<8;i++)registers.d[i]=(uint32_t)strtoul(argv[4+i],NULL,0);
    assert(wf_r18_runtime_import(&runtime,raw,sizeof raw,&registers)==WF_OK);
    memcpy(before,raw,sizeof raw);assert(wf_r18_runtime_export(&runtime,before,sizeof before)==WF_OK);
    assert(!memcmp(raw,before,sizeof raw));
    unsigned slot=(unsigned)strtoul(argv[3],NULL,0);assert(slot<WF_ACTOR_SLOTS);
    wf_r17_actions_r11_bridge_t bridge={0};
    wf_r17_dispatch_1c03e_ctx_t dispatch={.actions=&bridge};
    wf_status_t status=wf_r17_dispatch_1c03e_bounded(&runtime,&runtime.actors[slot],&dispatch);
    printf("status=%u callback=%x dependency=%x\n",status,bridge.trace[slot].callback_rom,bridge.trace[slot].blocked_dependency_rom);
    assert(wf_r18_runtime_export(&runtime,raw,sizeof raw)==WF_OK);
    file=fopen(argv[2],"wb");assert(file);assert(fwrite(raw,1,sizeof raw,file)==sizeof raw);
    for(unsigned i=0;i<8;i++)for(int shift=24;shift>=0;shift-=8)assert(fputc((int)((runtime.m68k_regs.d[i]>>shift)&255u),file)!=EOF);
    assert(!fclose(file));return status==WF_OK?0:2;
}
#endif
