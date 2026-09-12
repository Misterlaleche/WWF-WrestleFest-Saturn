#ifndef WP12_FIRSTMATCH_PRESENTATION_H
#define WP12_FIRSTMATCH_PRESENTATION_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    WP12FM_IDENTITY_COUNT = 12,
    WP12FM_TAG_PAIR_COUNT = 2,
    WP12FM_ROYAL_PRESENTATION_SLOTS = 4,
    WP12FM_REVEAL_LEVELS = 8,
    WP12FM_VBLANKS_PER_MASK_LEVEL = 3,
    WP12FM_CLOSE_INTERNAL_WAIT = 33,
    WP12FM_WALK_HOLD_UPDATES = 12,
    WP12FM_WALK_CONTINUING_UPDATES = 320,
    WP12FM_WALK_TERMINAL_UPDATE = 321
};

typedef enum {
    WP12FM_MODE_TAG = 0,
    WP12FM_MODE_ROYAL = 1
} wp12fm_mode_t;

typedef enum {
    WP12FM_RUN_NONE = 0,
    WP12FM_RUN_FIRST_MATCH,
    WP12FM_RUN_CAMPAIGN_SPECIAL
} wp12fm_run_kind_t;

typedef enum {
    WP12FM_OK = 0,
    WP12FM_NEED_SHARED_RNG_21B4,
    WP12FM_INVALID_IDENTITY,
    WP12FM_INVALID_ARGUMENT
} wp12fm_status_t;

typedef struct {
    uint16_t world_mode;
    uint16_t scroll_seed_x;
    uint16_t scroll_seed_y;
    uint8_t priority_byte;
    uint16_t initial_audio_raw;
    uint16_t common_resource;
    bool credit_visible;
} wp12fm_scene_setup_t;

typedef struct {
    uint8_t identity;
    uint16_t resource_a;
    uint16_t resource_b;
    uint16_t name_group;
    uint16_t entry_set;
    uint16_t world_x;
    uint16_t world_y;
    uint16_t world_z;
    uint16_t speed_word;
    uint16_t angle_word;
    uint16_t facing_word;
} wp12fm_actor_setup_t;

typedef struct {
    uint16_t counter_plus22;
    uint16_t phase_plus24;
    uint8_t selector_plus05;
} wp12fm_walk_state_t;

/* $7B70 runs only when $1C008C == 0 and progression byte $1C0163 is 0/4/9. */
wp12fm_run_kind_t wp12fm_run_kind(uint16_t global_1c008c, uint8_t global_1c0163);

/* $1C0161.bit0: 0 TAG, 1 ROYAL. */
wp12fm_mode_t wp12fm_mode_from_global_1c0161(uint8_t global_1c0161);

/* Exact first-match scene setup written at $7B9A..$7C16. */
wp12fm_status_t wp12fm_scene_setup(uint8_t global_1c0163, wp12fm_scene_setup_t *out);

/* TAG first match presents record pairs 0/1 then 2/3. */
bool wp12fm_tag_pair_indices(uint8_t pair_index, uint8_t *first, uint8_t *second);

/* ROYAL first-match branch $7E16 iterates selection records 0..3 singly. */
bool wp12fm_royal_record_index(uint8_t sequence_index, uint8_t *record_index);

/* SELECT PLAYERS handoff table $64CC: selector-slot 0..9 -> runtime identity.
 * This closes human-selector identity mapping only; CPU roster fill remains external. */
wp12fm_status_t wp12fm_selector_slot_to_runtime_identity(uint8_t selector_slot, uint8_t *runtime_identity);

/* TAG $7C28..$7C68 / $7CF6..$7D38 lane normalization swaps the full
 * +$02 type/identity WORD: ID0 is moved out of first lane; ID9 is moved
 * out of second lane. Apply in this exact order. */
wp12fm_status_t wp12fm_tag_normalize_pair_type_words(uint16_t *first_type_word, uint16_t *second_type_word);

/* $7F4C/$7F9A text producer joins.  Zero is a literal ROM table value, not
 * automatically interpreted as "no text" by this API. */
uint16_t wp12fm_tag_name_group(uint8_t identity, bool second_lane);
uint16_t wp12fm_royal_name_group(uint8_t identity);

/* $813E: two resource IDs prepared through $2AEA. */
wp12fm_status_t wp12fm_resource_pair(uint8_t identity, uint16_t *a, uint16_t *b);

/* Initial actor fields written by $7FC6..$8068. */
wp12fm_status_t wp12fm_actor_setup(wp12fm_mode_t mode,
                                   uint8_t identity,
                                   bool tag_second_lane,
                                   wp12fm_actor_setup_t *out);

/* IDs whose entry set byte becomes $40/$41/$47/$49 can select alternate
 * selectors 4..7 after each four-pose cycle.  The decision consumes $21B4.bit0. */
bool wp12fm_identity_uses_cycle_rng(uint8_t identity);

/* One $7F0E nonterminal animation update.  The caller must provide the shared
 * RNG bit only when the function returns NEED_SHARED_RNG_21B4. */
wp12fm_status_t wp12fm_walk_update(uint8_t identity,
                                   bool rng_bit0_valid,
                                   uint8_t rng_bit0,
                                   wp12fm_walk_state_t *state);

/* $2208 direction $80 / speed $08: Y delta -$00008000 in signed 16.16. */
int32_t wp12fm_world_y_after_updates(uint32_t updates);
bool wp12fm_terminal_after_updates(uint32_t updates);

/* $7F0E visual-offset producer.  +$19 remains zero from the $1F38 actor reset.
 * +$1B is normally zero, but entry set $41 (runtime identity 1) uses +$30
 * while the selected walk pose is 4..7. */
int8_t wp12fm_x_offset_plus19(uint8_t identity, uint8_t selector_plus05);
int8_t wp12fm_y_offset_plus1b(uint8_t identity, uint8_t selector_plus05);

/* Exact $247C world->screen transform including signed low-byte offsets and
 * the facing-controlled sign of X offset.  Selection clears +$2E, but facing
 * remains an explicit argument so the formula is not weakened. */
int16_t wp12fm_screen_x(uint16_t world_x, uint16_t scroll_x, int8_t x_offset, uint16_t facing_plus2e);
int16_t wp12fm_screen_y(uint16_t world_y, uint16_t world_z, uint16_t scroll_y, int8_t y_offset);

/* TAG overlay/reveal/close occurs only on the first actor of each pair.
 * ROYAL does it for each presented actor. */
bool wp12fm_overlay_owner(wp12fm_mode_t mode, bool tag_second_lane);

/* Exact mask timing. */
uint16_t wp12fm_reveal_vblanks(void);   /* $8FD6 = 8 * 3 */
uint16_t wp12fm_close_vblanks(void);    /* $8F50 mask body = 8 * 3 */
uint16_t wp12fm_close_tail_waits(void); /* internal $21E6(D0=$20) = 33 */

#ifdef __cplusplus
}
#endif
#endif
