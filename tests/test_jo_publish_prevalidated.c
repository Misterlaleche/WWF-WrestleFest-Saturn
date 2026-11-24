/* The production implementation is included unchanged except for generated
 * observation-only counter/seam hooks. Existing hardware doubles stay below
 * the production video lifetime code. Catch late validation, IRQ cache writes,
 * a skipped publish, or changes to the public checked helper. */
#include "wf_jo_texture_cache.h"
#include <stdlib.h>
#define main existing_pipeline_main
#include "test_jo_pipeline.c"
#undef main

/* This focused port fixture does not exercise SDK synchronization; the real
 * dual-CPU SDK runner covers that independently. */
void jo_core_set_frame_overlap(bool enabled){(void)enabled;}
void __jo_core_synch(void){__jo_core_pause();}
unsigned wf_test_publish_checks;
static void wf_test_submit_seam(unsigned phase);
#include "instrumented_video.c"

static unsigned seam_enabled,seam_seen[2],expect_checks=1;
static uint32_t validation_count;
static wf_jo_texture_cache_t cache_before;
static void wf_test_submit_seam(unsigned phase)
{
    if(!seam_enabled)return;
    ++seam_seen[phase];
    assert(texture_cache.building&&wf_test_publish_checks==validation_count+1u);
    cache_before=texture_cache;
    /* Retire an older actual production video snapshot at each seam, both
     * before and after SDK command commit; source pixels/maps remain checked
     * by the ordinary pipeline fixture. The texture builder must not move. */
    display(100u+phase);
    assert(!memcmp(&texture_cache,&cache_before,sizeof texture_cache));
}
static void checked_before_append(void)
{
    reset();unsigned bank;
    assert(wf_jo_video_acquire_bank(&bank)==WF_OK);
    assert(wf_jo_video_texture_begin(1)==WF_OK);
    wf_jo_texture_ref_t ref;unsigned hit;
    assert(wf_jo_video_texture_resolve(0,777,1,&ref,&hit)==WF_OK&&!hit);
    wf_jo_command_t command;memset(&command,0xa5,sizeof command);
    wf_jo_command_t saved=command;unsigned short oldkeys[1024];memcpy(oldkeys,keys,sizeof keys);
    cache_before=texture_cache;validation_count=wf_test_publish_checks;
    assert(wf_jo_video_submit(bank,&command,1)==WF_BAD_STATE);
    assert(wf_test_publish_checks==validation_count+1u);
    assert(!built_count&&!memcmp(&command,&saved,sizeof command));
    assert(!memcmp(oldkeys,keys,sizeof keys));
    assert(!memcmp(&texture_cache,&cache_before,sizeof texture_cache));
    wf_jo_video_commands_abort();wf_jo_video_release_bank(bank);
}
static void append_rejection_preserves_builder(void)
{
    reset();unsigned bank;
    assert(wf_jo_video_acquire_bank(&bank)==WF_OK);
    assert(wf_jo_video_texture_begin(1)==WF_OK);
    (void)prepare_tile(0,22,tile);
    cache_before=texture_cache;validation_count=wf_test_publish_checks;
    /* Null commands with a nonzero count pass the outer lifecycle/texture
     * guards but are rejected by append before any SDK mutation. */
    assert(wf_jo_video_submit(bank,NULL,1)==WF_BAD_STATE);
    assert(wf_test_publish_checks==validation_count+1u);
    assert(!built_count&&!memcmp(&texture_cache,&cache_before,sizeof texture_cache));
    wf_jo_video_commands_abort();wf_jo_video_release_bank(bank);
}
static void interleaved_publication(void)
{
    reset();publish();close_frame();publish();close_frame();
    unsigned bank;assert(wf_jo_video_acquire_bank(&bank)==WF_OK);
    assert(wf_jo_video_texture_begin(3)==WF_OK);
    unsigned offset=prepare_tile(0,23,tile);
    (void)prepare_tile(2,24,tile); /* Slot1 is an intentionally unused hole. */
    wf_vdp1_sprite_cmd_t sprite={0};wf_jo_crop_t crop={0,16,0,16};wf_jo_command_t command;
    assert(wf_jo_sprite_build(&sprite,&crop,offset,&command)==WF_OK);
    validation_count=wf_test_publish_checks;seam_enabled=1;
    assert(wf_jo_video_submit(bank,&command,1)==WF_OK);seam_enabled=0;
    assert(seam_seen[0]==1u&&seam_seen[1]==1u);
    assert(wf_test_publish_checks-validation_count==expect_checks);
    assert(!texture_cache.building&&texture_cache.repeat_valid&&texture_cache.repeat_count==3);
    assert(texture_cache.repeat[1]==WF_JO_GPU_NONE);
    assert(texture_cache.entries[texture_cache.repeat[0]].last_tag==102);
    assert(texture_cache.entries[texture_cache.repeat[2]].last_tag==102);
    snapshot_bank=bank;assert(wf_jo_video_commit()==WF_OK);close_frame();display(102);
}
int main(int argc,char **argv)
{
    if(argc>1)expect_checks=(unsigned)strtoul(argv[1],NULL,0);
    checked_before_append();append_rejection_preserves_builder();interleaved_publication();
    existing_pipeline_main();
    printf("PASS submit guards + 2 IRQ seams; validation passes/success=%u\n",expect_checks);
    return 0;
}
