#include <yaul.h>
#include <stdint.h>
#include <string.h>
#include "wf_gameplay_bindings.h"
#include "wf_r18_bootstrap.h"
#include "wf_r18_referee.h"
#include "wf_r18_match_result.h"
#include "wf_r18_cdfs.h"
#include "wf_r18_present.h"
#include "wf_r18_rom_hot_pages.h"
#include "wf_r18_ring.h"
#include "wf_r18_video.h"
#include "wf_r18_display.h"
#include "wf_r18_hud.h"
#include "wf_r18_world.h"

/* The IP loader places the executable in HWRAM. These three borrowed cache
 * ranges occupy LWRAM only, with no overlap and no dynamic allocation. */
#define RING_BLOB ((void *)0x00201000u)
#define ROM_CACHE ((void *)0x00220000u)
#define TILE_CACHE ((void *)0x00260000u)
#define BOOT_RNG ((volatile uint32_t *)0x0020005cu)
static volatile uint32_t video_clock;
static volatile uint32_t fatal_owner, fatal_dependency, fatal_frame;
static volatile uint32_t finished_frame, finished_result;
static volatile uint32_t finished_count, finished_timer;
static wf_runtime_t rt;
static wf_gameplay_binding_context_t gameplay;
static wf_phase_binding_t bindings[48];
static wf_r18_video_t video;
static wf_r18_display_t display;
static wf_r18_world_ctx_t world;
static wf_r18_bootstrap_context_t boot;
static smpc_peripheral_digital_t pad;
int main(void);
static void marker(unsigned r, unsigned g, unsigned b)
{
    const vdp2_scrn_bitmap_format_t fmt = {
        .scroll_screen = VDP2_SCRN_NBG0,
        .ccc = VDP2_SCRN_CCC_PALETTE_256,
        .bitmap_size = VDP2_SCRN_BITMAP_SIZE_512X256,
        .palette_base = VDP2_CRAM_MODE_2_OFFSET(0,0,0),
        .bitmap_base = VDP2_VRAM_ADDR(0,0)
    };
    vdp2_cram_mode_set(2);
    vdp2_scrn_bitmap_format_set(&fmt);
    vdp2_scrn_priority_set(VDP2_SCRN_NBG0, 7);
    vdp2_vram_cycp_t *cy = vdp2_vram_cycp_get();
    cy->pt[0].raw = 0x44eeeeeeu;
    vdp2_vram_cycp_set(cy);
    memset((void *)VDP2_VRAM_ADDR(0,0), 1, 512u*240u);
    vdp2_scrn_display_set(VDP2_SCRN_DISP_NBG0);
    *(volatile uint32_t *)VDP2_CRAM_MODE_2_OFFSET(0,0,1) =
        (b*255u/31u << 16) | (g*255u/31u << 8) | (r*255u/31u);
    vdp2_sync();
    vdp2_sync_wait();
    vdp2_tvmd_display_set();
}

static void vblank(void *work __unused)
{
    ++video_clock;
    smpc_peripheral_intback_issue();
}

void user_init(void)
{
    cd_block_init();
    smpc_peripheral_init();
    vdp_sync_vblank_out_set(vblank, NULL);
    vdp2_tvmd_display_clear();
    vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_NONE,
        VDP2_TVMD_HORZ_NORMAL_A, VDP2_TVMD_VERT_240);
    vdp2_scrn_back_color_set(VDP2_VRAM_ADDR(1, WF_SATURN_VDP2_BACK_COLOR),
        RGB1555(1, 0, 0, 0));
    (void)main();
}

static void fail(uint32_t owner, uint32_t dependency)
{
    fatal_owner = owner;
    fatal_dependency = dependency;
    fatal_frame = rt.g.frame_counter;
    marker(31, 0, 0);
    *(volatile uint32_t *)VDP2_CRAM_MODE_2_OFFSET(0,0,1) =
        ((owner & 255u) << 16) | (owner & 0xff00u) |
        ((owner >> 16) & 255u);
    *(volatile uint32_t *)VDP2_CRAM_MODE_2_OFFSET(0,0,2) =
        ((dependency & 255u) << 16) | (dependency & 0xff00u) |
        ((dependency >> 16) & 255u);
    memset((void *)(VDP2_VRAM_ADDR(0,0) + 512u*120u),
           2, 512u*120u);
    vdp2_sync();
    vdp2_sync_wait();
    for (;;) smpc_peripheral_process();
}

