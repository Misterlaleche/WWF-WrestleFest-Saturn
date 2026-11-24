#ifndef WF_R18_FRONTEND_DRIVER_H
#define WF_R18_FRONTEND_DRIVER_H
#include "wf_r18_frontend_reset.h"
#include "wf_r18_frontend_boot.h"
#include "wf_r18_frontend_coin.h"
#include "wf_r18_frontend_mode_plate.h"
#include "wf_r18_mode_select.h"
#include "wf_r18_roster_select.h"
#include "wf_r18_frontend_effect.h"
#include "wf_r18_frontend_leaf.h"
#include "wf_r18_frontend_board.h"
#include "wf_r18_frontend_map.h"
#include "wf_r18_frontend_io.h"

typedef struct wf_r18_frontend_driver wf_r18_frontend_driver_t;
typedef struct wf_r18_frontend_platform {
 wf_r18_read_rom_fn read_rom;
 wf_r18_frontend_read_bus_fn read_bus;
 wf_r18_frontend_store_bus_fn store_bus;
 /* Synchronous waits: return WF_OK only after the requested real level/
  * event. They may deliver actual events via driver_on_vblank/on_raster.
  * Any non-OK is fatal; a pending wait must remain inside the callback.
  * wait_level sees the original $140026.bit2 (WORD bit10). No guessed
  * delay or coin frequency is supplied by the portable driver. */
 wf_status_t (*wait_level)(void *,wf_r18_frontend_driver_t *,unsigned high);
 wf_status_t (*wait_vblank)(void *,wf_r18_frontend_driver_t *);
 /* Called after a completed VBlank IRQ; commits current video/MMIO state. */
 wf_status_t (*commit)(void *,wf_r18_frontend_driver_t *);
 wf_status_t (*bind_world)(void *,unsigned mode);
 /* Borrowed authoritative C2000 buffer, exactly count records. */
 wf_status_t (*submit_records)(void *,const wf_r18_sprite_record_t *,size_t count);
 void *user;
 /* After original RESET$6F6 installs factory ranking tables, before BOOT. */
 wf_status_t (*reset_ready)(void *,uint8_t *ram,size_t ram_bytes);
 /* Hide the physical scan before destructive video clears; release only
  * after the original foreground clear has completed. Optional on HOST. */
 wf_status_t (*scene_clear)(void *,uint32_t owner,unsigned complete);
} wf_r18_frontend_platform_t;
typedef struct wf_r18_frontend_driver_config {
 uint8_t *ram;size_t ram_bytes; /* Borrowed16KB; initial lower$60/RNG supplied. */
 wf_runtime_t *runtime;wf_r18_video_t *video; /* Borrowed, never game-reset. */
 wf_r18_frontend_platform_t platform;
 uint32_t ssp,usp;uint16_t sr; /* Explicit cold-entry supervisor state. */
} wf_r18_frontend_driver_config_t;
enum {WF_R18_DRIVER_RESET=0,WF_R18_DRIVER_BOOT=1,WF_R18_DRIVER_TITLE=2,
 WF_R18_DRIVER_MODE_PLATE=3,WF_R18_DRIVER_MODE=4,WF_R18_DRIVER_ROSTER=5,
 WF_R18_DRIVER_AC0=6,WF_R18_DRIVER_FATAL=7};
enum {WF_R18_DRIVER_MORE=0,WF_R18_DRIVER_WAIT=1,WF_R18_DRIVER_HANDOFF=2,
 WF_R18_DRIVER_BLOCKED=-1};
struct wf_r18_frontend_driver {
 uint8_t *ram;size_t ram_bytes;
 wf_runtime_t *runtime;wf_r18_video_t *video;
 wf_r18_frontend_platform_t platform;
 wf_r17_m68k_shadow_t regs;uint32_t a[7];
 wf_r18_frontend_control_context_t control;
 union {
  wf_r18_frontend_reset_t reset;wf_r18_frontend_boot_t boot;
  wf_r18_title_t title;wf_r18_frontend_mode_plate_t plate;
  wf_r18_mode_select_t mode;wf_r18_roster_select_t roster;
 } scene;
 wf_r18_frontend_irq_t irq;wf_r18_frontend_coin_t coin;
 wf_r18_frontend_effect_t fade;
 uint32_t pc,blocked_dependency,blocked_address,generation,active_owner;
 uint8_t phase,credited_start,irq_active,coin_active,call_depth,world_visible;
};

/* init changes driver metadata only; RESET654 performs the original RAM
 * clear. No RNG seed, credit, selection, frame or IRQ is synthesized.
 * All callbacks mandatory. VRAM/palette accesses are routed internally to
 * wf_r18_video; platform read/store_bus own remaining board/MMIO addresses.
 * video->fg0+C2000offset is the sole sprite-record shadow (capacity512).
 */
wf_status_t wf_r18_frontend_driver_init(wf_r18_frontend_driver_t *,const wf_r18_frontend_driver_config_t *);
int wf_r18_frontend_driver_step(wf_r18_frontend_driver_t *);
/* Runs original quanta and synchronous waits until AC0 or fatal dependency. */
wf_status_t wf_r18_frontend_driver_run(wf_r18_frontend_driver_t *);
/* Resume the original gameover JMP6FC, exclusively at a consumed boundary.
 * Metadata-only: preserves raw/D/A/control/runtime/video/platform state,
 * invalidates old cursors by advancing generation and initializes BOOT6FC.
 * No RESET654, factory records, coinage or seed effects. Caller must abandon
 * its previous post/match cursor. No active owner or IRQ may be interrupted. */
wf_status_t wf_r18_frontend_driver_resume_boot(wf_r18_frontend_driver_t *,uint32_t entry_pc);
/* Explicit admitted IRQ3/IRQ2 events; backend owns hardware scheduling and
 * interrupt delivery/priority. These APIs supply actual exception state to
 * original control owners. Coin IRQ rate must be supplied by backend.
 * Credited START abandons the old scene/callee generation and enters52BE;
 * callbacks must return promptly after observing generation change.
 */
wf_status_t wf_r18_frontend_driver_on_vblank(wf_r18_frontend_driver_t *);
wf_status_t wf_r18_frontend_driver_on_raster(wf_r18_frontend_driver_t *);
/* Accepted START continuation at the original $8AE seam. The caller has
 * completed the IRQ video owners and $978 with carry set; it supplies the
 * live raw/D/A state and supervisor SR. Reuses the original stack/SR reset,
 * scene clears and HANDOFF52BE without repeating the IRQ video owners. */
wf_status_t wf_r18_frontend_driver_start_8ae(wf_r18_frontend_driver_t *);
/* Reusable exact synchronous owner dispatcher for C98/A654/paidflow.
 * Unknown/missing dependency is fatal WF_BLOCKED, never silently omitted.
 * AC0 preserves rawRAM/D/A/control state; caller imports it explicitly.
 */
wf_status_t wf_r18_frontend_driver_call(wf_r18_frontend_driver_t *,uint32_t owner);
/* ROM $8490 special attract/title scene, called by attract setup at $B7E. */
wf_status_t wf_r18_frontend_driver_attract_visual_8490(wf_r18_frontend_driver_t *);
#endif
