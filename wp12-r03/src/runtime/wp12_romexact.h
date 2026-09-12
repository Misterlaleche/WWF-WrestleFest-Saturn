#ifndef WP12_ROMEXACT_H
#define WP12_ROMEXACT_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    WP12_WRESTLER_RECORDS = 9,
    WP12_MAX_OPS = 96
};

typedef enum {
    WP12_MODE_TAG = 0,
    WP12_MODE_ROYAL = 1
} wp12_mode_t;

typedef enum {
    WP12_PHASE_INACTIVE = 0,
    WP12_PHASE_NEED_COMMON_SETUP,
    WP12_PHASE_VISIBLE,
    WP12_PHASE_HANDOFF,
    WP12_PHASE_BLOCKED
} wp12_phase_t;

typedef enum {
    WP12_OK = 0,
    WP12_NEED_COMMON_SETUP,
    WP12_NEED_RNG_21B4,
    WP12_BLOCKED_BAD_SETUP,
    WP12_BLOCKED_BAD_IDENTITY,
    WP12_BLOCKED_BAD_FINAL_AUDIO_INDEX,
    WP12_BLOCKED_OP_OVERFLOW,
    WP12_ALREADY_HANDOFF
} wp12_status_t;

typedef enum {
    WP12_OP_NONE = 0,
    WP12_OP_AUDIO_RAW,
    WP12_OP_CALL_AE20,
    WP12_OP_CALL_264E2,
    WP12_OP_CALL_7B70,
    WP12_OP_CALL_C98,
    WP12_OP_CALL_98BA,
    WP12_OP_RESOURCE_PREP_2AEA,
    WP12_OP_RENDER_PRESENTER,
    WP12_OP_RENDER_WRESTLER,
    WP12_OP_RENDER_REFEREE,
    WP12_OP_VIDEO_SYNC_2836,
    WP12_OP_RESOURCE_TEARDOWN_2B58,
    WP12_OP_CALL_9A42,
    WP12_OP_CALL_7506,
    WP12_OP_CALL_2A06,
    WP12_OP_CALL_8AE6,
    WP12_OP_HANDOFF_F8C
} wp12_op_kind_t;

typedef struct {
    wp12_op_kind_t kind;
    /* For WP12_OP_RENDER_* this is $247C: the operation represents the
       canonical $247C -> $27B8 render pair, not $27B8 in isolation. */
    uint32_t rom_address;
    uint32_t arg;
    uint8_t actor_index;
} wp12_op_t;

typedef struct {
    wp12_op_t ops[WP12_MAX_OPS];
    uint16_t count;
} wp12_batch_t;

typedef struct {
    bool active;                  /* +$00 bit7 */
    uint16_t type_plus02;
    uint16_t draw_plus04;
    uint16_t x_plus06;
    uint16_t y_plus0a;
    uint16_t z_plus0e;
    uint16_t local_word_plus1c;   /* high-byte init latch + low-byte local state */
    uint16_t primary_plus20;
    uint16_t counter_plus22;
    uint16_t phase_plus24;
    uint16_t orientation_plus2e;
    uint16_t plus32;
    uint8_t plus33;
    uint16_t plus34;
    uint16_t identity_word_plus56;/* low byte == physical +$57 */
    uint16_t plusb6;
    uint16_t plus104;
} wp12_wrestler_t;

typedef struct {
    uint16_t flags_plus00;
    bool active;
    uint16_t type_plus02;
    uint16_t draw_plus04;
    uint16_t x_plus06;
    uint16_t y_half_units;        /* 2 * machine Y, preserves state15 half-step */
    uint16_t z_plus0e;
    bool init_latch;
    uint8_t state_plus1d;
    uint16_t aux_plus1e;
    uint16_t counter_plus22;
    uint16_t phase_plus24;
    uint8_t toggle_plus25;
} wp12_presenter_t;

typedef struct {
    uint16_t flags_plus00;
    bool active;
    uint16_t type_plus02;
    uint16_t draw_plus04;
    uint16_t x_plus06;
    uint16_t y_plus0a;
    uint16_t z_plus0e;
    uint16_t primary_plus20;
    uint16_t plus34;
} wp12_referee_t;

typedef struct {
    bool action_mask_0030_any_port;
    bool rng_21b4_valid;
    uint8_t rng_21b4_low_nibble;
} wp12_iteration_input_t;

typedef struct {
    wp12_mode_t mode;
    wp12_phase_t phase;

    /* Relevant globals preserved as machine values. */
    uint16_t global_1c0160;
    uint8_t global_1c0162;
    uint8_t global_1c0163;
    uint8_t global_1c0066;
    uint16_t global_1c0166;
    uint16_t global_1c0168;
    uint16_t global_1c15fc;
    uint8_t global_1c1697;

    wp12_wrestler_t wrestlers[WP12_WRESTLER_RECORDS];
    wp12_presenter_t presenter;
    wp12_referee_t referee;

    bool visible_entry_done;
    uint32_t visible_updates;
    wp12_status_t blocked_reason;
} wp12_ctx_t;

void wp12_batch_clear(wp12_batch_t *batch);
uint8_t wp12_identity57(const wp12_wrestler_t *actor);
uint16_t wp12_tag_identity_window_threshold(uint8_t identity57);
uint16_t wp12_tag_identity_voice_command(uint8_t identity57);
uint16_t wp12_tag_activation_command(uint8_t identity57);
uint16_t wp12_final_audio_command(uint8_t global_1c0162_low);

/* Paid-game route $BD2 through the call to $C98.  The exact common C98
 * setup remains an external master-owned call; caller must execute the batch
 * and then call wp12_confirm_common_setup(). */
wp12_status_t wp12_begin_paid_route(wp12_ctx_t *ctx, wp12_batch_t *batch);

/* Validates the ROM-proven WP12-facing fields that must exist after $C98.
 * It does not fabricate unknown C98 state. */
wp12_status_t wp12_confirm_common_setup(wp12_ctx_t *ctx,
                                         bool c98_equivalent_executed,
                                         bool referee_resource_bundle_prepared);

/* Executes one synchronized $A654 display-loop iteration.  All effects that
 * occur in the same 68000 control-flow slice are returned as one ordered batch;
 * no artificial VBlank is inserted between batch entries. */
wp12_status_t wp12_visible_iteration(wp12_ctx_t *ctx,
                                     const wp12_iteration_input_t *input,
                                     wp12_batch_t *batch);

/* Helpers used by renderer/integration tests. */
uint16_t wp12_presenter_y_machine_floor(const wp12_ctx_t *ctx);
int wp12_resolve_tag_actor_index(const wp12_ctx_t *ctx, uint8_t controller_state);

#ifdef __cplusplus
}
#endif
#endif
