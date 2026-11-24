/* Observe the real winning policy and original post owners. Only hardware
 * layer calls and post-step boundaries are wrapped; game state is read-only. */
#define wf_jo_video_layers wf_visibility_unused_layers
#define __wrap_wf_r18_post_tag_step wf_visibility_original_post
#include "test_tag_campaign_main.c"
#undef __wrap_wf_r18_post_tag_step
#undef wf_jo_video_layers

static FILE *visibility_trace;
static unsigned staff_frames,staff_hidden,ranking_frames,ranking_hidden;
static unsigned visibility_pc;
static void visibility_finish(void)
{
    host_snapshot("credits-return");
    host_bytes("credits-return","frontend-d.bin",frontend.driver.regs.d,sizeof frontend.driver.regs.d);
    host_bytes("credits-return","frontend-a.bin",frontend.driver.a,sizeof frontend.driver.a);
    host_bytes("credits-return","fg0.bin",video.fg0,sizeof video.fg0);
    FILE *state=host_file("credits-return","frontend-state.json","w");
    fprintf(state,"{\"pc\":%u,\"phase\":%u,\"credits_entered\":%u,\"credits_completed\":%u,\"completed_owner\":%u,\"fields\":%u,\"scene_gate\":%u,\"world_visible\":%u}\n",
        frontend.driver.pc,frontend.driver.phase,campaign_credits_entered,campaign_credits_completed,
        campaign_owner_completed,host_fields,frontend.scene_gate,frontend.driver.world_visible);
    assert(!fclose(state));
    if(visibility_trace)assert(!fclose(visibility_trace));
    printf("VISIBILITY staff=%u staff_hidden=%u ranking=%u ranking_hidden=%u\n",
           staff_frames,staff_hidden,ranking_frames,ranking_hidden);
}
int __wrap_wf_r18_post_tag_step(wf_r18_post_tag_t *cursor,wf_r18_post_tag_context_t *context)
{
    if(cursor->pc==0x1b4eu&&!visibility_trace){
        const char *path=getenv("WF_VISIBILITY_TRACE");assert(path);
        visibility_trace=fopen(path,"w");assert(visibility_trace);
        fputs("field,arcade_tick,post_pc,pc,gate,world_visible,world,layers,opaque,sprites,records,hud_nonzero\n",visibility_trace);
        assert(!atexit(visibility_finish));
    }
    visibility_pc=cursor->pc;
    return wf_visibility_original_post(cursor,context);
}
void wf_jo_video_layers(unsigned layers,unsigned opaque,unsigned sprites)
{
    wf_visibility_unused_layers(layers,opaque,sprites);
    if(!visibility_trace)return;
    unsigned records=(unsigned)frontend.pending_records,hud=0;
    for(unsigned i=0;i<0x2000u;i++)hud+=video.fg0[i]!=0;
    uint32_t pc=frontend.driver.pc;
    if(pc==0x1d54u&&records){++staff_frames;if(!layers||!sprites)++staff_hidden;}
    if(pc==0x673eu&&hud){++ranking_frames;if(!layers)++ranking_hidden;}
    fprintf(visibility_trace,"%u,%u,%x,%x,%u,%u,%u,%u,%u,%u,%u,%u\n",
        host_fields,frontend.arcade_ticks,visibility_pc,pc,frontend.scene_gate,
        frontend.driver.world_visible,frontend.bound_world,layers,opaque,sprites,records,hud);
}
