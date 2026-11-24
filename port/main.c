#include <jo/jo.h>
#ifdef __sh__
#include <internal/core.h>
extern volatile unsigned int __jo_smpc_timing_fault;
#else
extern void __jo_core_pause(void);
#endif
#include <stdint.h>
#include <string.h>
#include "wf_gameplay_bindings.h"
#include "wf_r18_bootstrap.h"
#include "wf_r18_referee.h"
#include "wf_r18_match_result.h"
#include "wf_r18_result_screen.h"
#include "wf_r18_present.h"
#include "wf_r18_ring.h"
#include "wf_r18_video.h"
#include "wf_r18_display.h"
#include "wf_r18_hud.h"
#include "wf_r18_world.h"
#include "wf_r18_transition.h"
#include "wf_r18_scsp.h"
#include "wf_jo_assets.h"
#include "wf_jo_video.h"
#include "wf_jo_audio.h"
#include "wf_jo_clock.h"
#include "wf_saturn_memory.h"
#include "wp1979_input.h"
#include "wp1980_input.h"
#include "wp1980_royal_init.h"
#ifdef WF_JO_FRONTEND
#include "wf_jo_frontend.h"
#include "wf_jo_scene_prepare.h"
#include "wf_jo_save.h"
#endif

/* Cabinet entry and separately selectable direct-combat diagnostics. Original
 * game owners compute live matches; automation supplies pad/coin levels only. */
typedef struct {
    uint32_t magic, stage, fields, ticks, draws, max_gap, debt, max_debt;
    uint32_t fault_owner, fault_dependency, rng, demo, cartridge;
    uint32_t start_field, finish_field, audio_ok, frame_tag;
    uint32_t g0166_before, g0166_ready, g0166_committed, g0166_ack;
    uint32_t handoff_debt_total, handoff_debt_max, handoff_count;
} wf_jo_diagnostic_t;
volatile wf_jo_diagnostic_t wf_jo_diagnostic = {.magic=0x574a3031u};
static wf_runtime_t rt;
#if defined(WF_JO_FRONTEND) && (defined(JO_COMPILE_WITH_PROFILER_SUPPORT) || defined(WF_JO_CAPTURE))
volatile uint32_t wf_jo_terminal_completed;
_Static_assert(64u+sizeof rt.actors<=WF_SATURN_LWRAM_DIAGNOSTIC_BYTES,
               "Terminal actor snapshot must fit its diagnostic low-RAM page");
#endif
static wf_gameplay_binding_context_t gameplay;
static wf_phase_binding_t bindings[48];
static wf_r18_video_t video;
static wf_r18_display_t display;
static wf_r18_world_ctx_t world;
static wf_r18_bootstrap_context_t boot;
static wf_r18_transition_context_t transition;
static wf_jo_clock_t arcade_clock;
static uint32_t previous_field;
static unsigned awaiting_ack, running, done, failed, selected_demo, selected_royal;
static unsigned load_requested, loaded;
static volatile uint32_t video_clock;
static unsigned loading_text_visible;
static char loading_file[16];
#ifdef WF_JO_FRONTEND
static uint32_t loading_start_field;
#endif
/* Last attempted disc operation survives a failure for the physical-console
 * photograph. No observer or loading text runs during combat. */
volatile uint32_t wf_jo_load_diagnostic[3]; /* bytes done, total, last field */
#ifdef WF_JO_FRONTEND
static wf_jo_frontend_t frontend;
static wf_jo_save_t records_save;
static unsigned frontend_state,frontend_demo_started;
#define WF_AUDIO_PREFIX_RAM (WF_SATURN_LWRAM_FRONT_RAM-32768u)
#define WF_COMBAT_CACHE_BYTES (WF_SATURN_LWRAM_FRONT_RAM-WF_SATURN_LWRAM_TILE_CACHE)
_Static_assert(WF_COMBAT_CACHE_BYTES/4096u==145u,"Permanent cabinet RAM leaves145 four-KiB sprite blocks");
#else
#define WF_COMBAT_CACHE_BYTES WF_SATURN_LWRAM_TILE_BYTES
#endif
#define RING_BLOB ((void *)WF_SATURN_LWRAM_SCENE)

/* Numeric counters are hexadecimal, clamped only on screen; the RAM snapshot
 * retains full 32-bit values. Print only on the loader or restored diagnostic
 * text surface, never from the filesystem or after gameplay takes VDP2. */
static void loading_cd_summary(unsigned row,unsigned detail)
{
    jo_fs_cd_diagnostics d;
    jo_fs_get_cd_diagnostics(&d);
    jo_printf(3,row,"CD S%04X T%04X R%04X E%04X M%04X",
        JO_MIN(d.start_read_attempts,0xffffu),JO_MIN(d.sector_wait_timeouts,0xffffu),
        JO_MIN(d.fill_read_retries,0xffffu),JO_MIN(d.command_failures,0xffffu),
        JO_MIN(d.max_read_fields,0xffffu));
    if(detail){
        jo_printf(3,row+1u,"F%08X C%04X S%08X",d.failure_fad,d.failure_command,d.failure_status);
        jo_printf(3,row+2u,"CD causa %u maxF %08X",d.failure_reason,d.max_read_fad);
    }
}

#ifdef WF_JO_FRONTEND
static void loading_progress(const char *name,size_t done_bytes,size_t total)
{
    strncpy(loading_file,name,sizeof loading_file-1u);
    loading_file[sizeof loading_file-1u]=0;
    wf_jo_load_diagnostic[0]=(uint32_t)done_bytes;
    wf_jo_load_diagnostic[1]=(uint32_t)total;
    wf_jo_load_diagnostic[2]=jo_core_get_vblank_count();
    if(!loading_text_visible)return;
    jo_printf(3,14,"Archivo: %-16s",loading_file);
    if(total)jo_printf(3,16,"%u / %u bytes          ",
            (unsigned)done_bytes,(unsigned)total);
    else jo_printf(3,16,"Consultando archivo en CD...   ");
    jo_printf(3,18,"Tiempo carga: %u s             ",
              (unsigned)((wf_jo_load_diagnostic[2]-loading_start_field)/
                         (unsigned)jo_core_get_refresh_rate()));
    loading_cd_summary(20u,0u);
}
#endif
static void loading_stage(unsigned stage,const char *label)
{
    wf_jo_diagnostic.stage=stage;
    if(loading_text_visible)jo_printf(3,12,"%02u %-27s",stage,label);
}

