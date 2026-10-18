#ifndef WF_R17_POST7A00_H
#define WF_R17_POST7A00_H
#include "wf_runtime.h"
#include "wf_r17_contract.h"

typedef wf_status_t (*wf_r17_post7_audio_fn)(wf_runtime_t *rt,uint16_t raw_command,void *user);
typedef struct wf_r17_fg0_write { uint32_t addr; uint32_t value; uint8_t width; } wf_r17_fg0_write_t;
typedef wf_status_t (*wf_r17_fg0_batch_fn)(wf_runtime_t *rt,const wf_r17_fg0_write_t *writes,size_t count,void *user);
typedef wf_status_t (*wf_r17_raw_word_fn)(wf_runtime_t *rt,uint16_t raw_value,void *user);
typedef wf_status_t (*wf_r17_raw_9aca_fn)(wf_runtime_t *rt,uint16_t d2,uint16_t d7,uint32_t dest,void *user);
typedef wf_status_t (*wf_r17_fg0_peek_long_fn)(wf_runtime_t *rt,uint32_t addr,uint32_t *value,void *user);
/* Branch-B $285DA dynamic visual object callback reached through $288F0.
 * The callback owns the ROM-selected visual handler. D2-D7 clobbers remain
 * caller-visible; D0/D1 are restored by $28896 itself. */
typedef wf_status_t (*wf_r17_branchb_288f0_fn)(wf_runtime_t *rt,uint8_t record[4],uint8_t table_bank,void *user);
/* Branch-B $26B68 graphics join ($26C1C/$26C6E/$26C8A). Gameplay owns
 * selection/movement state; WP26 consumes the render/update operation. */
typedef wf_status_t (*wf_r17_branchb_26b68_fn)(wf_runtime_t *rt,uint8_t side,uint16_t direction,void *user);
/* WP26 transactional join for ROM $2836 -> repeated $D1FC emission.
 * The hook may leave D6/D7 caller-visible exactly as the ROM render path does. */
typedef wf_status_t (*wf_r17_render_2836_batch_fn)(wf_runtime_t *rt,wf_actor_raw_t *const *actors,size_t count,void *user);
/* External closed-owner joins reached by ROM $18C4. Hooks must be transactional:
 * when they return non-WF_OK they must not leave externally committed state. */
typedef wf_status_t (*wf_r17_tag_6e34_fn)(wf_runtime_t *rt,void *user);
typedef wf_status_t (*wf_r17_royal_9132_fn)(wf_runtime_t *rt,wf_actor_raw_t *target,uint16_t paid_index,void *user);
struct wf_r17_participant_io;
typedef struct wf_r17_post7_ctx {
    wf_r17_post7_audio_fn audio_2052;
    wf_r17_fg0_batch_fn fg0_batch;
    /* Non-FG raw video-memory writes (e.g. arcade sprite RAM); renderer owner consumes them. */
    wf_r17_fg0_batch_fn video_batch;
    wf_r17_raw_word_fn resource_2aea;
    wf_r17_raw_word_fn resource_2b58;
    wf_r17_raw_word_fn raw_2503c;
    wf_r17_raw_9aca_fn raw_9aca;
    /* Exact $CB42 TST.L reader; required only on routes that inspect existing FG0. */
    wf_r17_fg0_peek_long_fn fg0_peek_long;
    wf_r17_branchb_288f0_fn branchb_288f0;
    wf_r17_branchb_26b68_fn branchb_26b68;
    wf_r17_render_2836_batch_fn render_2836_batch;
    wf_r17_tag_6e34_fn tag_6e34;
    wf_r17_royal_9132_fn royal_9132;
    struct wf_r17_participant_io *participants;
    wf_r17_render_ctx_t *render;
    void *user;
    uint32_t blocked_dependency_rom;
} wf_r17_post7_ctx_t;

