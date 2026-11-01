#include <yaul.h>
#include <stdint.h>
#include <string.h>
#include "wf_gameplay_bindings.h"
#include "wf_r18_bootstrap.h"
#include "wf_r18_referee.h"
#include "wf_r18_match_result.h"
#include "wf_r18_result_screen.h"
#include "wf_r18_cdfs.h"
#include "wf_r18_present.h"
#include "wf_r18_rom_hot_pages.h"
#include "wf_r18_ring.h"
#include "wf_r18_video.h"
#include "wf_r18_display.h"
#include "wf_r18_hud.h"
#include "wf_r18_world.h"
#include "wf_r18_transition.h"
#include "wf_r18_fault_pages.h"
#include "wf_r17_collision_24062.h"
#include "wf_collision.h"
#include "wf_r18_reset_combo.h"
#include "wf_r18_pause.h"
#include "wf_r18_pad_route.h"
#include "wf_r18_save_saturn.h"
#include "wf_r18_scsp.h"

/* The IP loader places the executable in HWRAM. Borrowed CPU-owned LWRAM
 * caches and video-write staging have disjoint ranges and no allocation. */
#define RING_BLOB ((void *)WF_SATURN_LWRAM_SCENE)
#define ROM_CACHE ((void *)WF_SATURN_LWRAM_ROM_CACHE)
#define TILE_CACHE ((void *)WF_SATURN_LWRAM_TILE_CACHE)
#define BOOT_RNG ((volatile uint32_t *)0x0020005cu)
static volatile uint32_t video_clock;
/* One-time boot stamps for read-only emulator profiling. No per-frame cost. */
static volatile uint32_t boot_timing[8];
static volatile uint32_t legal_first_visible;
static volatile uint32_t fatal_owner, fatal_dependency, fatal_frame;
static volatile uint32_t fatal_diagnostic[10], boot_seed_observed;
static wf_fault_snapshot_t fatal_snapshot;
static volatile uint32_t finished_frame, finished_result;
static volatile uint32_t attract_exit_frame;
static volatile uint32_t finished_count, finished_timer;
static volatile uint32_t finished_presentation;
static volatile uint32_t finished_vblanks,finished_presentation_vblanks;
static wf_runtime_t rt;
static wf_gameplay_binding_context_t gameplay;
static wf_phase_binding_t bindings[48];
static wf_r18_video_t video;
static wf_r18_display_t display;
static wf_r18_world_ctx_t world;
static wf_r18_bootstrap_context_t boot;
static wf_r18_transition_context_t transition;
static smpc_peripheral_digital_t pad;
static smpc_peripheral_digital_t frontend_pad2,frontend_pad3,frontend_pad4;
static smpc_peripheral_digital_t *const player_pads[4]={&pad,&frontend_pad2,&frontend_pad3,&frontend_pad4};
static uint32_t controllers_sample_clock=UINT32_MAX;
#ifndef WF_R18_REPLAY_TEST
static unsigned scsp_boot_complete;
static wf_r18_reset_combo_t reset_combo;
static wf_r18_pause_t combat_pause;
static unsigned reset_to_title_requested;
static unsigned title_loading_hidden;
static unsigned reset_combo_held(void)
{
    return pad.connected && pad.pressed.button.a && pad.pressed.button.b &&
           pad.pressed.button.c && pad.pressed.button.start;
}
#endif
static unsigned saturn_port_pads(unsigned port,smpc_peripheral_digital_t out[4])
{
    unsigned count=0;
    const smpc_peripheral_port_t *raw=smpc_peripheral_raw_port((uint8_t)port);
    if(raw){
        const smpc_peripheral_t *device;
        TAILQ_FOREACH(device,&raw->peripherals,peripherals){
            if(count==4u)break;
            if(device->type!=TYPE_DIGITAL&&device->type!=TYPE_ANALOG)continue;
            smpc_peripheral_digital_get(device,&out[count]);
            if(out[count].connected)++count;
        }
    }
    if(!count){
        smpc_peripheral_digital_port((uint8_t)port,&out[0]);
        if(out[0].connected)count=1u;
    }
    return count;
}
static void saturn_controllers_sample(void)
{
    smpc_peripheral_digital_t physical[2][4];
    unsigned counts[2];
    wf_r18_pad_location_t route[4];
    if(controllers_sample_clock==video_clock)return;
    controllers_sample_clock=video_clock;
    smpc_peripheral_process();
    counts[0]=saturn_port_pads(1u,physical[0]);
    counts[1]=saturn_port_pads(2u,physical[1]);
    wf_r18_pad_route(counts[0],counts[1],route);
    for(unsigned i=0;i<4u;i++){
        if(route[i].port)*player_pads[i]=physical[route[i].port-1u][route[i].slot];
        else memset(player_pads[i],0,sizeof *player_pads[i]);
    }
}
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
    vdp2_scrn_scroll_x_set(VDP2_SCRN_NBG0, FIX16(0));
    vdp2_scrn_scroll_y_set(VDP2_SCRN_NBG0, FIX16(0));
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
    wf_r18_scsp_vblank();
    if(!wf_r18_scsp_smpc_owned())smpc_peripheral_intback_issue();
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
    unsigned slot=0xffu;
    for(unsigned i=0;dependency&&i<WF_ACTOR_SLOTS;i++){
        if(gameplay.cpu_trace[i].blocked_dependency_rom==dependency||
           gameplay.cpu_trace[i].state5.blocked_dependency_rom==dependency||
           gameplay.f4c2_actions.trace[i].blocked_dependency_rom==dependency){slot=i;break;}
    }
    if(slot==0xffu&&owner==0x0000f4c2u&&
       gameplay.last_blocked_slot<WF_ACTOR_SLOTS)
        slot=gameplay.last_blocked_slot;
    fatal_diagnostic[0]=owner;
    fatal_diagnostic[1]=dependency;
    fatal_diagnostic[2]=rt.g.frame_counter;
    fatal_diagnostic[3]=boot_seed_observed;
    fatal_diagnostic[4]=rt.g.rng_state;
    fatal_diagnostic[5]=((uint32_t)pad.type<<24)|((uint32_t)pad.size<<16)|
        ((uint32_t)(pad.connected?1u:0u)<<8)|gameplay.input_hw_low[0];
    fatal_diagnostic[6]=((uint32_t)pad.pressed.raw<<16)|pad.held.raw;
    fatal_diagnostic[7]=((uint32_t)slot<<24)|((uint32_t)rt.g.mode_0161<<16);
    fatal_diagnostic[8]=0u;
    if(slot<WF_ACTOR_SLOTS){
        wf_actor_raw_t*a=&rt.actors[slot];
        wf_actor_raw_t*t=wf_actor_from_rom_ptr(&rt,wf_be32(a,WF_A_TARGET),NULL);
        fatal_diagnostic[7]|=((uint32_t)wf_u8(a,WF_A_FLAGS34)<<8)|
            (t?wf_u8(t,WF_A_FLAGS34):0u);
        fatal_diagnostic[8]=((uint32_t)wf_u8(a,WF_A_CPU_B5)<<24)|
            ((uint32_t)wf_u8(a,WF_A_CPU_B6)<<16)|
            ((uint32_t)wf_u8(a,WF_A_CPU_B7)<<8)|wf_u8(a,WF_A_CPU_B9);
    }
    fatal_diagnostic[9]=0x20261024u;
    /* Copy every field before changing video memory or polling the pad.
     * The action trace's selector is the value entering the dispatch, even
     * if the actor record was changed by a partial action callback. */
    memset(&fatal_snapshot,0,sizeof fatal_snapshot);
    fatal_snapshot.owner=owner;
    fatal_snapshot.dep=dependency;
    wf_fault_capture_collision(&fatal_snapshot,owner,dependency,wf_collision_blocked_probe,
                               wf_first_callbacks,wf_second_callbacks);
    fatal_snapshot.frame=rt.g.frame_counter;
    fatal_snapshot.vblank=video_clock;
    fatal_snapshot.rng=rt.g.rng_state;
    fatal_snapshot.seed=boot_seed_observed;
    fatal_snapshot.mode=rt.g.mode_0161;
    fatal_snapshot.progression=rt.g.progression_0162;
    fatal_snapshot.slot=(uint8_t)slot;
    fatal_snapshot.pad_type=pad.type;
    fatal_snapshot.pad_size=pad.size;
    fatal_snapshot.pad_connected=(uint8_t)(pad.connected?1u:0u);
    fatal_snapshot.pad_pressed=pad.pressed.raw;
    fatal_snapshot.pad_held=pad.held.raw;
    fatal_snapshot.input_low=gameplay.input_hw_low[0];
    fatal_snapshot.cabinet=gameplay.cabinet_word_140020;
    fatal_snapshot.video_failed=video.failed_address;
    wf_r18_cdfs_stats(&fatal_snapshot.cd_windows,&fatal_snapshot.cd_sectors);
    wf_r18_cdfs_file_stats(fatal_snapshot.cd_refills,fatal_snapshot.cd_bulk_sectors);
    for(unsigned i=0;i<WF_ACTOR_SLOTS;i++){
        const wf_actor_raw_t*a=&rt.actors[i];
        wf_fault_actor_t*d=&fatal_snapshot.actor[i];
        uint32_t target=wf_be32(a,WF_A_TARGET);
        d->id=wf_be16(a,WF_A_CHARACTER);
        d->state=wf_be16(a,WF_A_STATE);
        d->action=wf_be16(a,WF_A_SELECTOR);
        d->x=wf_be16(a,WF_A_X);d->y=wf_be16(a,WF_A_Y);d->z=wf_be16(a,WF_A_Z);
        d->vx=wf_be16(a,WF_A_VX);d->vy=wf_be16(a,WF_A_VY);
        d->mirror=wf_be16(a,WF_A_MIRROR);
        d->stamina=wf_be16(a,WF_A_STAMINA);
        d->anim=wf_be16(a,WF_A_ANIM_INDEX);
        d->timer=wf_be16(a,WF_A_TIMER44);
        d->active=wf_u8(a,WF_A_ACTIVE);
        d->role=wf_u8(a,WF_A_ROLE);
        d->flags=wf_u8(a,WF_A_FLAGS34);
        d->latch=wf_u8(a,WF_A_LATCHED+1u);
        d->phase=wf_u8(a,WF_A_ACTION_PHASE);
        d->facing=wf_u8(a,WF_A_FACING);
        d->input_fresh=wf_u8(a,WF_A_INPUT_FRESH);
        d->input_held=wf_u8(a,WF_A_INPUT_HELD);
        d->input_derived=wf_u8(a,WF_A_INPUT_DERIVED);
        d->target=0xffu;
        unsigned target_slot;
        if(wf_actor_from_rom_ptr(&rt,target,&target_slot))
            d->target=(uint8_t)target_slot;
    }
    if(slot<WF_ACTOR_SLOTS){
        const wf_r17_actions_r11_trace_t*t=&gameplay.f4c2_actions.trace[slot];
        const wf_cpu_trace_t*c=&gameplay.cpu_trace[slot];
        fatal_snapshot.trace_action=t->selector;
        fatal_snapshot.descriptor=t->descriptor_rom;
        fatal_snapshot.callback=t->callback_rom;
        fatal_snapshot.tick=t->tick_descriptor_rom;
        fatal_snapshot.action_dep=t->blocked_dependency_rom;
        fatal_snapshot.cpu_state=c->state;
        fatal_snapshot.cpu_handler=c->handler_rom;
        fatal_snapshot.cpu_dep=c->blocked_dependency_rom?
            c->blocked_dependency_rom:c->state5.blocked_dependency_rom;
    }
    marker(31, 0, 0);
    for(unsigned i=0;i<8u;i++)vdp2_sprite_priority_set(i,0);
    vdp2_scrn_display_set(VDP2_SCRN_DISP_NBG0);
    *(volatile uint32_t *)VDP2_CRAM_MODE_2_OFFSET(0,0,1)=0x00102483u;
    *(volatile uint32_t *)VDP2_CRAM_MODE_2_OFFSET(0,0,3)=0x00ffffffu;
    *(volatile uint32_t *)VDP2_CRAM_MODE_2_OFFSET(0,0,4)=0x00ffde3bu;
    *(volatile uint32_t *)VDP2_CRAM_MODE_2_OFFSET(0,0,5)=0x0082eaffu;
    wf_fault_draw((volatile uint16_t *)VDP2_VRAM_ADDR(0,0),&fatal_snapshot,0u);
    vdp2_sync();
    vdp2_sync_wait();
    /* Navigation reads a fresh pad, but never changes the frozen snapshot. */
    unsigned page=0u;
    unsigned prior=pad.pressed.button.right?1u:pad.pressed.button.left?2u:0u;
    smpc_peripheral_digital_t fault_pad;
    for (;;) {
        smpc_peripheral_process();
        smpc_peripheral_digital_port(1u,&fault_pad);
        unsigned nav=fault_pad.pressed.button.right?1u:
                     fault_pad.pressed.button.left?2u:0u;
        if(nav&&nav!=prior){
            page=nav==1u?(page+1u)%5u:(page+4u)%5u;
            wf_fault_draw((volatile uint16_t *)VDP2_VRAM_ADDR(0,0),
                          &fatal_snapshot,page);
            vdp2_sync();vdp2_sync_wait();
        }
        prior=nav;
    }
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
    if (!out) return WF_BAD_STATE;
    addr &= 0x00ffffffu;
    if (addr < 0x140020u || addr > 0x140026u || (addr & 1u)) return WF_BAD_STATE;
    /* Cached physical pad level from this combat quantum. Other ports and
     * the coin/DIP byte remain released; reading does not resample SMPC. */
    *out = (uint16_t)(0xff00u | gameplay.input_hw_low[(addr-0x140020u)/2u]);
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
/* $21E6 DBRA D0 waits for D0.W+1 complete vertical-blank edges. */
static wf_status_t wait_21e6(wf_runtime_t *r,uint16_t raw,void *user)
{
    (void)r; (void)user;
    uint32_t start=video_clock;
    while ((uint32_t)(video_clock-start)<(uint32_t)raw+1u) {
        (void)wf_r18_scsp_service();
        smpc_peripheral_process();
    }
    return WF_OK;
}
static wf_status_t result_wait(wf_runtime_t *r,uint16_t periods,void *user)
{
    if(!periods)return WF_BAD_STATE;
    return wait_21e6(r,(uint16_t)(periods-1u),user);
}
static wf_status_t result_commit(void *user)
{
    /* The character map and CRAM are direct stores. No extra sync wait:
     * the original33/33/65-period waits own the terminal presentation. */
    return wf_r18_hud_commit(user);
}
static wf_status_t royal_result_commit(void *user)
{
    wf_r18_video_t *v=user;
    wf_status_t s=wf_r18_hud_commit(v);
    if(s!=WF_OK)return s;
    for(unsigned role=WF_R18_PALETTE_TEXT;role<=WF_R18_PALETTE_BG;role++){
        s=wf_r18_palettes_commit(v,role);if(s!=WF_OK)return s;
    }
    return WF_OK;
}
static wf_status_t result_level(wf_runtime_t *r,unsigned high,void *user)
{
    (void)r;(void)user;
    while(!!vdp2_tvmd_vblank_in()!=!!high)smpc_peripheral_process();
    return WF_OK;
}
static wf_status_t render(wf_runtime_t *r, wf_actor_raw_t *const *actors,
                           size_t count, void *user)
{
    return wf_r18_present_batch(r, actors, count, user);
}
static uint8_t controls(const smpc_peripheral_digital_t *sample)
{
    uint8_t bits = 0xffu;
    if (!sample->connected) return bits;
    /* Yaul pressed is the sampled level; held contains rising edges. */
    if (sample->pressed.button.right) bits &= (uint8_t)~0x01u;
    if (sample->pressed.button.left)  bits &= (uint8_t)~0x02u;
    /* The physical NTSC-U pad report and frontend agree on this order.
     * The previous combat-only reversal moved the player's wrestler south
     * when north was pressed on Saturn. */
    if (sample->pressed.button.up)    bits &= (uint8_t)~0x04u;
    if (sample->pressed.button.down)  bits &= (uint8_t)~0x08u;
    if (sample->pressed.button.a)     bits &= (uint8_t)~0x10u;
    if (sample->pressed.button.b)     bits &= (uint8_t)~0x20u;
    if (sample->pressed.button.c)     bits &= (uint8_t)~0x40u;
    if (sample->pressed.button.start) bits &= (uint8_t)~0x80u;
    return bits;
}
#ifdef WF_R18_DIAG_RATE
/* Diagnostic only: count completed combat draws per 60 video blanks. The
 * arcade HUD shadow and gameplay state are untouched. FONT8.BIN glyphs
 * 32..41 are digits and glyph 54 is F. */