static void fail(uint32_t owner, uint32_t dependency)
{
    wf_jo_audio_emergency_stop();
    /* Keep the original owner/tick/RNG if a later error follows this one. */
    if(failed)return;
    loading_text_visible=0;
    wf_jo_assets_set_loading_observer(NULL);
    wf_jo_diagnostic.fault_owner=owner;
    wf_jo_diagnostic.fault_dependency=dependency;
    wf_jo_diagnostic.ticks=rt.g.frame_counter;
    wf_jo_diagnostic.rng=rt.g.rng_state;
    failed=1;
    running=0;
    /* If an incomplete publication cannot close, preserve the fault in RAM
     * and keep its resources alive. Returning to the SDK loop would publish
     * that incomplete command list without a matching snapshot. */
    if(wf_jo_video_drain_overlap(NULL,NULL)!=WF_OK)for(;;){
        if(jo_is_pad1_key_down(JO_KEY_START))jo_goto_boot_menu();
        __jo_core_pause();
    }
    wf_jo_video_diagnostics();
    jo_clear_screen();
    jo_printf(3,4,"WRESTLEFEST - R03");
    jo_printf(3,7,"PAUSA DE DIAGNOSTICO");
    jo_printf(3,10,"Owner: %08X",(unsigned)owner);
    jo_printf(3,12,"Dato:  %08X",(unsigned)dependency);
    jo_printf(3,14,"Tick:  %u",(unsigned)rt.g.frame_counter);
    if(!loaded){
        jo_printf(3,16,"Ult. CD: %-16s",loading_file);
        jo_printf(3,18,"Carga %u: %u / %u bytes",(unsigned)wf_jo_diagnostic.stage,
                (unsigned)wf_jo_load_diagnostic[0],
                (unsigned)wf_jo_load_diagnostic[1]);
    }
    loading_cd_summary(20u,1u);
    jo_printf(3,23,"Fotografia esta pantalla.");
    jo_printf(3,24,"START: menu de la consola");
}

static int cartridge_probe(void)
{
    volatile uint32_t *const words=(volatile uint32_t *)0x22400000u;
    uint32_t saved[16];
    unsigned good=1;
    wf_jo_diagnostic.cartridge=JO_RAM_CARTRIDGE_ID;
    if(JO_RAM_CARTRIDGE_ID!=JO_RAM_CARTRIDGE_ID_32MBITS) return 0;
    JO_RAM_CARTRIDGE_ENABLE=1;
    JO_SCU_ASR0=JO_RAM_CARTRIDGE_ASR0;
    JO_SCU_AREF=JO_RAM_CARTRIDGE_AREF;
    for(unsigned i=0;i<16;++i) saved[i]=words[i*0x10000u];
    for(unsigned i=0;i<16;++i) words[i*0x10000u]=0x57520000u^(i*0x01010101u);
    for(unsigned i=0;i<16;++i)
        if(words[i*0x10000u]!=(0x57520000u^(i*0x01010101u)))good=0;
    for(unsigned i=0;i<16;++i) words[i*0x10000u]=saved[i];
    return (int)good;
}

static wf_status_t read_ram(wf_runtime_t *r,uint32_t addr,uint16_t *value,void *user)
{
    (void)user;
    if(!value)return WF_BAD_STATE;
    switch(addr){
    case 0x1c0062u:case 0x1c0063u:*value=r->g.global_0063;return WF_OK;
    case 0x1c004eu:*value=(uint16_t)((r->g.credit_remainder_004e<<8)|r->g.credit_count_004f);return WF_OK;
    case 0x1c004au:*value=r->g.credit_cycle_units_004b;return WF_OK;
    default:video.failed_address=addr;return WF_BLOCKED;
    }
}
static wf_status_t text_owner(wf_runtime_t*r,uint32_t pc,void*u)
{
    uint16_t id=(uint16_t)r->m68k_regs.d[0];
    return pc==0x2503cu?wf_r18_text_2503c(r,id,u):wf_r18_number_26122(r,id,u);
}
static wf_status_t palette_reset(wf_runtime_t*r,void*u)
{
    wf_r18_video_t*v=u;(void)r;
    memset(v->palette_map,0,sizeof v->palette_map);
    memset(v->palette_used,0,sizeof v->palette_used);return WF_OK;
}
static wf_status_t map_26e66(wf_runtime_t*r,void*u)
{
    (void)u;
    return wf_r18_ring_update((int16_t)r->g.global_17e6,(int16_t)r->g.global_17ee,
        (int16_t)r->g.global_17ea,(int16_t)r->g.global_17f2_word,boot.priority_140010)==0?WF_OK:WF_BLOCKED;
}
static wf_status_t input_word(wf_runtime_t*r,uint32_t addr,uint16_t*out,void*u)
{
    (void)r;(void)u;addr&=0xffffffu;
    if(!out||(addr&1u))return WF_BAD_STATE;
    if(addr>=0x140020u&&addr<=0x140026u){
        *out=(uint16_t)(0xff00u|gameplay.input_hw_low[(addr-0x140020u)/2u]);return WF_OK;
    }
    /* Royal's original celebration actor has +$8A=0. $517C reads its
     * physical WORD from ROM, independently of the cabinet input ports. */
    if(addr<=WF_JO_ROM_BYTES-2u){
        uint8_t bytes[2];
        if(wf_jo_assets_rom_read(addr,bytes,sizeof bytes))return WF_BLOCKED;
        *out=(uint16_t)(((uint16_t)bytes[0]<<8)|bytes[1]);return WF_OK;
    }
    return WF_BAD_STATE;
}
static wf_status_t world_288f0(wf_runtime_t*r,uint8_t a[4],uint8_t b,void*u)
{(void)u;return wf_r18_world_288f0(r,a,b,&world);}
static wf_status_t world_26b68(wf_runtime_t*r,uint8_t a,uint16_t b,void*u)
{(void)u;return wf_r18_world_26b68(r,a,b,&world);}
static wf_status_t render(wf_runtime_t*r,wf_actor_raw_t*const*a,size_t n,void*u)
{
    if(!wf_jo_audio_service_cooperate())return WF_BAD_STATE;
    jo_profile_begin(JO_PROFILE_USER(7));
    wf_status_t result=wf_r18_present_batch(r,a,n,u);
    jo_profile_end(JO_PROFILE_USER(7));return result;
}
static int service_during_assets(void *unused)
{(void)unused;return wf_jo_audio_service_cooperate()?0:-1;}

