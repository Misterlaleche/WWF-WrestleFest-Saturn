#ifndef WF_JO_FRONTEND_H
#define WF_JO_FRONTEND_H
#include "wf_jo_clock.h"
#include "wf_r18_display.h"
#include "wf_r18_match_driver.h"
#include "wf_r18_world.h"

#define WF_JO_FRONTEND_RAM_BYTES 0x4000u
typedef struct wf_jo_frontend wf_jo_frontend_t;
extern volatile uint32_t wf_jo_front_diagnostic[8];

typedef struct wf_jo_frontend_config {
    wf_runtime_t *runtime;
    wf_r18_video_t *video;
    wf_r18_world_ctx_t *world;
    wf_r18_display_t *display;
    wf_jo_clock_t *clock;
    uint8_t *ram;
    size_t ram_bytes;
    void *scene_blob;
    size_t scene_capacity;
    uint8_t *priority_140010;
    const uint16_t *render_counts; /* Six original render-list counts. */
    wf_r18_ram_read_fn combat_read_ram;
    void *combat_read_user;
    /* Optional ordinary cabinet-input fixture. Physical pad/coin/DIP levels
     * are supplied first; the callback must not write game state. */
    uint16_t (*input_word)(void *,const wf_jo_frontend_t *,unsigned,uint16_t);
    /* Optional backup restore, called after the original factory RESET. */
    wf_status_t (*reset_ready)(void *,uint8_t *,size_t);
    /* Optional persistence notification after the full native ranking owner
     * has returned successfully. Never called during initials input. */
    wf_status_t (*records_ready)(void *,const uint8_t *,size_t);
    void *user;
} wf_jo_frontend_config_t;

/* Borrowed for the entire cabinet session, including combat. In particular,
 * ram is a distinct persistent 16 KiB allocation, never a sprite-cache lease.
 * Existing runtime/video/world/presenter storage is shared without copies.
 * The adapter allocates no memory and never opens a disc file. */
struct wf_jo_frontend {
    wf_r18_frontend_driver_t driver;
    wf_r18_match_driver_t match;
    wf_jo_frontend_config_t config;
    uint8_t scroll_ports[0x20],control_ports[0x20];
    uint16_t back_color;
    uint16_t bound_world;
    uint32_t arcade_ticks,raster_irqs,vblank_irqs,frame_commits;
    wf_jo_clock_t combat_coin_clock;
    uint32_t combat_coin_irqs;
    /* Admitted combat boundaries left at accepted START after its display
     * drain. The accepted boundary is excluded; handoff keeps original reset. */
    uint32_t combat_start_debt;
    uint32_t blocked_pc,blocked_dependency;
    size_t pending_records;
    uint8_t pending,high,scene_gate,handed_off;
};

/* Before init, preload ROM/sprites/all scene packs and bind FONT8/HUD, then
 * initialize/prefetch audio. Prime cue 02 as the last loading operation.
 * Jo video and the native presenter must already be initialized. Only the
 * original lower-$60 cold cabinet bytes and the explicit RNG seed are set;
 * RESET654 owns the rest of the initial arcade RAM image. */
wf_status_t wf_jo_frontend_init(wf_jo_frontend_t *,
                               const wf_jo_frontend_config_t *,uint32_t seed);
/* Synchronously run intro/selection or the resumed scene through original
 * F8C. Jo owns every physical sync; each rational arcade tick delivers the
 * admitted 17 raster IRQs and one VBlank IRQ. Portrait/speech divisors are1.
 * No additional transition delays are inserted. */
wf_status_t wf_jo_frontend_run(wf_jo_frontend_t *);
/* Publish the latest staged scene once in the current Jo frame, without
 * advancing time. Call before returning the application callback after run
 * or after_combat, and begin combat on a subsequent callback. WF_BLOCKED
 * with no driver fault means this Jo tag was already published: retry in
 * the next callback. Internal synchronous waits handle that case themselves. */
wf_status_t wf_jo_frontend_flush(wf_jo_frontend_t *);
/* Merge drained live combat state into the retained raw image and continue
 * the original owner. 11B6 includes TAG's result; 122C starts after an already
 * completed result; 1AFC includes Royal's GAME OVER result and1B1E follows
 * an already completed Royal result; 1120 ends attract;
 * 1B4E runs the original interactive credits after a final victory.
 * No RESET/bootstrap or new seed is performed. Call run() for the next match.
 * Unsupported boundaries remain explicit WF_BLOCKED diagnostics.
 * The compatibility wrapper below enters with virtual VBlank LOW. */
wf_status_t wf_jo_frontend_after_combat(wf_jo_frontend_t *,uint32_t owner);
/* Explicit arcade level at the handoff. A just-admitted IRQ3 ACK has HIGH;
 * exporting RAM does not change that virtual level. $2A06 tests this level
 * and waits only when needed. The physical SH-2 time spent exporting cannot
 * determine the original arcade level. Values other than0/1 are rejected. */
wf_status_t wf_jo_frontend_after_combat_at_level(wf_jo_frontend_t *,uint32_t owner,unsigned high);
/* Call once per application callback while handed_off, independently of
 * whether a combat tick is due. The original IRQ2 coin owners run17 times
 * per elapsed rational arcade period. Physical pads are sampled once per
 * application callback; every original input request and fixture hook still
 * runs against those levels. No native IRQ2 or pulse counter is skipped.
 * Only the typed credit bytes are synchronized with the retained raw image;
 * gameplay registers/actors and the combat clock are untouched. */
wf_status_t wf_jo_frontend_combat_coins(wf_jo_frontend_t *);
/* Original IRQ seam $89E: call after combat's video owners and before ACK.
 * Paid-match START/join remains with gameplay $18C4/$6E34. Attract START
 * executes $978 and accepted $8AE..$52BE, consumes original credits and
 * returns accepted=1. Stop combat and call run() then; do not call
 * after_combat(), which would overwrite the original clear/selection state.
 * Full scene export occurs only for a physical START candidate able to pay.
 * With accepted=0, the live runtime and borrowed video callbacks stay active.
 * On acceptance combat_start_debt records the previous combat debt, including
 * elapsed drain fields, before this function reanchors the scene clock. */
wf_status_t wf_jo_frontend_combat_start(wf_jo_frontend_t *,unsigned *accepted);
#endif
