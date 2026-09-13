#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "wp12_firstmatch_presentation.h"

static void test_gate_and_mode(void)
{
    assert(wp12fm_run_kind(0u,0u)==WP12FM_RUN_FIRST_MATCH);
    assert(wp12fm_run_kind(0u,4u)==WP12FM_RUN_CAMPAIGN_SPECIAL);
    assert(wp12fm_run_kind(0u,9u)==WP12FM_RUN_CAMPAIGN_SPECIAL);
    assert(wp12fm_run_kind(1u,0u)==WP12FM_RUN_NONE);
    assert(wp12fm_run_kind(0u,1u)==WP12FM_RUN_NONE);
    assert(wp12fm_mode_from_global_1c0161(0u)==WP12FM_MODE_TAG);
    assert(wp12fm_mode_from_global_1c0161(1u)==WP12FM_MODE_ROYAL);
}

static void test_scene_setup(void)
{
    wp12fm_scene_setup_t s;
    assert(wp12fm_scene_setup(0u,&s)==WP12FM_OK);
    assert(s.world_mode==4u && s.scroll_seed_x==0x140u && s.scroll_seed_y==0x200u);
    assert(s.priority_byte==0x7bu && s.initial_audio_raw==0x310eu && s.common_resource==0x34u);
    assert(!s.credit_visible);
    assert(wp12fm_scene_setup(4u,&s)==WP12FM_OK); assert(s.initial_audio_raw==0x310du);
    assert(wp12fm_scene_setup(9u,&s)==WP12FM_OK); assert(s.initial_audio_raw==0x310du);
}

static void test_sequence_topology(void)
{
    uint8_t a,b,r;
    assert(wp12fm_tag_pair_indices(0,&a,&b) && a==0u && b==1u);
    assert(wp12fm_tag_pair_indices(1,&a,&b) && a==2u && b==3u);
    assert(!wp12fm_tag_pair_indices(2,&a,&b));
    for (uint8_t i=0;i<4u;i++) { assert(wp12fm_royal_record_index(i,&r)); assert(r==i); }
    assert(!wp12fm_royal_record_index(4,&r));
}

static void test_select_mapping_and_tag_normalization(void)
{
    static const uint8_t expected[10]={0u,7u,2u,8u,6u,9u,1u,3u,10u,11u};
    uint8_t id=0xffu;
    uint16_t a,b;
    for (uint8_t slot=0;slot<10u;slot++) {
        assert(wp12fm_selector_slot_to_runtime_identity(slot,&id)==WP12FM_OK);
        assert(id==expected[slot]);
    }
    assert(wp12fm_selector_slot_to_runtime_identity(10u,&id)==WP12FM_INVALID_ARGUMENT);

    a=0x1200u; b=0x3401u;
    assert(wp12fm_tag_normalize_pair_type_words(&a,&b)==WP12FM_OK);
    assert(a==0x3401u && b==0x1200u);
    a=0x1202u; b=0x3409u;
    assert(wp12fm_tag_normalize_pair_type_words(&a,&b)==WP12FM_OK);
    assert(a==0x3409u && b==0x1202u);
    a=0x1200u; b=0x3409u;
    assert(wp12fm_tag_normalize_pair_type_words(&a,&b)==WP12FM_OK);
    assert(a==0x3409u && b==0x1200u);
    a=0x1202u; b=0x3403u;
    assert(wp12fm_tag_normalize_pair_type_words(&a,&b)==WP12FM_OK);
    assert(a==0x1202u && b==0x3403u);
}

static void test_name_tables(void)
{
    static const uint16_t tag[12][2]={
      {0x2b,0x2c},{0x2d,0x2e},{0x2f,0x30},{0x31,0x32},
      {0x33,0x34},{0x33,0x34},{0x35,0x36},{0x37,0x38},
      {0x39,0x3a},{0x3b,0x3c},{0x3d,0x3d},{0x3d,0x3d}
    };
    static const uint16_t royal[12]={0x2c,0x2e,0x30,0x32,0,0,0x36,0x38,0x3a,0x3c,0x3e,0x3f};
    for (uint8_t i=0;i<12u;i++) {
        assert(wp12fm_tag_name_group(i,false)==tag[i][0]);
        assert(wp12fm_tag_name_group(i,true)==tag[i][1]);
        assert(wp12fm_royal_name_group(i)==royal[i]);
    }
}