/* A blocking arcade wait retains its original IRQ2/coin deferral. */
static wf_status_t service_during_wait(void *unused)
{(void)unused;return wf_jo_audio_service_cooperate()?WF_OK:WF_BAD_STATE;}

static wf_status_t service_during_drain(void *unused)
{
    (void)unused;
    if(!wf_jo_audio_service_cooperate())return WF_BAD_STATE;
#ifdef WF_JO_FRONTEND
    if(frontend.handed_off)return wf_jo_frontend_combat_coins(&frontend);
#endif
    return WF_OK;
}

/* Only original terminal/transition owners call this blocking wait. Jo owns
 * the synchronization and continues audio service; elapsed fields are charged
 * to the arcade clock when the callback returns. */
extern void __jo_core_synch(void);
static wf_status_t wait_21e6(wf_runtime_t*r,uint16_t raw,void*u)
{
    /* Close only the preceding frozen presentation epoch. Do not publish
     * live HUD/maps here or count video backlog as an original wait period. */
    wf_status_t closed=wf_jo_video_drain_overlap(service_during_wait,NULL);
    if(closed!=WF_OK)return closed;
    uint32_t start=jo_core_get_vblank_count(),periods=(uint32_t)raw+1u;
    wf_jo_clock_t wait_clock;(void)r;(void)u;
    if(!wf_jo_clock_advance(&arcade_clock,start))return WF_BAD_STATE;
    wf_jo_clock_start(&wait_clock,start);
    wait_clock.remainder=arcade_clock.remainder;
    while(wait_clock.pending<periods){
        if(!wf_r18_scsp_service())return WF_BAD_STATE;
        if(wf_jo_video_repeat()!=WF_OK)return WF_BLOCKED;
        __jo_core_synch();
        if(!wf_jo_clock_advance(&wait_clock,jo_core_get_vblank_count()))return WF_BAD_STATE;
    }
    /* Original waits consume arcade VBlank periods without advancing combat.
     * Do not turn those deliberate waits into catch-up simulation afterwards. */
    arcade_clock.last_field=wait_clock.last_field;
    arcade_clock.remainder=wait_clock.remainder;
    arcade_clock.pending+=wait_clock.pending-periods;
    return WF_OK;
}
static wf_status_t result_wait(wf_runtime_t*r,uint16_t periods,void*u)
{return periods?wait_21e6(r,(uint16_t)(periods-1u),u):WF_BAD_STATE;}
static wf_status_t result_commit(void*u)
{return wf_r18_hud_commit(u);}
static wf_status_t royal_result_commit(void*u)
{
    wf_r18_video_t*v=u;wf_status_t s=wf_r18_hud_commit(v);
    if(s!=WF_OK)return s;
    for(unsigned role=WF_R18_PALETTE_TEXT;role<=WF_R18_PALETTE_BG;++role){
        s=wf_r18_palettes_commit(v,role);if(s!=WF_OK)return s;
    }
    return WF_OK;
}
static wf_status_t result_level(wf_runtime_t*r,unsigned high,void*u)
{return high?wait_21e6(r,0,u):WF_OK;}
static wf_status_t result_sound(wf_runtime_t*r,void*u)
{
    wf_status_t status=wf_r18_result_sound_20156(r,u);
    if(status!=WF_OK)return status;
    /* Original $2052 suppresses attract audio only through DIP bit 5.
     * $20156's selected low word of D0 remains caller-visible. */
    if(!r->g.credited_route&&(r->g.config_0066&0x20u))return WF_OK;
    return wf_r18_scsp_audio(r,(uint16_t)r->m68k_regs.d[0],u);
}

static uint8_t controls(unsigned p)
{
    uint8_t level=0xffu;
    int port=jo_get_player_input_port((int)p);
    if(port<0)return level;
    p=(unsigned)port;
    if(jo_is_input_key_pressed(p,JO_KEY_RIGHT))level&=(uint8_t)~1u;
    if(jo_is_input_key_pressed(p,JO_KEY_LEFT))level&=(uint8_t)~2u;
    /* $F2D6 bit2 increases world Y; the sprite raster reverses Y, so it
     * moves up on screen. Bit3 decreases world Y and moves down. */
    if(jo_is_input_key_pressed(p,JO_KEY_UP))level&=(uint8_t)~4u;
    if(jo_is_input_key_pressed(p,JO_KEY_DOWN))level&=(uint8_t)~8u;
    if(jo_is_input_key_pressed(p,JO_KEY_A))level&=(uint8_t)~0x10u;
    if(jo_is_input_key_pressed(p,JO_KEY_B))level&=(uint8_t)~0x20u;
    if(jo_is_input_key_pressed(p,JO_KEY_C))level&=(uint8_t)~0x40u;
    if(jo_is_input_key_pressed(p,JO_KEY_START))level&=(uint8_t)~0x80u;
    return level;
}
static int palette_and_ring(void)
{
    return wf_r18_hud_commit(&video)==WF_OK &&
        wf_r18_palettes_commit(&video,WF_R18_PALETTE_FG)==WF_OK &&
        wf_r18_palettes_commit(&video,WF_R18_PALETTE_BG)==WF_OK &&
        wf_r18_ring_update((int16_t)display.scroll[0],(int16_t)(2048u-display.scroll[1]),
            (int16_t)display.scroll[2],(int16_t)(2048u-display.scroll[3]),boot.priority_140010)==0;
}

