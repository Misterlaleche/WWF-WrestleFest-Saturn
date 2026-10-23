/* Read-only frame observation of the existing direct-combat HOST probe. */
#define main stamina_probe_main
#define wf_runtime_frame stamina_observe_frame
#include "../r17/direct_combat_probe.c"
#undef wf_runtime_frame
#undef main

extern wf_status_t wf_runtime_frame(wf_runtime_t *);
static FILE *stamina_trace;
static uint32_t stamina_last_frame=0xffffffffu;
static void stamina_snapshot(wf_runtime_t *r){
    if(stamina_last_frame==r->g.frame_counter)return;
    stamina_last_frame=r->g.frame_counter;
    for(unsigned i=0;i<WF_ACTOR_SLOTS;i++){
        const wf_actor_raw_t *a=&r->actors[i];if((wf_u8(a,0)&128u)==0u)continue;
        fprintf(stamina_trace,"%u,%u,%u,%u,%u,%d,%d,%u,%u,%u,%u,%04x,%04x,%04x,%04x\n",
            r->g.frame_counter,i,wf_be16(a,2),wf_be16(a,0x66),wf_be16(a,0x68),
            (int16_t)wf_be16(a,0x6a),(int16_t)wf_be16(a,0x6c),wf_be16(a,0x6e),
            wf_be16(a,0x70),wf_be16(a,0x72),wf_be16(a,0x106),wf_be16(a,0x20),
            wf_be16(a,0x60),wf_be16(a,0x56),wf_be16(a,0xfe));
    }
}
wf_status_t stamina_observe_frame(wf_runtime_t *r){
    stamina_snapshot(r);wf_status_t status=wf_runtime_frame(r);stamina_snapshot(r);return status;
}
int main(int argc,char **argv){
    const char *path=getenv("WF_STAMINA_TRACE");if(!path)return 2;
    stamina_trace=fopen(path,"w");if(!stamina_trace)return 2;
    fputs("frame,slot,internal_id,live66,pending68,queue6a,accumulator6c,visible6e,tier70,max72,recovery106,state20,selector60,role56,lockoutfe\n",stamina_trace);
    int result=stamina_probe_main(argc,argv);fclose(stamina_trace);return result;
}
