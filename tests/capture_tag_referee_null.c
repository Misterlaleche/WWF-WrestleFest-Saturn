/* Save the real owner-entry state; delegate without changing runtime state. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "wf_r17_referee.h"
#include "wf_r18_runtime_handoff.h"
wf_status_t __real_wf_r17_referee_logic_1f914(wf_runtime_t *,wf_actor_raw_t *,wf_r17_referee_ctx_t *);
wf_status_t __wrap_wf_r17_referee_logic_1f914(wf_runtime_t *runtime,wf_actor_raw_t *referee,wf_r17_referee_ctx_t *context)
{
    if(runtime->g.progression_0162==3u&&runtime->g.frame_counter==1008u){
        const char *directory=getenv("WF_CAPTURE_TAG_REFEREE");assert(directory);uint8_t raw[0x4000];char path[1024];
        assert(wf_r18_runtime_export(runtime,raw,sizeof raw)==WF_OK);
        snprintf(path,sizeof path,"%s/arcade-ram.bin",directory);FILE *file=fopen(path,"wb");assert(file);
        assert(fwrite(raw,1,sizeof raw,file)==sizeof raw);assert(!fclose(file));
        snprintf(path,sizeof path,"%s/state.json",directory);file=fopen(path,"w");assert(file);
        fprintf(file,"{\"frame\":%u,\"stage\":%u,\"owner\":129300,\"d\":[",runtime->g.frame_counter,runtime->g.progression_0162);
        for(unsigned i=0;i<8;i++){fprintf(file,"%s%u",i?",":"",runtime->m68k_regs.d[i]);}
        fputs("]}\n",file);assert(!fclose(file));
    }
    return __real_wf_r17_referee_logic_1f914(runtime,referee,context);
}