#define WF_R18_RATE_CELL (29u*64u+36u)
static uint32_t rate_start_clock,rate_draws;
static uint8_t rate_value,rate_palette,rate_ready;
static void rate_reset(void)
{
    rate_start_clock=video_clock;rate_draws=0;rate_value=rate_ready=0;
    rate_palette=0;
    for(unsigned row=0;row<30u;row++)for(unsigned col=0;col<40u;col++){
        const uint8_t*p=video.fg0+(row*64u+col)*4u;
        unsigned tile=(unsigned)p[1]|((unsigned)(p[3]&15u)<<8);
        if(tile>=32u&&tile<=74u){rate_palette=p[3]>>4;return;}
    }
}
static void rate_restore(void)
{
    volatile uint16_t*map=(volatile uint16_t*)VDP2_VRAM_ADDR(1,WF_SATURN_VDP2_HUD_NAMES);
    for(unsigned i=0;i<3u;i++){
        const uint8_t*p=video.fg0+(WF_R18_RATE_CELL+i)*4u;
        map[WF_R18_RATE_CELL+i]=(uint16_t)((p[3]<<8)|p[1]);
    }
    rate_ready=0;
}
static void rate_draw(void)
{
    ++rate_draws;
    uint32_t elapsed=video_clock-rate_start_clock;
    if(elapsed>=60u){
        uint32_t value=(rate_draws*60u+elapsed/2u)/elapsed;
        rate_value=(uint8_t)(value>99u?99u:value);
        rate_start_clock=video_clock;rate_draws=0;rate_ready=1;
    }
    if(rate_ready){
        volatile uint16_t*map=(volatile uint16_t*)VDP2_VRAM_ADDR(1,WF_SATURN_VDP2_HUD_NAMES);
        uint16_t palette=(uint16_t)rate_palette<<12;
        map[WF_R18_RATE_CELL]=(uint16_t)(palette|54u);
        map[WF_R18_RATE_CELL+1u]=(uint16_t)(palette|(32u+rate_value/10u));
        map[WF_R18_RATE_CELL+2u]=(uint16_t)(palette|(32u+rate_value%10u));
    }
}
#endif
static void palette_and_ring(void)
{
    if(wf_r18_hud_commit(&video)!=WF_OK)fail(0x0c0000u,0);
#ifdef WF_R18_DIAG_RATE
    rate_draw();
#endif
    if (wf_r18_palettes_commit(&video,WF_R18_PALETTE_FG) != WF_OK ||
        wf_r18_palettes_commit(&video,WF_R18_PALETTE_BG) != WF_OK ||
        (transition.unblank_pending?wf_r18_ring_update_deferred:wf_r18_ring_update)((int16_t)display.scroll[0],
            (int16_t)(2048u-display.scroll[1]), (int16_t)display.scroll[2],
            (int16_t)(2048u-display.scroll[3]),
            boot.priority_140010) != 0)
        fail(0x26e66u, video.failed_address);
}

