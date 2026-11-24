#ifndef WF_JO_VIDEO_H
#define WF_JO_VIDEO_H
#include <stddef.h>
#include <stdint.h>
#include "wf_vdp1_plan.h"
#include "wf_jo_texture_cache.h"

/* Jo owns command tables below 0x10000 and Gouraud tables from 0x7e4f8.
 * Four frame banks retain VDP2 snapshots; immutable VDP1 textures are shared. */
#define WF_JO_TEXTURE_BANKS 4u
#define WF_JO_TEXTURE_FIRST 0x10000u
/* Retained for the portable bank-address helper; no texture bank is copied. */
#define WF_JO_TEXTURE_BANK_BYTES 0x8000u
#define WF_JO_TEXTURE_BYTES (WF_JO_GPU_TILES*128u)
#define WF_JO_TEXTURE_STAGE_BYTES 2048u
#define WF_JO_MAP_GROUP_BYTES 0x6000u
#define WF_JO_VISIBLE_Y 8

typedef struct wf_jo_crop {
    uint8_t top, height, left, width;
} wf_jo_crop_t;

typedef struct __attribute__((aligned(4))) wf_jo_command {
    uint16_t ctrl, link, pmod, colr, srca, size;
    int16_t xa, ya, xb, yb, xc, yc, xd, yd;
    uint16_t grda, reserved;
} wf_jo_command_t;

int wf_jo_bank_reusable(unsigned used, uint32_t last_tag,
                        unsigned shown_valid, uint32_t shown_tag);
wf_status_t wf_jo_texture_offset(unsigned bank, unsigned slot, uint32_t *out);
void wf_jo_crop_tile(uint8_t pixels[128], wf_jo_crop_t *crop);
wf_status_t wf_jo_sprite_build(const wf_vdp1_sprite_cmd_t *source,
                               const wf_jo_crop_t *crop, uint32_t texture_offset,
                               wf_jo_command_t *out);

wf_status_t wf_jo_video_init(void);
wf_status_t wf_jo_video_acquire_bank(unsigned *bank);
void wf_jo_video_release_bank(unsigned bank);
/* Exclusive CURRENT Jo CPU command prefix. Jo waits for this buffer's old
 * sort/DMA before returning it to the next callback, with one or two buffers.
 * In audited dual mode the other buffer can still be read by slave/DMA: a
 * repeat copies its immutable normal-sprite prefix to the current buffer.
 * No client may retain a writable borrow after submit/abort, nor synchronize
 * Jo between begin and submit. Opening invalidates replay before writes; an
 * abort blocks replay until a complete new submission. No other Jo drawing
 * API may append while the video adapter owns display. */
wf_jo_command_t *wf_jo_video_commands_begin(unsigned count);
void wf_jo_video_commands_abort(void);
wf_status_t wf_jo_video_submit(unsigned bank, const wf_jo_command_t *commands,
                               unsigned count);
typedef struct wf_jo_texture_ref {
    uint16_t physical;
    uint8_t meta,height;
} wf_jo_texture_ref_t;
/* Protect the complete logical frame before resolving any new source. Source
 * invalidation preserves tags and the last complete frame's repeat references. */
wf_status_t wf_jo_video_texture_invalidate(void);
wf_status_t wf_jo_video_texture_begin(unsigned count);
void wf_jo_video_texture_abort(void);
/* Private ROM preparation: a token belongs to one complete immutable submit.
 * The caller proves its logical source array has not changed. Every cache
 * mutation revokes the candidate; restoring still opens a fresh tagged frame. */
int wf_jo_video_texture_preparation_available(const void *owner,uint32_t generation,unsigned count);
int wf_jo_video_texture_restore_preparation(const void *owner,uint32_t generation,const uint8_t *active);
int wf_jo_video_texture_consume_preparation(const void *owner,uint32_t generation,unsigned count);
void wf_jo_video_texture_remember_preparation(const void *owner,uint32_t generation,const uint8_t *active);
wf_status_t wf_jo_video_texture_protect(unsigned logical,uint16_t source,
                                       wf_jo_texture_ref_t *ref,unsigned *hit);