static int init_platform(void)
{
    uint32_t size=0;size_t count;
    loading_stage(5,"RING Y VIDEO");
    memset(&gameplay,0,sizeof gameplay);
    memset(&video,0,sizeof video);
    memset(&world,0,sizeof world);
    memset(&boot,0,sizeof boot);
    if(wf_jo_assets_file_size("R18RING.BIN",&size)||size>WF_SATURN_LWRAM_SCENE_BYTES||
       wf_jo_assets_file_read("R18RING.BIN",0,RING_BLOB,size)){fail(0x18cd01u,size);return 0;}
    /* From here the game owns the VDP2 maps/font. Never print into them. */
    loading_text_visible=0;
    wf_jo_assets_set_loading_observer(NULL);
    if(wf_saturn_yaul_video_init()!=WF_OK||wf_r18_ring_bind(RING_BLOB,size)){
        fail(0x18cd04u,size);return 0;
    }
    if(wf_r18_hud_bind(&video,wf_jo_assets_file_read)!=WF_OK){fail(0x18cd08u,0);return 0;}
    wf_jo_diagnostic.stage=6;
    video.read_rom=wf_r18_rom_read;video.read_ram=read_ram;world.read_rom=wf_r18_rom_read;
    boot.palette_2aea=wf_r18_palette_2aea;boot.palette_release_2b58=wf_r18_palette_2b58;
    boot.palette_reset=palette_reset;boot.map_26e66=map_26e66;
    boot.video_batch=wf_r18_video_batch;boot.fg0_batch=wf_r18_video_batch;
    boot.text_owner=text_owner;boot.user=&video;
    count=wf_gameplay_bindings_build(&gameplay,bindings,48);
    if(!count){fail(0x18cd05u,0);return 0;}
    wf_runtime_init(&rt,bindings,count);
    wf_r18_present_init(wf_jo_assets_file_read,NULL,0,NULL,0);
    return 1;
}

static int prepare_gameplay(void)
{
    /* The opaque cabinet image already returned to the frontend before this
     * point. Start fresh platform transactions for the newly handed-off match
     * while preserving every native runtime byte imported at F8C. */
    size_t count=wf_gameplay_bindings_build(&gameplay,bindings,48);
    if(!count){fail(0x18cd05u,0);return 0;}
    rt.bindings=bindings;rt.binding_count=count;
    wf_r18_video_ring_bind(&video);
    gameplay.cabinet_words_valid=1;gameplay.cabinet_word_140020=0xffffu;
    gameplay.cabinet_word_140022=0xffffu;gameplay.camera_valid=1;
    gameplay.camera.scene_index=rt.g.global_007e;
    gameplay.camera.scroll0_x=rt.g.global_17e6;gameplay.camera.scroll1_x=rt.g.global_17ea;
    gameplay.camera.scroll0_y=rt.g.global_17ee;gameplay.camera.scroll1_y=rt.g.global_17f2_word;
    gameplay.input_valid_mask=0x1ffu;memset(gameplay.input_hw_low,0xff,sizeof gameplay.input_hw_low);
    gameplay.audio_command=wf_r18_scsp_audio;
    gameplay.referee_direction_20c8=wf_r18_referee_direction_20c8;
    gameplay.referee_display_2067c=wf_r18_referee_display_2067c;
    gameplay.referee_number_2067c=wf_r18_referee_number_2067c;
    gameplay.referee_clear_206fe=wf_r18_referee_clear_206fe;
    gameplay.referee_tag_phase6_90d6=wf_r18_tag_bonus_90d6;
    gameplay.referee_result_20156=result_sound;
    gameplay.participant_input_word=input_word;gameplay.participant_commit_batch=wf_r18_video_commit;
    gameplay.post7_wait_21e6=wait_21e6;gameplay.post7_fg0_batch=wf_r18_video_batch;
    gameplay.post7_video_batch=wf_r18_video_batch;gameplay.post7_resource_2aea=wf_r18_palette_2aea;
    gameplay.post7_resource_2b58=wf_r18_palette_2b58;gameplay.post7_raw_2503c=wf_r18_text_2503c;
    gameplay.post7_raw_9aca=wf_r18_strip_9aca;gameplay.post7_fg0_peek_long=wf_r18_video_peek;
    gameplay.post7_render_2836_batch=render;gameplay.branchb_288f0=world_288f0;
    gameplay.branchb_26b68=world_26b68;gameplay.r17_owner_user=&video;
    if(wf_r18_display_init(&display,&rt,wf_r18_rom_read,NULL)!=WF_OK){fail(0x27048u,0);return 0;}
    transition=(wf_r18_transition_context_t){.video=&video,.world=&world,.display=&display,
        .render_queue=&gameplay.render_queue,.camera=&gameplay.camera,.scene_blob=RING_BLOB,
        .scene_capacity=WF_SATURN_LWRAM_SCENE_BYTES,.file_size=wf_jo_assets_file_size,
        .file_read=wf_jo_assets_file_read,.priority_140010=&boot.priority_140010};
    wf_r18_transition_bind(&transition);gameplay.transition_reset=wf_r18_transition_reset;
    gameplay.transition_barrier=wf_r18_transition_barrier;
    if(!palette_and_ring()){fail(0x26e66u,video.failed_address);return 0;}
    /* No command list has been built yet. First live frame commits its maps
     * and textures together; committing an empty list here would reveal garbage. */
    wp1979_demo_active=selected_demo;
    wp1980_demo_active=selected_demo;
    previous_field=jo_core_get_vblank_count();
#ifdef WF_JO_FRONTEND
    /* Preserve the sub-tick phase delivered by the original scene owner. */
    if(!wf_jo_clock_advance(&arcade_clock,previous_field)){fail(0x434c4f43u,0);return 0;}
    /* A late phase must never shorten the following scene to catch up.
     * Record its missed budget permanently before starting the next phase;
     * acceptance requires these counters to be zero, not merely current debt. */
    wf_jo_diagnostic.handoff_debt_total+=arcade_clock.pending;
    if(arcade_clock.pending>wf_jo_diagnostic.handoff_debt_max)
        wf_jo_diagnostic.handoff_debt_max=arcade_clock.pending;
    ++wf_jo_diagnostic.handoff_count;arcade_clock.pending=0;
#else
    wf_jo_clock_start(&arcade_clock,previous_field);
#endif
    wf_jo_diagnostic.start_field=previous_field;wf_jo_diagnostic.stage=10;
    awaiting_ack=0;running=1;
    return 1;
}

