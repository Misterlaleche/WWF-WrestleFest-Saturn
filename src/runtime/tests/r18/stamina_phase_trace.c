/* Test-only observation around the real damage owner; gameplay is unchanged. */
#define wf_phase_damage stamina_unobserved_damage
#include "../../src/wf_core_phases.c"
#undef wf_phase_damage
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static FILE *stamina_phase_file;
static void close_trace(void){if(stamina_phase_file)fclose(stamina_phase_file);}
wf_status_t wf_phase_damage(wf_runtime_t *r,uint32_t address,void *user){
    wf_actor_raw_t before[WF_ACTOR_SLOTS];uint32_t registers[8];
    if(!stamina_phase_file){
        const char *path=getenv("WF_STAMINA_PHASE_TRACE");if(!path)return WF_BAD_STATE;
        stamina_phase_file=fopen(path,"wb");if(!stamina_phase_file)return WF_BAD_STATE;atexit(close_trace);
    }
    memcpy(before,r->actors,sizeof(before));memcpy(registers,r->m68k_regs.d,sizeof(registers));
    wf_status_t status=stamina_unobserved_damage(r,address,user);
    uint32_t metadata[4]={0x5354414du,r->g.frame_counter,(uint32_t)status,sizeof(before)};
    if(fwrite(metadata,sizeof(metadata),1,stamina_phase_file)!=1u||
       fwrite(registers,sizeof(registers),1,stamina_phase_file)!=1u||
       fwrite(before,sizeof(before),1,stamina_phase_file)!=1u||
       fwrite(r->m68k_regs.d,sizeof(registers),1,stamina_phase_file)!=1u||
       fwrite(r->actors,sizeof(before),1,stamina_phase_file)!=1u)return WF_BAD_STATE;
    return status;
}
