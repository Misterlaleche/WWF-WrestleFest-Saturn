/* Additional ASan/UBSan fixture: two CPU buffers, immutable command snapshots,
 * deterministic delayed manual display. Scheduling itself is tested by the
 * separate actual-SDK coroutine/device test. */
static unsigned drain_hint,drain_fixture_bank,drain_services,drain_service_fail;
void jo_core_set_frame_overlap(bool allow){drain_hint=allow;}
void __jo_core_synch(void)
{
    assert(drain_fixture);drain_hint=0;
    snapshot_bank=drain_fixture_bank;close_frame();
    __jo_core_pause();
}
static wf_status_t drain_service(void *unused)
{
    (void)unused;++drain_services;
    assert(wf_jo_video_drain_overlap(NULL,NULL)==WF_BAD_STATE);
    assert(!wf_jo_video_commands_begin(1));
    if(drain_service_fail){drain_service_fail=0;return WF_BLOCKED;}
    return WF_OK;
}
static void overlap_closure(void)
{
    reset();drain_fixture=1;build_tag=UINT32_MAX;
    wf_jo_video_cram_write(5,0x123456u);
    drain_fixture_bank=publish();assert(wf_jo_video_allow_overlap()==WF_OK);
    assert(drain_hint);__jo_core_synch();assert(!drain_hint);
    drain_fixture_bank=publish();assert(build_tag==0u);
    assert(wf_jo_video_allow_overlap()==WF_OK);__jo_core_synch();
    unsigned bank;assert(wf_jo_video_acquire_bank(&bank)==WF_OK);
    wf_jo_command_t *borrow=wf_jo_video_commands_begin(1);assert(borrow);
    memset(borrow,0xa5,sizeof *borrow);assert(wf_jo_video_texture_begin(1)==WF_OK);
    assert(wf_jo_video_drain_overlap(NULL,NULL)==WF_BAD_STATE);
    wf_jo_video_commands_abort();wf_jo_video_release_bank(bank);
    assert(wf_jo_video_repeat()==WF_BAD_STATE);
    wf_jo_video_cram_write(5,0xabcdefu);wf_jo_video_map(0)[0]=0x55aa;
    wf_jo_video_map_written(0);assert(wf_jo_video_pending_changes());
    drain_service_fail=1;
    assert(wf_jo_video_drain_overlap(drain_service,NULL)==WF_BLOCKED);
    unsigned sent=build_tag;
    assert(wf_jo_video_drain_overlap(drain_service,NULL)==WF_OK);
    assert(build_tag==sent); /* The retry does not append another successor. */
    assert(wf_jo_video_stats.overlap_last_target==0u);
    assert(show_tag==0u&&wf_jo_video_stats.overlap_last_drain_field>
           wf_jo_video_stats.overlap_last_shown_field);
    assert(cram[5]==0x123456u&&wf_jo_video_pending_changes());
    assert(wf_jo_video_repeat()==WF_BAD_STATE); /* Abort still blocks normal replay. */
    unsigned old_field=field_count,old_services=drain_services;
    uint32_t elapsed=999;
    assert(wf_jo_video_drain_overlap_fields(drain_service,NULL,&elapsed)==WF_OK);
    assert(elapsed==0u);
    assert(field_count==old_field&&drain_services==old_services);
    wf_jo_video_gate();assert(blanked);
    printf("PASS ASan/UBSan overlap drain target=0, two buffers, abort, dirty snapshot, retry, full field, reentrancy\n");
}
int main(void)
{
    int result=pipeline_legacy_main();assert(!result);
    overlap_closure();return 0;
}