#ifndef WF_JO_FRONTEND
static int start_native_match(void)
{
    wf_r18_launch_config_t launch;wf_status_t s;uint32_t size=0;
    if(!init_platform())return 0;
    memset(&launch,0,sizeof launch);
    launch.human=0;launch.partner=1;launch.cabinet_config=0x300u;
    launch.rng_state=1085u; /* Explicit reproducible TAG test fixture. */
    wf_r17_m68k_software_reset(&launch.registers);
    if(selected_royal){
        wp1980_royal_init_info_t original;
        if(wf_jo_assets_file_size("ROYINIT.BIN",&size)||size!=WP1980_ROYINIT_BYTES||
           wf_jo_assets_file_read("ROYINIT.BIN",0,RING_BLOB,size)){
            fail(0x18cd12u,size);return 0;
        }
        s=wp1980_royal_init_apply(&rt,&video,&world,RING_BLOB,size,&original);
        if(s==WF_OK){
            boot.priority_140010=original.priority;
            if(wf_jo_assets_file_size("R18RING.BIN",&size)||
               wf_jo_assets_file_read("R18RING.BIN",0,RING_BLOB,size)||
               wf_r18_ring_bind(RING_BLOB,size)){fail(0x18cd01u,size);return 0;}
        }
    }else s=wf_r18_direct_boot(&rt,&boot,&launch);
    if(s!=WF_OK){fail(boot.blocked_pc,video.failed_address);return 0;}
    return prepare_gameplay();
}
#else
/* Backup BIOS code uses a short sprite-cache loan, never combat memory or a
 * permanent heap allocation. Drain every renderer/worker owner before the
 * loan. An unavailable/corrupt/full backup device does not stop the game. */
static wf_status_t frontend_records_access(uint8_t *ram,size_t bytes,unsigned restore)
{
    if(!ram||bytes<0xc2u+WF_R18_SAVE_RANKING_BYTES)return WF_BAD_STATE;
    uint8_t *rankings=ram+0xc2u;
    if(!restore&&!wf_jo_save_needs_store(&records_save,rankings))return WF_OK;
    wf_status_t drained=wf_jo_video_drain_overlap(service_during_wait,NULL);
    if(drained!=WF_OK)return drained;
    if(!wf_jo_audio_service_cooperate())return WF_BAD_STATE;
    wf_jo_assets_demand_end();
    wf_jo_scene_prepare_cancel();
    void *workspace=NULL;
    if(wf_jo_assets_borrow_sprite_cache_tail(WF_JO_SAVE_WORKSPACE_BYTES,&workspace)){
        records_save.status=WF_JO_SAVE_UNAVAILABLE;return WF_OK;
    }
    if(restore)(void)wf_jo_save_load(&records_save,rankings,workspace,WF_JO_SAVE_WORKSPACE_BYTES);
    else (void)wf_jo_save_store(&records_save,rankings,workspace,WF_JO_SAVE_WORKSPACE_BYTES);
    int returned=wf_jo_assets_return_sprite_cache_tail(workspace);
    if(returned)return WF_BAD_STATE;
    return wf_jo_audio_service_cooperate()?WF_OK:WF_BAD_STATE;
}
static wf_status_t frontend_records_restore(void *user,uint8_t *ram,size_t bytes)
{(void)user;return frontend_records_access(ram,bytes,1u);}
static wf_status_t frontend_records_store(void *user,const uint8_t *ram,size_t bytes)
{(void)user;return frontend_records_access((uint8_t *)(uintptr_t)ram,bytes,0u);}
static uint16_t frontend_input(void *user,const wf_jo_frontend_t *f,unsigned port,uint16_t physical)
{
    (void)user;
#ifdef WF_JO_AUTORUN
    if(!frontend_demo_started&&f->driver.phase==WF_R18_DRIVER_TITLE&&f->driver.scene.title.pc>=0x837eu){
        wp1980_frontend_input_begin(f->arcade_ticks);frontend_demo_started=1;
    }
    if(!frontend_demo_started)return physical;
    if(f->driver.phase==WF_R18_DRIVER_AC0)return physical;
    if(WF_JO_AUTORUN_MODE!=2&&f->driver.phase==WF_R18_DRIVER_MODE){
        uint16_t word=(uint16_t)(physical|0xffu);if(port)return word;
        if(f->driver.ram[0x161]&1u)word&=(uint16_t)~1u;else word&=(uint16_t)~0x10u;
        return word;
    }
    if(WF_JO_AUTORUN_MODE!=2&&f->driver.phase==WF_R18_DRIVER_ROSTER){
        uint16_t word=(uint16_t)(physical|0xffu);
        if(!port&&(f->arcade_ticks&7u)<4u)word&=(uint16_t)~0x10u;
        return word;
    }
    return wp1980_frontend_input(f->driver.phase,f->driver.ram,f->arcade_ticks,port,physical);
#else
    (void)f;(void)port;return physical;
#endif
}
static int init_frontend(void)
{
    wf_jo_save_init(&records_save,NULL,NULL);
    wf_jo_frontend_config_t config={.runtime=&rt,.video=&video,.world=&world,.display=&display,
        .clock=&arcade_clock,.ram=(uint8_t *)WF_SATURN_LWRAM_FRONT_RAM,
        .ram_bytes=WF_SATURN_LWRAM_FRONT_RAM_BYTES,.scene_blob=RING_BLOB,
        .scene_capacity=WF_SATURN_LWRAM_SCENE_BYTES,.priority_140010=&boot.priority_140010,
        .render_counts=gameplay.render_queue.count,.combat_read_ram=read_ram,.input_word=frontend_input,
        .reset_ready=frontend_records_restore,.records_ready=frontend_records_store};
    if(wf_jo_frontend_init(&frontend,&config,1085u)!=WF_OK){fail(0x654u,0);return 0;}
    frontend_state=1;wf_jo_diagnostic.stage=8;return 1;
}
static int frontend_step(void)
{
    wf_status_t status;
    uint32_t owner,dependency;
    if(frontend_state==1){
        status=wf_jo_frontend_run(&frontend);
        if(status!=WF_OK)goto failed;
        frontend_state=2;
    }
    status=wf_jo_frontend_flush(&frontend);
    if(status==WF_BLOCKED&&!frontend.blocked_dependency)return 1;
    if(status!=WF_OK)goto failed;
    selected_royal=(rt.g.mode_0161&1u)!=0;
    if(!prepare_gameplay())return 0;
    frontend_state=0;return 1;
failed:
    owner=frontend.blocked_pc?frontend.blocked_pc:frontend.driver.pc;
    dependency=frontend.blocked_dependency;
    if(!dependency&&frontend.driver.phase==WF_R18_DRIVER_FATAL)
        dependency=frontend.driver.blocked_dependency;
    /* FRNT marks an absent owner. ST|status is an adapter return code,
     * outside the24-bit ROM address space; preserve genuine faults first. */
    fail(owner?owner:0x46524e54u,
         dependency?dependency:(0x53540000u|((uint32_t)status&0xffffu)));
    return 0;
}
static int resume_frontend(uint32_t owner)
{
#if defined(JO_COMPILE_WITH_PROFILER_SUPPORT) || defined(WF_JO_CAPTURE)
    /* A result owner can immediately change actors. Retain the exact
     * post-ACK image in unused low RAM before entering that owner, so an
     * asynchronous observer never confuses a later result with this seam. */
    volatile uint32_t *terminal=(volatile uint32_t *)WF_SATURN_LWRAM_DIAGNOSTIC;
    uint32_t terminal_field=jo_core_get_vblank_count();
    terminal[0]=0;
    jo_memcpy((void *)((uintptr_t)terminal+64u),rt.actors,sizeof rt.actors);
    terminal[1]=1;terminal[2]=sizeof rt.actors;terminal[3]=rt.g.frame_counter;
    terminal[4]=terminal_field;terminal[5]=rt.g.mode_0161&1u;
    terminal[6]=owner;terminal[7]=rt.g.rng_state;
    terminal[8]=wf_jo_diagnostic.start_field;terminal[9]=frontend.arcade_ticks;
    terminal[10]=arcade_clock.pending;terminal[11]=arcade_clock.remainder;
    terminal[12]=terminal[13]=terminal[14]=terminal[15]=0;
    terminal[0]=0x57465431u; /* WFT1; publish only the complete actor image. */
#endif
    running=0;wf_jo_diagnostic.stage=22;
    if(!wf_jo_clock_advance(&arcade_clock,jo_core_get_vblank_count())){fail(0x434c4f43u,0);return 0;}
    wf_jo_diagnostic.handoff_debt_total+=arcade_clock.pending;
    if(arcade_clock.pending>wf_jo_diagnostic.handoff_debt_max)
        wf_jo_diagnostic.handoff_debt_max=arcade_clock.pending;
    ++wf_jo_diagnostic.handoff_count;
    /* The admitted post-ACK boundary still has virtual VBlank HIGH. Host
     * RAM export is transport work, not an arcade LOW transition. */
#if defined(JO_COMPILE_WITH_PROFILER_SUPPORT) || defined(WF_JO_CAPTURE)
    terminal[12]=jo_core_get_vblank_count();
#endif
    if(wf_jo_frontend_after_combat_at_level(&frontend,owner,1u)!=WF_OK){
        fail(frontend.blocked_pc?frontend.blocked_pc:owner,frontend.blocked_dependency);return 0;
    }
#if defined(JO_COMPILE_WITH_PROFILER_SUPPORT) || defined(WF_JO_CAPTURE)
    terminal[13]=jo_core_get_vblank_count();terminal[14]=frontend.arcade_ticks;
    terminal[15]=1;++wf_jo_terminal_completed;
#endif
    frontend_state=1;return 1;
}
#endif

