/* Read-only observation of the direct-combat host replay. */
#define main facing_probe_main
#define wf_runtime_frame facing_observe_frame
#include "../r17/direct_combat_probe.c"
#undef wf_runtime_frame
#undef main

extern wf_status_t wf_runtime_frame(wf_runtime_t *);
static FILE *facing_output;
static uint32_t last_facing_frame=0xffffffffu;

static void facing_snapshot(wf_runtime_t *r){
    if(last_facing_frame==r->g.frame_counter)return;
    last_facing_frame=r->g.frame_counter;
    for(unsigned slot=0;slot<4u;slot++){
        const wf_actor_raw_t *a=&r->actors[slot];
        if(!(wf_u8(a,WF_A_ACTIVE)&0x80u))continue;
        wf_actor_raw_t *target=wf_actor_from_rom_ptr(r,wf_be32(a,WF_A_TARGET),0);
        int target_slot=target?(int)(target-r->actors):-1;
        fprintf(facing_output,"%u,%u,%u,%u,%d,%u,%04x,%04x,%04x,%04x,%04x,%04x\n",
            r->g.frame_counter,slot,wf_be16(a,WF_A_INTERNAL_ID),wf_be16(a,WF_A_X),
            target_slot,target?wf_be16(target,WF_A_X):0u,wf_be16(a,WF_A_STATE),
            wf_be16(a,WF_A_AE),wf_be16(a,WF_A_FACING),wf_be16(a,WF_A_MIRROR),
            wf_be16(a,WF_A_LATCHED),wf_be16(a,WF_A_FLAGS32));
    }
}
wf_status_t facing_observe_frame(wf_runtime_t *r){
    wf_status_t status=wf_runtime_frame(r);
    facing_snapshot(r);
    return status;
}
int main(int argc,char **argv){
    const char *path=getenv("WF_FACING_TRACE");if(!path)return 2;
    facing_output=fopen(path,"w");if(!facing_output)return 2;
    fputs("frame,slot,id,x,target_slot,target_x,state,ae,facing,pose,latch,flags32\n",facing_output);
    int result=facing_probe_main(argc,argv);
    fclose(facing_output);
    return result;
}