wf_status_t wf_jo_video_texture_resolve(unsigned logical,uint16_t source,unsigned immutable,
                                       wf_jo_texture_ref_t *ref,unsigned *hit);
wf_status_t wf_jo_video_texture_transfer(unsigned physical,const void *pixels,unsigned tiles);
wf_status_t wf_jo_video_texture_ready(unsigned logical,uint16_t source,unsigned immutable,
                                     uint8_t meta,uint8_t height);
const uint16_t *wf_jo_video_texture_frame_slots(void);
#ifndef WF_PLATFORM_SATURN
const uint8_t *wf_jo_video_texture_pixels(unsigned offset,unsigned bytes);
#endif
wf_status_t wf_jo_video_commit(void);
wf_status_t wf_jo_video_repeat(void);
/* One-shot callback-loop permission after a fresh complete command commit.
 * The caller must already have admitted combat debt. Repeats cannot arm it.
 * Explicit SDK waits always cancel the permission. An active epoch must be
 * drained before a gate, character rebind or departure from combat. */
wf_status_t wf_jo_video_allow_overlap(void);
/* Finish the last committed image and its full visible field. If necessary,
 * submit one immutable repeat successor with a new Jo tag, never a tick/ACK.
 * The callback may service audio/coin only; it may not enter this adapter,
 * rebind assets or advance game logic. Failure preserves all display fences;
 * retry is allowed and never creates an unbounded chain of successors. */
wf_status_t wf_jo_video_drain_overlap(wf_status_t (*service)(void *),void *user);
/* Same closure, reporting only its own elapsed fields (zero without an active
 * epoch). START uses this to preserve its original transport-time exclusion. */
wf_status_t wf_jo_video_drain_overlap_fields(wf_status_t (*service)(void *),void *user,
                                            uint32_t *elapsed_fields);
int wf_jo_video_frame_queued(void);
/* Canonical palette/map/scroll/layer state differs from the last complete
 * published snapshot. A frontend must preserve it before another IRQ frame. */
int wf_jo_video_pending_changes(void);
void wf_jo_video_blank(void);
/* Hide layers/sprites immediately while leaving the display clock running.
 * Only a subsequently published complete frame can release this gate. */
void wf_jo_video_gate(void);
void wf_jo_video_diagnostics(void);
volatile uint16_t *wf_jo_video_map(unsigned plane);
void wf_jo_video_map_written(unsigned plane);
volatile uint8_t *wf_jo_video_characters(unsigned plane);
void wf_jo_video_scroll(unsigned plane, unsigned x, unsigned y);
void wf_jo_video_ring_priority(uint8_t priority, unsigned show);
/* Bits0..2 select FG/BG/HUD; opaque uses the same bits. The sprite priority
 * is explicit so legal text can expose only the opaque HUD plane. */
void wf_jo_video_layers(unsigned layers,unsigned opaque,unsigned sprites);
void wf_jo_video_cram_write(unsigned entry, uint32_t rgb888);

typedef struct wf_jo_video_stats {
    uint32_t submitted_frames, repeated_frames, displayed_frames;
    uint32_t texture_bytes, texture_transfers, last_texture_bytes;
    uint32_t map_bytes, palette_bytes, bank_waits, bank_wait_fields;
    /* Fields without a new displayed frame while a submitted snapshot waits. */
    uint32_t pipeline_stalled_fields;
    uint32_t overlap_requests, overlap_drains, overlap_drain_fields;
    uint32_t overlap_last_target, overlap_last_shown_field, overlap_last_drain_field;
} wf_jo_video_stats_t;
extern volatile wf_jo_video_stats_t wf_jo_video_stats;

#endif