static int load_resources(void)
{
    size_t available;void *tail;uint32_t audio_size=0;
    size_t cache_capacity=WF_COMBAT_CACHE_BYTES;
#ifdef WF_JO_FRONTEND
    loading_start_field=jo_core_get_vblank_count();
#endif
    loading_stage(1,"CONSULTA DE ARCHIVOS");
#ifdef WF_JO_FRONTEND
    uint32_t pack_size=0;void *prefix_workspace=NULL;
    /* Both music prefixes fit the new compact pack's cartridge tail. Older
     * 8 KiB packs retain the separate 32 KiB LWRAM lease as a fallback. This
     * decision precedes binding the cache, so the leases can never overlap. */
    if(wf_jo_assets_file_size("SPRITES.WFP",&pack_size)){fail(0x18cd10u,1);return 0;}
    if(wf_jo_assets_file_size("SFX.DAT",&audio_size)){fail(0x18cd10u,2);return 0;}
    uint64_t resident=WF_JO_ROM_BYTES+(((uint64_t)pack_size+31u)&~UINT64_C(31))+
        (((uint64_t)audio_size+31u)&~UINT64_C(31));
    if(resident+98304u>WF_JO_CART_BYTES){
        prefix_workspace=(void *)WF_AUDIO_PREFIX_RAM;cache_capacity-=32768u;
    }
#endif
    loading_stage(2,"ROM Y LUCHADORES");
    /* The probe uses the cache-through alias. All subsequent content loads
     * and reads use this one cached alias: SH7604 manual 7.1.5/8.4.2.
     * ROM and packed graphics are immutable after preload. Jo's DMA helper
     * invalidates any destination cache lines when it performs a transfer. */
    int asset_status=wf_jo_assets_load((void *)0x02400000u,0x400000u,
            (void *)WF_SATURN_LWRAM_TILE_CACHE,cache_capacity);
    if(asset_status){
        uint32_t detail=wf_jo_assets_last_detail()&UINT32_C(0x00ffffff);
        fail(0x18cd10u,((uint32_t)(-asset_status)&0xffu)<<24|detail);return 0;
    }
    loading_stage(3,"FONDOS Y ESCENAS");
    if(wf_jo_assets_preload_scenes((void *)WF_SATURN_LWRAM_SCENE_RESIDENT,
            WF_SATURN_LWRAM_SCENE_RESIDENT_BYTES)){fail(0x18cd11u,0);return 0;}
    loading_stage(4,"VOCES Y EFECTOS");
    tail=wf_jo_assets_cart_tail(&available);
    if(wf_jo_assets_file_size("SFX.DAT",&audio_size)||audio_size>available||
       wf_jo_assets_file_read("SFX.DAT",0,tail,audio_size)){
        fail(0x2052u,audio_size);return 0;
    }
    /* Finish every synchronous graphics/fixture read before music takes the
     * CD stream. Jo's filesystem and streaming player share one CD block. */
#ifdef WF_JO_FRONTEND
    if(!init_platform())return 0;
#else
    if(!start_native_match())return 0;
#endif
    wf_jo_diagnostic.stage=7;
    if(!wf_jo_audio_init(tail,audio_size)){fail(0x2052u,0x68000u);return 0;}
#ifdef WF_JO_FRONTEND
    if(prefix_workspace&&!wf_jo_audio_set_prefix_workspace(prefix_workspace,32768u)){
        fail(0x2052u,0x68004u);return 0;
    }
#endif
    size_t audio_aligned=((size_t)audio_size+31u)&~(size_t)31u;
    if(audio_aligned>available||wf_jo_audio_prefetch_music((uint8_t *)tail+audio_aligned,
            available-audio_aligned)!=0x900u){fail(0x2052u,0x68002u);return 0;}
#ifdef WF_JO_FRONTEND
    const uint8_t first_cue=0x02u;
#else
    const uint8_t first_cue=0x04u;
#endif
    if(!wf_jo_audio_prime_music(first_cue)){fail(0x2052u,0x68003u);return 0;}
    wf_jo_assets_set_service_hook(service_during_assets,NULL);
    wf_jo_diagnostic.audio_ok=1;
#ifdef WF_JO_FRONTEND
    if(!init_frontend())return 0;
    previous_field=arcade_clock.last_field;
#else
    previous_field=jo_core_get_vblank_count();
    wf_jo_clock_start(&arcade_clock,previous_field);
    wf_jo_diagnostic.stage=10;
#endif
    wf_jo_diagnostic.start_field=previous_field;
    loaded=1;
    return 1;
}