/* ROM $285DA: Branch-B raw list/object manager; dynamic graphics handler remains WP26 join. */
wf_status_t wf_r17_owner_285da(wf_runtime_t *rt,wf_r17_post7_ctx_t *ctx);
/* ROM $26A42: Branch-B dual-side map/scroll state update + WP26 graphics join. */
wf_status_t wf_r17_owner_26a42(wf_runtime_t *rt,wf_r17_post7_ctx_t *ctx);

/* ROM $BCFE: Branch-B Royal status/video/FG0 owner. */
wf_status_t wf_r17_owner_bcfe(wf_runtime_t *rt,wf_r17_post7_ctx_t *ctx);
/* ROM $C04A: Branch-B warning/message state owner; raw FG0/video/9ACA joins remain external. */
wf_status_t wf_r17_owner_c04a(wf_runtime_t *rt,wf_r17_post7_ctx_t *ctx);

/* ROM $C710: Branch-B rotating message/status producer; WP18 owns FG0 pixels. */
wf_status_t wf_r17_owner_c710(wf_runtime_t *rt,wf_r17_post7_ctx_t *ctx);

/* ROM $7548: Branch-B visible stamina smoothing + raw WP18 FG0 producer. */
wf_status_t wf_r17_owner_7548(wf_runtime_t *rt,wf_r17_post7_ctx_t *ctx);

/* ROM $88A2: four-wrestler FG0/HUD raw producer. */
wf_status_t wf_r17_owner_88a2(wf_runtime_t *rt,wf_r17_post7_ctx_t *ctx);
/* ROM $8710: four compact visual records; external resource ownership remains WP25. */
wf_status_t wf_r17_owner_8710(wf_runtime_t *rt,wf_r17_post7_ctx_t *ctx);
/* ROM $BB14/$BC00: Royal replacement service. */
wf_status_t wf_r17_owner_bb14(wf_runtime_t *rt,wf_r17_post7_ctx_t *ctx);
/* ROM $7366: Royal structural actor/table owner. */
wf_status_t wf_r17_owner_7366(wf_runtime_t *rt,wf_r17_post7_ctx_t *ctx);
/* ROM $8CF8: shared compact owner / raw WP18 boundary. */
wf_status_t wf_r17_owner_8cf8(wf_runtime_t *rt,wf_r17_post7_ctx_t *ctx);

/* ROM $2836: final six-list sort/emit owner; WP26 consumes the D1FC batch. */
wf_status_t wf_r17_owner_2836(wf_runtime_t *rt,wf_r17_post7_ctx_t *ctx);

/* ROM $18C4: branch-A TAG/Royal payment/participant owner. */
wf_status_t wf_r17_owner_18c4(wf_runtime_t *rt,wf_r17_post7_ctx_t *ctx,const uint8_t input_hw_low[WF_ACTOR_SLOTS],uint16_t input_valid_mask);

/* ROM $298EC: final counted raw video transfer owner. */
wf_status_t wf_r17_owner_298ec(wf_runtime_t *rt,wf_r17_post7_ctx_t *ctx);

/* ROM $262D2: combat BCD timer + raw FG0 producer. Time-up remains fail-closed. */
wf_status_t wf_r17_owner_262d2(wf_runtime_t *rt,wf_r17_post7_ctx_t *ctx);

/* ROM $899C: raw FG0 producer; atomic batch boundary owned by WP18. */
wf_status_t wf_r17_owner_899c(wf_runtime_t *rt,wf_r17_post7_ctx_t *ctx);

/* ROM $9052: post-damage low-stamina warning / progression audio owner. */
wf_status_t wf_r17_owner_9052(wf_runtime_t *rt,wf_r17_post7_ctx_t *ctx);

/* ROM $8ECC: Royal natural stamina recovery owner. */
wf_status_t wf_r17_owner_8ecc(wf_runtime_t *rt);
wf_status_t wf_phase_owner_8ecc(wf_runtime_t *rt,uint32_t rom_addr,void *user);

#endif