#ifndef WF_R18_REPLAY_TEST
#include "wf_saturn_frontend.inc"
#endif

int main(void)
{
    uint32_t size, previous, gameplay_start;
#ifdef WF_R18_REPLAY_TEST
    wf_r18_launch_config_t launch;
#endif
    wf_status_t status;
    size_t count;

    if (wf_r18_cdfs_init() != 0) fail(0x18cd00u, 0);
    boot_timing[0]=video_clock;
#ifdef WF_R18_REPLAY_TEST
    /* Both sound executables borrow the scene buffer before ring loading. */
    bool sound_file_ok=wf_r18_file_size("SCSP.ELF",&size)==0&&size<=WF_SATURN_LWRAM_SCENE_BYTES&&
        wf_r18_file_read("SCSP.ELF",0,RING_BLOB,size)==0;
    boot_timing[1]=video_clock;
    if(!sound_file_ok||!wf_r18_scsp_boot(&video_clock,RING_BLOB,size))
        wf_r18_scsp_disable(WF_R18_AUDIO_BOOT);
    boot_timing[2]=video_clock;
    if (wf_r18_file_size("R18RING.BIN", &size) != 0 || size != 114918u ||
        wf_r18_file_read("R18RING.BIN", 0, RING_BLOB, size) != 0)
        fail(0x18cd01u, size);
    boot_timing[3]=video_clock;
#endif
    if (wf_r18_file_size("ROM68K.BIN", &size) != 0 || size != 524288u)
        fail(0x18cd02u, size);
    if (wf_r18_file_size("SPRITES.PAK", &size) != 0 || size != 8388608u)
        fail(0x18cd03u, size);
    if (wf_saturn_yaul_video_init() != WF_OK) fail(0x18cd04u, 0);
#ifdef WF_R18_REPLAY_TEST
    if (wf_r18_ring_bind(RING_BLOB, 114918u) != 0) fail(0x18cd04u, 0);
#endif
    if(wf_r18_file_size("FONT8.BIN",&size)!=0||size!=WF_SATURN_FONT_BYTES||
       wf_r18_hud_bind(&video,wf_r18_file_read)!=WF_OK)fail(0x18cd08u,size);
    boot_timing[4]=video_clock;
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
#ifdef WF_R18_REPLAY_TEST
    wf_r18_present_init(wf_r18_file_read, ROM_CACHE, WF_SATURN_LWRAM_ROM_BYTES,
                         TILE_CACHE, WF_SATURN_LWRAM_TILE_BYTES);
    if (wf_r18_rom_pages_preload(wf_r18_rom_hot_pages,sizeof wf_r18_rom_hot_pages/sizeof wf_r18_rom_hot_pages[0]) != WF_OK) fail(0x18cd06u, 0);
    if (wf_r18_rom_extra_pages_preload(wf_r18_rom_tag_extra_pages,
        sizeof wf_r18_rom_tag_extra_pages/sizeof wf_r18_rom_tag_extra_pages[0]) != WF_OK) fail(0x18cd06u, 1);
    if (wf_r18_file_size("HOTSPR.BIN", &size) != 0 ||wf_r18_hot_sprites_load() != WF_OK||
        size != wf_r18_hot_sprites_bytes()) fail(0x18cd07u, size);
    memset(&launch, 0, sizeof launch);
    launch.human = 0;
    launch.partner = 1;
    launch.cabinet_config = 0x300u;
    launch.rng_state = *BOOT_RNG;
#if defined(WF_R18_REPLAY_TEST) || defined(WF_R18_BOOT_SEED_ZERO)
    launch.rng_state = 0u; /* Validation fixture: same reset seed as the HOST replay. */
#endif
    boot_seed_observed=launch.rng_state;
    wf_r17_m68k_software_reset(&launch.registers);
    status = wf_r18_direct_boot(&rt, &boot, &launch);
    if (status != WF_OK) fail(boot.blocked_pc, video.failed_address);
#else
    wf_r18_save_saturn_init();
    boot_seed_observed=*BOOT_RNG;
#ifdef WF_R18_BOOT_SEED_ZERO
    boot_seed_observed=0u;
#endif
frontend_restart:
    reset_to_title_requested=0;
    boot_timing[5]=video_clock;
    if(frontend_begin(boot_seed_observed,title_loading_hidden)!=WF_OK){
        if(reset_to_title_requested)goto frontend_soft_reset;
        fail(frontend.pc,frontend.blocked_dependency);
    }
    if(frontend_to_gameplay()!=WF_OK){
        if(reset_to_title_requested)goto frontend_soft_reset;
        fail(frontend_match.blocked_pc?frontend_match.blocked_pc:frontend.pc,
             frontend_match.blocked_dependency?frontend_match.blocked_dependency:frontend.blocked_dependency);
    }
    if(frontend_combat_cache()!=WF_OK)fail(0x18cd06u,0);
    frontend_observed[7]=5;
#endif
#ifndef WF_R18_REPLAY_TEST
combat_resume:
    frontend_observed[7]=5;
#endif
    wf_r18_video_ring_bind(&video);

    gameplay.cabinet_words_valid = 1;
    gameplay.cabinet_word_140020 = 0xffffu;
    gameplay.cabinet_word_140022 = 0xffffu;
    gameplay.camera_valid = 1;
    gameplay.camera.scene_index = rt.g.global_007e;
    gameplay.camera.scroll0_x = rt.g.global_17e6;
    gameplay.camera.scroll1_x = rt.g.global_17ea;
    gameplay.camera.scroll0_y = rt.g.global_17ee;
    gameplay.camera.scroll1_y = rt.g.global_17f2_word;
    gameplay.input_valid_mask = 0x1ffu;
    memset(gameplay.input_hw_low, 0xff, sizeof gameplay.input_hw_low);
    gameplay.audio_command = wf_r18_scsp_audio;
    gameplay.referee_direction_20c8=wf_r18_referee_direction_20c8;
    gameplay.referee_display_2067c=wf_r18_referee_display_2067c;
    gameplay.referee_number_2067c=wf_r18_referee_number_2067c;
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
    transition=(wf_r18_transition_context_t){
        .video=&video,.world=&world,.display=&display,
        .render_queue=&gameplay.render_queue,.camera=&gameplay.camera,
        .scene_blob=RING_BLOB,.scene_capacity=WF_SATURN_LWRAM_SCENE_BYTES,
        .file_size=wf_r18_file_size,.file_read=wf_r18_file_read,
        .priority_140010=&boot.priority_140010
    };
    wf_r18_transition_bind(&transition);
    gameplay.transition_reset=wf_r18_transition_reset;
    gameplay.transition_barrier=wf_r18_transition_barrier;
#ifndef WF_R18_REPLAY_TEST
    /* The frontend and combat share the NBG2 name table. Refresh every
     * visible cell while blank, then reveal the ring only after its first
     * VDP1 command list has completed. Physical VDP2 can retain old menu
     * names even when the RAM shadow already contains the combat HUD. */
    vdp2_scrn_display_set(0);
    frontend_sprite_priority_set(0);
    vdp2_sync();vdp2_sync_wait();
    wf_r18_hud_reset(&video);
    transition.unblank_pending=1u;
#endif
#ifdef WF_R18_DIAG_RATE
    rate_reset();
#endif
    palette_and_ring();
#ifndef WF_R18_REPLAY_TEST
    vdp2_scrn_display_set(0);
#endif
    vdp2_sync();
    vdp2_sync_wait();
#ifndef WF_R18_REPLAY_TEST
    transition.unblank_pending=1u;
#else
    vdp2_tvmd_display_set();
#endif
    previous = video_clock;
    gameplay_start = video_clock;
#ifndef WF_R18_REPLAY_TEST
    saturn_controllers_sample();
    wf_r18_pause_reset(&combat_pause,pad.connected && pad.pressed.button.x);
#endif
    for (;;) {
        (void)wf_r18_scsp_service();
        saturn_controllers_sample();
#ifndef WF_R18_REPLAY_TEST
        if(wf_r18_reset_combo_poll(&reset_combo,reset_combo_held(),0)==WF_R18_RESET_TITLE)
            goto frontend_soft_reset;
        /* Keep polling the first controller and servicing audio at VBlank
         * while gameplay, animation, timers and the displayed frame stop. */
        if(wf_r18_pause_poll(&combat_pause,pad.connected && pad.pressed.button.x)){
            while(video_clock==previous){}
            previous=video_clock;
            continue;
        }
#endif
        for(unsigned i=0;i<4u;i++)gameplay.input_hw_low[i]=controls(player_pads[i]);
        gameplay.cabinet_word_140020=(uint16_t)(0xff00u|gameplay.input_hw_low[0]);
        gameplay.cabinet_word_140022=(uint16_t)(0xff00u|gameplay.input_hw_low[1]);
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
            if(transition.unblank_pending){
                unsigned priority=boot.priority_140010;
                vdp2_scrn_display_set(VDP2_SCRN_DISPTP_NBG2|
                    (priority==0x78u?(VDP2_SCRN_DISPTP_NBG0|VDP2_SCRN_DISP_NBG1):
                     (VDP2_SCRN_DISP_NBG0|VDP2_SCRN_DISPTP_NBG1)));
#ifndef WF_R18_REPLAY_TEST
                frontend_sprite_priority_set(priority==0x7cu?2u:3u);
#endif
                vdp2_sync();vdp2_sync_wait();
                vdp2_tvmd_display_set();transition.unblank_pending=0;
            }
            while (video_clock == previous) {}
            previous = video_clock;
            wf_runtime_vblank_ack(&rt);
            status = wf_runtime_frame(&rt);
        }
        *BOOT_RNG = rt.g.rng_state;
        /* The full frontend path resumes the original TAG main at$11B6.
         * The direct-ring test entry keeps its bounded result segment. */
        if (status == WF_BLOCKED &&
            (wf_r18_tag_match_complete(&rt)||wf_r18_royal_game_over(&rt)||
             wf_r18_attract_timeout(&rt))) {
#ifdef WF_R18_DIAG_RATE
            rate_restore();
#endif
            if(wf_r18_attract_timeout(&rt)){
#ifndef WF_R18_REPLAY_TEST
                attract_exit_frame=rt.g.frame_counter;
                if(frontend_after_attract()!=WF_OK){
                    if(reset_to_title_requested)goto frontend_soft_reset;
                    fail(frontend_match.blocked_pc?frontend_match.blocked_pc:0x1120u,
                         frontend_match.blocked_dependency?frontend_match.blocked_dependency:frontend.blocked_dependency);
                }
                if(frontend_to_gameplay()!=WF_OK){
                    if(reset_to_title_requested)goto frontend_soft_reset;
                    fail(frontend_match.blocked_pc?frontend_match.blocked_pc:frontend.pc,
                         frontend_match.blocked_dependency?frontend_match.blocked_dependency:frontend.blocked_dependency);
                }
                if(frontend_combat_cache()!=WF_OK)fail(0x18cd06u,0);
                goto combat_resume;
#else
                fail(0x1120u,0u);
#endif
            }
            int royal=wf_r18_royal_game_over(&rt);
            finished_frame = rt.g.frame_counter;
            finished_result = wf_be16(&rt.actors[0], WF_A_RESULT);
            finished_count = rt.g.global_169a;
            finished_timer = rt.g.global_16a0;
            finished_vblanks=video_clock-gameplay_start;
            uint32_t presentation_start=video_clock;
#ifndef WF_R18_REPLAY_TEST
            if(!royal){
                if(frontend_after_tag()!=WF_OK){
                    if(reset_to_title_requested)goto frontend_soft_reset;
                    fail(frontend_match.blocked_pc?frontend_match.blocked_pc:0x11b6u,
                         frontend_match.blocked_dependency?frontend_match.blocked_dependency:frontend.blocked_dependency);
                }
                finished_presentation_vblanks=video_clock-presentation_start;
                finished_presentation=1u;
                if(frontend_to_gameplay()!=WF_OK){
                    if(reset_to_title_requested)goto frontend_soft_reset;
                    fail(frontend_match.blocked_pc?frontend_match.blocked_pc:frontend.pc,
                         frontend_match.blocked_dependency?frontend_match.blocked_dependency:frontend.blocked_dependency);
                }
                if(frontend_combat_cache()!=WF_OK)fail(0x18cd06u,0);
                goto combat_resume;
            }
#endif
            status=royal?wf_r18_result_royal_game_over(&rt,&video,result_level,result_wait,royal_result_commit,&video):
                wf_r18_result_in_ring(&rt,&video,result_wait,result_commit,&video);
            if(status!=WF_OK)fail(royal?0x1afcu:0x11ceu,video.failed_address);
            finished_presentation_vblanks=video_clock-presentation_start;
            finished_presentation=royal?2u:1u;
#ifndef WF_R18_REPLAY_TEST
            if(royal){
                if(frontend_after_royal()!=WF_OK){
                    if(reset_to_title_requested)goto frontend_soft_reset;
                    fail(frontend_match.blocked_pc?frontend_match.blocked_pc:0x1b1eu,
                         frontend_match.blocked_dependency?frontend_match.blocked_dependency:frontend.blocked_dependency);
                }
                if(frontend_to_gameplay()!=WF_OK){
                    if(reset_to_title_requested)goto frontend_soft_reset;
                    fail(frontend_match.blocked_pc?frontend_match.blocked_pc:frontend.pc,
                         frontend_match.blocked_dependency?frontend_match.blocked_dependency:frontend.blocked_dependency);
                }
                if(frontend_combat_cache()!=WF_OK)fail(0x18cd06u,0);
                goto combat_resume;
            }
#endif
#if defined(WF_R18_REPLAY_TEST) && !defined(WF_R18_CAPTURE_RESULT)
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
            fail(transition.blocked_pc ? 0xf560u : gameplay.last_blocked_phase ?
                 gameplay.last_blocked_phase : boot.blocked_pc,
                 transition.blocked_pc ? transition.blocked_pc : gameplay.last_blocked_dependency ?
                 gameplay.last_blocked_dependency : video.failed_address);
    }
#ifndef WF_R18_REPLAY_TEST
frontend_soft_reset:
    frontend_settle();vdp1_sync_wait();vdp2_sync_wait();
    vdp2_scrn_display_set(0);vdp2_sync();vdp2_sync_wait();
    vdp2_tvmd_display_set();
    title_loading_hidden=1;
    frontend_bound_world=0xffffu;
    video.read_rom=wf_r18_rom_read;video.read_ram=read_ram;video.read_user=NULL;
    wf_runtime_init(&rt,bindings,count);
    goto frontend_restart;
#endif
}