static void test_actor_setup(void)
{
    wp12fm_actor_setup_t a;
    assert(wp12fm_actor_setup(WP12FM_MODE_TAG,0,false,&a)==WP12FM_OK);
    assert(a.entry_set==0x40u && a.world_x==0x1c0u && a.world_y==0x130u && a.world_z==0x100u);
    assert(a.speed_word==8u && a.angle_word==0x80u && a.facing_word==0u && a.name_group==0x2bu);
    assert(wp12fm_actor_setup(WP12FM_MODE_TAG,1,true,&a)==WP12FM_OK);
    assert(a.entry_set==0x41u && a.world_x==0x1f0u && a.name_group==0x2eu);
    assert(wp12fm_actor_setup(WP12FM_MODE_ROYAL,3,false,&a)==WP12FM_OK);
    assert(a.entry_set==0x43u && a.world_x==0x1d8u && a.name_group==0x32u);
    assert(wp12fm_actor_setup(WP12FM_MODE_ROYAL,11,false,&a)==WP12FM_OK);
    assert(a.entry_set==0x4au); /* original 0x4B is decremented */
}

static void test_rng_cycle(void)
{
    wp12fm_walk_state_t s;
    memset(&s,0,sizeof(s));
    for (unsigned i=0;i<47u;i++) assert(wp12fm_walk_update(0,true,1,&s)==WP12FM_OK);
    assert(s.phase_plus24==0x0303u && s.counter_plus22==11u);
    wp12fm_walk_state_t before=s;
    assert(wp12fm_walk_update(0,false,0,&s)==WP12FM_NEED_SHARED_RNG_21B4);
    assert(memcmp(&s,&before,sizeof(s))==0);
    assert(wp12fm_walk_update(0,true,0,&s)==WP12FM_OK);
    assert(s.phase_plus24==4u && s.counter_plus22==0u);
    for (unsigned i=0;i<12u;i++) assert(wp12fm_walk_update(0,true,1,&s)==WP12FM_OK);
    assert(s.phase_plus24==0x0105u);

    memset(&s,0,sizeof(s));
    for (unsigned i=0;i<48u;i++) assert(wp12fm_walk_update(2,false,0,&s)==WP12FM_OK);
    assert(s.phase_plus24==0u); /* identity 2 has no RNG branch */
}

static void test_motion_and_terminal(void)
{
    assert(wp12fm_world_y_after_updates(0)==(int32_t)0x01300000);
    assert(wp12fm_world_y_after_updates(1)==(int32_t)0x012f8000);
    assert(wp12fm_world_y_after_updates(320)==(int32_t)0x00900000);
    assert(wp12fm_world_y_after_updates(321)==(int32_t)0x008f8000);
    assert(!wp12fm_terminal_after_updates(320));
    assert(wp12fm_terminal_after_updates(321));
    assert(wp12fm_x_offset_plus19(0u,0u)==0);
    assert(wp12fm_y_offset_plus1b(1u,3u)==0);
    assert(wp12fm_y_offset_plus1b(1u,4u)==0x30);
    assert(wp12fm_y_offset_plus1b(1u,7u)==0x30);
    assert(wp12fm_y_offset_plus1b(2u,7u)==0);
    assert(wp12fm_screen_x(0x1c0u,0x140u,0,0u)==0x80);
    assert(wp12fm_screen_x(0x1f0u,0x140u,0,0u)==0xb0);
    assert(wp12fm_screen_x(0x1d8u,0x140u,0,0u)==0x98);
    assert(wp12fm_screen_x(0x1d8u,0x140u,5,0u)==0x9d);
    assert(wp12fm_screen_x(0x1d8u,0x140u,5,0x8000u)==0x93);
    assert(wp12fm_screen_y(0x130u,0x100u,0x200u,0)==0x30);
    assert(wp12fm_screen_y(0x130u,0x100u,0x200u,0x30)==0x60);
    assert(wp12fm_screen_y(0x90u,0x100u,0x200u,0)==-0x70);
    assert(wp12fm_sprite_raster_y_from_screen_y(0x30)==192);
    assert(wp12fm_sprite_raster_anchor_y(0x130u,0x100u,0x200u,0)==192);
    assert(wp12fm_sprite_raster_y_from_screen_y(-0x70)==352);
    assert(wp12fm_sprite_raster_anchor_y(0x90u,0x100u,0x200u,0)==352);
}

static void test_overlay_and_wipe(void)
{
    assert(wp12fm_overlay_owner(WP12FM_MODE_TAG,false));
    assert(!wp12fm_overlay_owner(WP12FM_MODE_TAG,true));
    assert(wp12fm_overlay_owner(WP12FM_MODE_ROYAL,false));
    assert(wp12fm_reveal_vblanks()==24u);
    assert(wp12fm_close_vblanks()==24u);
    assert(wp12fm_close_tail_waits()==33u);
}

int main(void)
{
    test_gate_and_mode();
    test_scene_setup();
    test_sequence_topology();
    test_select_mapping_and_tag_normalization();
    test_name_tables();
    test_actor_setup();
    test_rng_cycle();
    test_motion_and_terminal();
    test_overlay_and_wipe();
    puts("WP12 R04 first-match presentation tests: PASS");
    return 0;
}
