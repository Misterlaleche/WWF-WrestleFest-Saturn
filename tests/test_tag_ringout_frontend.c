/* Reuse the complete native frontend hardware fixture, without replacing
 * the production bind_world callback. This test isolates the newly required
 * resident arena6 so it reports its failure before a long frontend replay.
 */
#define main wf_frontend_transport_fixture_main
#include "test_jo_frontend.c"
#undef main

int main(void)
{
    wf_host_assets_init();wf_r18_present_init(wf_jo_assets_file_read,NULL,0,NULL,0);
    size_t count=wf_gameplay_bindings_build(&gameplay,bindings,48);assert(count);
    wf_runtime_init(&runtime,bindings,count);
    wf_jo_frontend_config_t config={.runtime=&runtime,.video=&video,.world=&world,.display=&display,
        .clock=&clock_state,.ram=raw,.ram_bytes=sizeof raw,.scene_blob=scene,.scene_capacity=sizeof scene,
        .priority_140010=&priority,.render_counts=gameplay.render_queue.count,.combat_read_ram=combat_ram};
    assert(wf_jo_frontend_init(&front,&config,1085u)==WF_OK);
    wf_jo_scene_prepare_cancel();
    wf_host_assets_play();
    wf_jo_asset_stats_t before,loaded,repeated;
    wf_jo_assets_get_stats(&before);
    size_t cache_bytes=wf_jo_assets_sprite_cache_active_bytes();
    wf_status_t status=front.driver.platform.bind_world(&front,6u);
    printf("FRONTEND_WORLD6 status=%u blocked=%08x/%08x bound_world=%u\n",
        status,front.blocked_pc,front.blocked_dependency,front.bound_world);
    if(status!=WF_OK){fputs("FAIL frontend cannot bind the original arena6\n",stderr);return 1;}
    assert(front.bound_world==6u&&scene[6]==0u&&scene[7]==6u&&scene[8]==0u&&scene[9]==6u);
    assert(video.ring_cell&&front.scene_gate==2u);
    wf_jo_assets_get_stats(&loaded);
    assert(loaded.scene_block_decodes>before.scene_block_decodes);
    assert(loaded.cd_reads==before.cd_reads&&loaded.cd_bytes==before.cd_bytes);
    assert(wf_jo_assets_sprite_cache_active_bytes()==cache_bytes);
    assert(front.driver.platform.bind_world(&front,6u)==WF_OK);
    wf_jo_assets_get_stats(&repeated);
    assert(repeated.scene_block_decodes==loaded.scene_block_decodes);
    assert(front.driver.platform.bind_world(&front,5u)==WF_OK&&front.bound_world==5u);
    assert(scene[6]==0u&&scene[7]==5u&&scene[8]==0u&&scene[9]==5u);
    assert(front.driver.platform.bind_world(&front,0u)==WF_OK&&front.bound_world==0u);
    wf_jo_assets_get_stats(&repeated);
    assert(repeated.cd_reads==before.cd_reads&&repeated.cd_bytes==before.cd_bytes);
    puts("PASS frontend arena6 loads from resident pack, reuses its binding and returns through arenas5/0 without CD/cache changes");
    return 0;
}