static wf_status_t read_ram(wf_runtime_t *r, uint32_t addr,
                            uint16_t *value, void *user)
{
    (void)user;
    if (!value) return WF_BAD_STATE;
    switch (addr) {
    case 0x1c0062u:
    case 0x1c0063u: *value = r->g.global_0063; return WF_OK;
    case 0x1c004eu:
        *value = (uint16_t)((r->g.credit_remainder_004e << 8) |
                             r->g.credit_count_004f);
        return WF_OK;
    case 0x1c004au: *value = r->g.credit_cycle_units_004b; return WF_OK;
    default: video.failed_address = addr; return WF_BLOCKED;
    }
}
static wf_status_t text_owner(wf_runtime_t *r, uint32_t pc, void *user)
{
    uint16_t id = (uint16_t)r->m68k_regs.d[0];
    return pc == 0x2503cu ? wf_r18_text_2503c(r, id, user) :
                            wf_r18_number_26122(r, id, user);
}
static wf_status_t palette_reset(wf_runtime_t *r, void *user)
{
    wf_r18_video_t *v = user;
    (void)r;
    memset(v->palette_map, 0, sizeof v->palette_map);
    memset(v->palette_used, 0, sizeof v->palette_used);
    return WF_OK;
}
static wf_status_t map_26e66(wf_runtime_t *r, void *user)
{
    (void)user;
    return wf_r18_ring_update((int16_t)r->g.global_17e6,
        (int16_t)r->g.global_17ee, (int16_t)r->g.global_17ea,
        (int16_t)r->g.global_17f2_word, boot.priority_140010) == 0 ?
        WF_OK : WF_BLOCKED;
}
static wf_status_t input_word(wf_runtime_t *r, uint32_t addr,
                              uint16_t *out, void *user)
{
    (void)r; (void)user;
    if (!out || addr < 0x140020u || addr > 0x140026u) return WF_BAD_STATE;
    *out = 0xffffu;
    return WF_OK;
}
static wf_status_t world_288f0(wf_runtime_t *r, uint8_t a[4],
                                uint8_t b, void *user)
{
    (void)user;
    return wf_r18_world_288f0(r, a, b, &world);
}
static wf_status_t world_26b68(wf_runtime_t *r, uint8_t s,
                                uint16_t d, void *user)
{
    (void)user;
    return wf_r18_world_26b68(r, s, d, &world);
}
static wf_status_t silent_audio(wf_runtime_t *r, uint16_t command, void *user)
{
    (void)r; (void)command; (void)user;
    return WF_OK;
}
/* $21E6 DBRA D0 waits for D0.W+1 complete vertical-blank edges. */
static wf_status_t wait_21e6(wf_runtime_t *r,uint16_t raw,void *user)
{
    (void)r; (void)user;
    uint32_t start=video_clock;
    while ((uint32_t)(video_clock-start)<(uint32_t)raw+1u)
        smpc_peripheral_process();
    return WF_OK;
}
static wf_status_t render(wf_runtime_t *r, wf_actor_raw_t *const *actors,
                           size_t count, void *user)
{
    return wf_r18_present_batch(r, actors, count, user);
}
static uint8_t controls(void)
{
    uint8_t bits = 0xffu;
    if (!pad.connected) return bits;
    if (pad.held.button.right) bits &= (uint8_t)~0x01u;
    if (pad.held.button.left)  bits &= (uint8_t)~0x02u;
    if (pad.held.button.down)  bits &= (uint8_t)~0x04u;
    if (pad.held.button.up)    bits &= (uint8_t)~0x08u;
    if (pad.held.button.a)     bits &= (uint8_t)~0x10u;
    if (pad.held.button.b)     bits &= (uint8_t)~0x20u;
    if (pad.held.button.c)     bits &= (uint8_t)~0x40u;
    if (pad.held.button.start) bits &= (uint8_t)~0x80u;
    return bits;
}
static void palette_and_ring(void)
{
    if(wf_r18_hud_commit(&video)!=WF_OK)fail(0x0c0000u,0);
    if (wf_r18_ring_palette_set(0, video.fg_palette,
                                 sizeof video.fg_palette) != 0 ||
        wf_r18_ring_palette_set(1, video.bg_palette,
                                 sizeof video.bg_palette) != 0 ||
        wf_r18_ring_update((int16_t)display.scroll[0],
            (int16_t)(2048u-display.scroll[1]), (int16_t)display.scroll[2],
            (int16_t)(2048u-display.scroll[3]),
            boot.priority_140010) != 0)
        fail(0x26e66u, video.failed_address);
}