static void game_frame(void)
{
    wf_status_t s;
    uint32_t now=jo_core_get_vblank_count();video_clock=now;
    wf_jo_diagnostic.fields=now;
    if(failed){if(jo_is_pad1_key_down(JO_KEY_START))jo_goto_boot_menu();return;}
#ifdef __sh__
    if(__jo_smpc_timing_fault){fail(0x534d5043u,__jo_smpc_timing_fault);return;}
#endif
    if(!loaded){
        if(load_requested){load_resources();return;}
#ifdef WF_JO_AUTORUN
        selected_demo=1;load_requested=1;
        selected_royal=(WF_JO_AUTORUN_MODE==2);
#else
        if(jo_is_pad1_key_down(JO_KEY_LEFT)||jo_is_pad1_key_down(JO_KEY_RIGHT)){
            selected_royal=!selected_royal;
            jo_printf(3,10,selected_royal?"ROYAL RUMBLE - PRUEBA NATIVA":"COMBATE TAG  - PRUEBA NATIVA");
        }
        if(jo_is_pad1_key_down(JO_KEY_A)||jo_is_pad1_key_down(JO_KEY_START)){selected_demo=0;load_requested=1;}
        if(jo_is_pad1_key_down(JO_KEY_B)){selected_demo=1;load_requested=1;}
#endif
        if(load_requested){jo_clear_screen();jo_printf(3,10,"CARGANDO GRAFICOS Y AUDIO...");
            jo_printf(3,14,"Todos los luchadores en RAM.");}
        return;
    }
    jo_profile_begin(JO_PROFILE_USER(0));
    unsigned audio_serviced=wf_r18_scsp_service();
    jo_profile_end(JO_PROFILE_USER(0));
    if(!audio_serviced){fail(0x2052u,0x68001u);return;}
#ifdef WF_JO_FRONTEND
    if(frontend_state){frontend_step();return;}
#endif
    if(done){wf_jo_video_repeat();return;}
    if(!running)return;
#ifdef WF_JO_FRONTEND
    /* The cabinet's coin IRQ remains live even on repeated display fields. */
    if(wf_jo_frontend_combat_coins(&frontend)!=WF_OK){
        fail(frontend.blocked_pc?frontend.blocked_pc:0x434u,frontend.blocked_dependency);return;
    }
#endif
    if(!wf_jo_clock_advance(&arcade_clock,now)){fail(0x434c4f43u,0);return;}
    /* A repeated Saturn field cannot acknowledge a new arcade frame. */
    if(!arcade_clock.pending){wf_jo_video_repeat();return;}
    if(awaiting_ack){
#ifdef WF_JO_FRONTEND
        unsigned accepted=0;
        if(wf_jo_frontend_combat_start(&frontend,&accepted)!=WF_OK){
            fail(frontend.blocked_pc?frontend.blocked_pc:0x978u,frontend.blocked_dependency);return;
        }
        if(accepted){
            /* IRQ3 has consumed this boundary and already cleared the old
             * actors through $978/$8AE. Do not export them a second time. */
            uint32_t missed=frontend.combat_start_debt;
            wf_jo_diagnostic.handoff_debt_total+=missed;
            if(missed>wf_jo_diagnostic.handoff_debt_max)wf_jo_diagnostic.handoff_debt_max=missed;
            ++wf_jo_diagnostic.handoff_count;
            awaiting_ack=0;running=0;frontend_state=1;wf_jo_diagnostic.stage=8;
            return;
        }
#endif
        jo_profile_begin(JO_PROFILE_USER(1));
        wf_runtime_vblank_ack(&rt);s=wf_runtime_frame(&rt);awaiting_ack=0;
        jo_profile_end(JO_PROFILE_USER(1));
        wf_jo_diagnostic.g0166_ack=rt.g.global_0166;
        wf_jo_diagnostic.ticks=rt.g.frame_counter;wf_jo_diagnostic.rng=rt.g.rng_state;
        if(s==WF_BLOCKED&&(wf_r18_tag_match_complete(&rt)||wf_r18_royal_game_over(&rt)
#ifdef WF_JO_FRONTEND
                ||wf_r18_attract_timeout(&rt)||((rt.g.mode_0161&1u)&&(rt.g.global_16c4>>8))
#endif
                )){
            /* This admitted arcade boundary acknowledges the final frame. */
            if(!wf_jo_clock_take(&arcade_clock)){fail(0x434c4f43u,1);return;}
            if(wf_jo_video_drain_overlap(service_during_drain,NULL)!=WF_OK){
                fail(0x56445032u,rt.g.frame_counter);return;
            }
            wf_jo_diagnostic.stage=20;wf_jo_diagnostic.finish_field=jo_core_get_vblank_count();
#ifdef WF_JO_FRONTEND
            if(wf_r18_attract_timeout(&rt)){resume_frontend(0x1120u);return;}
            if((rt.g.mode_0161&1u)&&(rt.g.global_16c4>>8)){resume_frontend(0x1b4eu);return;}
            if(!selected_royal){resume_frontend(0x11b6u);return;}
            resume_frontend(0x1afcu);return;
#endif
            s=selected_royal?wf_r18_result_royal_game_over(&rt,&video,result_level,result_wait,royal_result_commit,&video):
                wf_r18_result_in_ring(&rt,&video,result_wait,result_commit,&video);
            if(s!=WF_OK){fail(0x11ceu,video.failed_address);return;}
#ifdef WF_JO_FRONTEND
            resume_frontend(0x1b1eu);return;
#else
            done=1;wf_jo_diagnostic.stage=21;
#endif
            wf_jo_video_repeat();return;
        }
        if(s!=WF_OK){fail(gameplay.last_blocked_phase,gameplay.last_blocked_dependency);return;}
    }
    wf_jo_diagnostic.debt=arcade_clock.pending;wf_jo_diagnostic.max_debt=arcade_clock.max_pending;
    if(!wf_jo_clock_take(&arcade_clock)){wf_jo_video_repeat();return;}
    jo_profile_begin(JO_PROFILE_USER(2));
    for(unsigned i=0;i<4;++i)gameplay.input_hw_low[i]=controls(i);
    if(selected_demo)gameplay.input_hw_low[0]=selected_royal?wp1980_input(&rt,gameplay.input_hw_low[0]):
        wp1979_input(rt.g.frame_counter,gameplay.input_hw_low[0]);
    wf_jo_diagnostic.demo=selected_royal?wp1980_demo_active:wp1979_demo_active;
    gameplay.cabinet_word_140020=(uint16_t)(0xff00u|gameplay.input_hw_low[0]);
    gameplay.cabinet_word_140022=(uint16_t)(0xff00u|gameplay.input_hw_low[1]);
    jo_profile_end(JO_PROFILE_USER(2));
    wf_jo_diagnostic.g0166_before=rt.g.global_0166;
    jo_profile_begin(JO_PROFILE_USER(3));
    s=wf_runtime_frame(&rt);
    jo_profile_end(JO_PROFILE_USER(3));
    wf_jo_diagnostic.g0166_ready=rt.g.global_0166;
    if(s==WF_BLOCKED&&rt.frame_ready){
        jo_profile_begin(JO_PROFILE_USER(4));
        if(wf_r18_display_irq(&rt,&display)!=WF_OK||wf_r18_world_commit(&rt,&world)!=WF_OK||
           wf_r18_hud_irq(&rt,&video)!=WF_OK||!palette_and_ring()){
            fail(0x27012u,video.failed_address);return;
        }
        jo_profile_end(JO_PROFILE_USER(4));
        jo_profile_begin(JO_PROFILE_USER(5));
        if(wf_jo_video_commit()!=WF_OK){fail(0x56445032u,rt.g.frame_counter);return;}
        jo_profile_end(JO_PROFILE_USER(5));
        wf_jo_diagnostic.g0166_committed=rt.g.global_0166;
        awaiting_ack=1;++wf_jo_diagnostic.draws;
        uint32_t gap=now-previous_field;if(gap>wf_jo_diagnostic.max_gap)wf_jo_diagnostic.max_gap=gap;
        previous_field=now;
        if(arcade_clock.pending&&wf_jo_video_allow_overlap()!=WF_OK){
            fail(0x56445031u,rt.g.frame_counter);return;
        }
        return;
    }
    fail(transition.blocked_pc?0xf560u:gameplay.last_blocked_phase,
        transition.blocked_pc?transition.blocked_pc:gameplay.last_blocked_dependency?gameplay.last_blocked_dependency:video.failed_address);
}