int main(void)
{
    uint32_t size, previous;
    wf_r18_launch_config_t launch;
    wf_status_t status;
    size_t count;

    if (wf_r18_cdfs_init() != 0) fail(0x18cd00u, 0);
    if (wf_r18_file_size("R18RING.BIN", &size) != 0 || size != 114918u ||
        wf_r18_file_read("R18RING.BIN", 0, RING_BLOB, size) != 0)
        fail(0x18cd01u, size);
    if (wf_r18_file_size("ROM68K.BIN", &size) != 0 || size != 524288u)
        fail(0x18cd02u, size);
    if (wf_r18_file_size("SPRITES.PAK", &size) != 0 || size != 8388608u)
        fail(0x18cd03u, size);
    if (wf_saturn_yaul_video_init() != WF_OK ||
        wf_r18_ring_bind(RING_BLOB, 114918u) != 0)
        fail(0x18cd04u, 0);
    if(wf_r18_file_size("FONT8.BIN",&size)!=0||size!=WF_SATURN_FONT_BYTES||
       wf_r18_hud_bind(&video,wf_r18_file_read)!=WF_OK)fail(0x18cd08u,size);
    wf_r18_present_init(wf_r18_file_read, ROM_CACHE, 256u*1024u,
                         TILE_CACHE, 640u*1024u);
    if (wf_r18_rom_pages_preload(wf_r18_rom_hot_pages,sizeof wf_r18_rom_hot_pages/sizeof wf_r18_rom_hot_pages[0]) != WF_OK) fail(0x18cd06u, 0);
    if (wf_r18_file_size("HOTSPR.BIN", &size) != 0 ||
        wf_r18_hot_sprites_load() != WF_OK ||
        size != wf_r18_hot_sprites_bytes()) fail(0x18cd07u, size);
    video.read_rom = wf_r18_rom_read;
    video.read_ram = read_ram;
    world.read_rom = wf_r18_rom_read;
    boot.palette_2aea = wf_r18_palette_2aea;
    boot.palette_release_2b58 = wf_r18_palette_2b58;
    boot.palette_reset = palette_reset;
    boot.map_26e66 = map_26e66;
    boot.video_batch = wf_r18_video_batch;
    boot.fg0_batch = wf_r18_video_batch;
    boot.text_owner = text_owner;
    boot.user = &video;
    count = wf_gameplay_bindings_build(&gameplay, bindings,
                                       sizeof bindings/sizeof bindings[0]);
    if (!count) fail(0x18cd05u, 0);
    wf_runtime_init(&rt, bindings, count);
    memset(&launch, 0, sizeof launch);
    launch.human = 0;
    launch.partner = 1;
    launch.cabinet_config = 0x300u;
    launch.rng_state = *BOOT_RNG;
#ifdef WF_R18_REPLAY_TEST
    launch.rng_state = 0u; /* Validation fixture: same reset seed as the HOST replay. */
#endif
    wf_r17_m68k_software_reset(&launch.registers);
    status = wf_r18_direct_boot(&rt, &boot, &launch);
    if (status != WF_OK) fail(boot.blocked_pc, video.failed_address);

    gameplay.cabinet_words_valid = 1;
    gameplay.cabinet_word_140020 = 0xffffu;
    gameplay.cabinet_word_140022 = 0xffffu;
    gameplay.camera_valid = 1;
    gameplay.camera.scene_index = 0;
    gameplay.camera.scroll0_x = rt.g.global_17e6;
    gameplay.camera.scroll1_x = rt.g.global_17ea;
    gameplay.camera.scroll0_y = rt.g.global_17ee;
    gameplay.camera.scroll1_y = rt.g.global_17f2_word;
    gameplay.input_valid_mask = 0x1ffu;
    memset(gameplay.input_hw_low, 0xff, sizeof gameplay.input_hw_low);
    gameplay.audio_command = silent_audio;
    gameplay.referee_direction_20c8=wf_r18_referee_direction_20c8;
    gameplay.referee_display_2067c=wf_r18_referee_display_2067c;
    gameplay.referee_clear_206fe=wf_r18_referee_clear_206fe;
    gameplay.referee_tag_phase6_90d6=wf_r18_tag_bonus_90d6;
    gameplay.referee_result_20156=wf_r18_result_sound_20156;

    gameplay.participant_input_word = input_word;
    gameplay.participant_commit_batch = wf_r18_video_commit;
    gameplay.post7_wait_21e6 = wait_21e6;
    gameplay.post7_fg0_batch = wf_r18_video_batch;
    gameplay.post7_video_batch = wf_r18_video_batch;
    gameplay.post7_resource_2aea = wf_r18_palette_2aea;
    gameplay.post7_resource_2b58 = wf_r18_palette_2b58;
    gameplay.post7_raw_2503c = wf_r18_text_2503c;
    gameplay.post7_raw_9aca = wf_r18_strip_9aca;
    gameplay.post7_fg0_peek_long = wf_r18_video_peek;
    gameplay.post7_render_2836_batch = render;
    gameplay.branchb_288f0 = world_288f0;
    gameplay.branchb_26b68 = world_26b68;
    gameplay.r17_owner_user = &video;
    if(wf_r18_display_init(&display,&rt,wf_r18_rom_read,NULL)!=WF_OK)fail(0x27048u,0);
    palette_and_ring();
    vdp2_sync();
    vdp2_sync_wait();
    vdp2_tvmd_display_set();
    previous = video_clock;
    for (;;) {
        smpc_peripheral_process();
        smpc_peripheral_digital_port(1, &pad);
        gameplay.input_hw_low[0] = controls();
#ifdef WF_R18_REPLAY_TEST
        /* Validation input only. No actor, CPU, stamina or result is changed. */
#ifdef WF_R18_REPLAY_NOINPUT
        gameplay.input_hw_low[0] = 0xffu;
#else
        gameplay.input_hw_low[0] = rt.g.frame_counter >= 60u && rt.g.frame_counter < 180u ?
            0xfeu : rt.g.frame_counter >= 180u && rt.g.frame_counter < 184u ? 0xefu : 0xffu;
#endif
#endif
        status = wf_runtime_frame(&rt);
        if (status == WF_BLOCKED && rt.frame_ready) {
            if(wf_r18_display_irq(&rt,&display)!=WF_OK)fail(0x27012u,0);
            if (wf_r18_world_commit(&rt, &world) != WF_OK)
                fail(0x2946u, 0);
            if(wf_r18_hud_irq(&rt,&video)!=WF_OK)fail(0x264aau,0);
            palette_and_ring();
            /* Queue both processors before waiting for their shared VBlank. */
            vdp1_sync();
            vdp2_sync();
            vdp1_sync_wait();
            vdp2_sync_wait();
            while (video_clock == previous) {}
            previous = video_clock;
            wf_runtime_vblank_ack(&rt);
            status = wf_runtime_frame(&rt);
        }
        *BOOT_RNG = rt.g.rng_state;
        /* $11B6 begins the post-match scene. Direct-ring mode retains the
         * final rendered frame only after the ROM result flags exist. */
        if (status == WF_BLOCKED && wf_r18_tag_match_complete(&rt)) {
            finished_frame = rt.g.frame_counter;
            finished_result = wf_be16(&rt.actors[0], WF_A_RESULT);
            finished_count = rt.g.global_169a;
            finished_timer = rt.g.global_16a0;
#ifdef WF_R18_REPLAY_TEST
            /* Test-disc terminal report, only after the real $11B6 condition.
             * Green plus the frame RGB bytes makes completion externally visible. */
            marker(0,31,0);
            *(volatile uint32_t *)VDP2_CRAM_MODE_2_OFFSET(0,0,1) = 0x0000ff00u;
            *(volatile uint32_t *)VDP2_CRAM_MODE_2_OFFSET(0,0,2) =
                ((finished_frame & 255u) << 16) | (finished_frame & 0xff00u) |
                ((finished_frame >> 16) & 255u);
            memset((void *)(VDP2_VRAM_ADDR(0,0)+512u*120u),2,512u*120u);
            vdp2_sync();vdp2_sync_wait();
#endif
            for (;;) smpc_peripheral_process();
        }
        if (status != WF_OK)
            fail(gameplay.last_blocked_phase ?
                 gameplay.last_blocked_phase : boot.blocked_pc,
                 gameplay.last_blocked_dependency ?
                 gameplay.last_blocked_dependency : video.failed_address);
    }
}