void jo_main(void)
{
    jo_core_init(JO_COLOR_Black);
#if defined(WF_JO_FRONTEND) && (defined(JO_COMPILE_WITH_PROFILER_SUPPORT) || defined(WF_JO_CAPTURE))
    *(volatile uint32_t *)WF_SATURN_LWRAM_DIAGNOSTIC=0;
    wf_jo_terminal_completed=0;
#endif
    wf_jo_diagnostic.stage=1;
    jo_clear_screen();
#ifdef __sh__
    if(__jo_smpc_timing_fault){
        wf_jo_diagnostic.fault_owner=0x534d5043u;
        wf_jo_diagnostic.fault_dependency=__jo_smpc_timing_fault;
        jo_printf(3,7,"WRESTLEFEST - R03");
        jo_printf(3,11,"CONFIGURACION SMPC NO ADMITIDA");
        jo_printf(3,14,"Codigo: %08X",__jo_smpc_timing_fault);
        jo_printf(3,18,"Fotografia esta pantalla.");
        jo_printf(3,21,"Reinicia la consola.");
        jo_core_run();return;
    }
#endif
    if(!cartridge_probe()){
        jo_printf(3,7,"WRESTLEFEST - R03");
        jo_printf(3,12,"REQUIERE CARTUCHO RAM DE 4 MB");
        jo_printf(3,16,"Apaga la consola e insertalo.");
        jo_core_run();return;
    }
#ifdef WF_JO_FRONTEND
    loading_text_visible=1;
    wf_jo_assets_set_loading_observer(loading_progress);
    jo_printf(3,10,"CARGANDO WRESTLEFEST...");
    jo_printf(3,23,"R03  JO ENGINE 4MB  T-1245G");
    load_requested=1;
#ifdef WF_JO_AUTORUN
    selected_demo=1;
#endif
#else
    jo_printf(3,4,"WRESTLEFEST - R03");
    jo_printf(3,7,"SATURN NTSC + RAM 4 MB");
    jo_printf(3,10,"COMBATE TAG - PRUEBA NATIVA");
    jo_printf(3,12,"IZQUIERDA / DERECHA: modo");
    jo_printf(3,15,"A / START: jugar con Hogan");
    jo_printf(3,18,"B: demo con control manual");
    jo_printf(3,21,"A golpe  B patada  D-PAD mover");
    jo_printf(3,25,"Jo Engine v2026.3");
#endif
    jo_core_add_callback(game_frame);
    jo_core_run();
}
