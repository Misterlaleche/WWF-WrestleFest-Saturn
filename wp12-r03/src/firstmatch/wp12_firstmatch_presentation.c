#include "wp12_firstmatch_presentation.h"

static const uint16_t k_resource_pairs[WP12FM_IDENTITY_COUNT][2] = {
    {0x0000u,0x0000u}, {0x0001u,0x0001u}, {0x0002u,0x0030u},
    {0x0003u,0x0003u}, {0x0004u,0x0030u}, {0x0004u,0x0030u},
    {0x0006u,0x0006u}, {0x0007u,0x0007u}, {0x0008u,0x0008u},
    {0x0009u,0x0009u}, {0x000Au,0x000Au}, {0x000Au,0x000Au}
};

/* $8186 is two WORDs per runtime identity: first-lane and second-lane text. */
static const uint16_t k_tag_name_groups[WP12FM_IDENTITY_COUNT][2] = {
    {0x002Bu,0x002Cu}, {0x002Du,0x002Eu}, {0x002Fu,0x0030u},
    {0x0031u,0x0032u}, {0x0033u,0x0034u}, {0x0033u,0x0034u},
    {0x0035u,0x0036u}, {0x0037u,0x0038u}, {0x0039u,0x003Au},
    {0x003Bu,0x003Cu}, {0x003Du,0x003Du}, {0x003Du,0x003Du}
};

/* $816E, one WORD per runtime identity.  IDs 4/5 are literally zero in ROM;
 * reachability/semantic interpretation remains external/OPEN. */
static const uint16_t k_royal_name_groups[WP12FM_IDENTITY_COUNT] = {
    0x002Cu,0x002Eu,0x0030u,0x0032u,0x0000u,0x0000u,
    0x0036u,0x0038u,0x003Au,0x003Cu,0x003Eu,0x003Fu
};

static const uint8_t k_walk_selectors[8] = {0u,1u,2u,3u,4u,5u,6u,7u};

/* $64CC: selector-slot -> runtime identity. */
static const uint8_t k_selector_slot_to_runtime[10] = {
    0u,7u,2u,8u,6u,9u,1u,3u,10u,11u
};

wp12fm_run_kind_t wp12fm_run_kind(uint16_t global_1c008c, uint8_t global_1c0163)
{
    if (global_1c008c != 0u) return WP12FM_RUN_NONE;
    if (global_1c0163 == 0u) return WP12FM_RUN_FIRST_MATCH;
    if (global_1c0163 == 4u || global_1c0163 == 9u) return WP12FM_RUN_CAMPAIGN_SPECIAL;
    return WP12FM_RUN_NONE;
}

wp12fm_mode_t wp12fm_mode_from_global_1c0161(uint8_t global_1c0161)
{
    return (global_1c0161 & 1u) != 0u ? WP12FM_MODE_ROYAL : WP12FM_MODE_TAG;
}

wp12fm_status_t wp12fm_scene_setup(uint8_t global_1c0163, wp12fm_scene_setup_t *out)
{
    if (out == 0) return WP12FM_INVALID_ARGUMENT;
    out->world_mode = 4u;
    out->scroll_seed_x = 0x0140u;
    out->scroll_seed_y = 0x0200u;
    out->priority_byte = 0x7Bu;
    out->initial_audio_raw = (global_1c0163 == 4u || global_1c0163 == 9u) ? 0x310Du : 0x310Eu;
    out->common_resource = 0x0034u;
    out->credit_visible = false;
    return WP12FM_OK;
}

bool wp12fm_tag_pair_indices(uint8_t pair_index, uint8_t *first, uint8_t *second)
{
    if (first == 0 || second == 0 || pair_index >= WP12FM_TAG_PAIR_COUNT) return false;
    *first = (uint8_t)(pair_index * 2u);
    *second = (uint8_t)(*first + 1u);
    return true;
}

bool wp12fm_royal_record_index(uint8_t sequence_index, uint8_t *record_index)
{
    if (record_index == 0 || sequence_index >= WP12FM_ROYAL_PRESENTATION_SLOTS) return false;
    *record_index = sequence_index;
    return true;
}

wp12fm_status_t wp12fm_selector_slot_to_runtime_identity(uint8_t selector_slot, uint8_t *runtime_identity)
{
    if (runtime_identity == 0) return WP12FM_INVALID_ARGUMENT;
    if (selector_slot >= 10u) return WP12FM_INVALID_ARGUMENT;
    *runtime_identity = k_selector_slot_to_runtime[selector_slot];
    return WP12FM_OK;
}

wp12fm_status_t wp12fm_tag_normalize_pair_type_words(uint16_t *first_type_word, uint16_t *second_type_word)
{
    uint16_t t;
    if (first_type_word == 0 || second_type_word == 0) return WP12FM_INVALID_ARGUMENT;
    if ((uint8_t)(*first_type_word & 0x00FFu) == 0u) {
        t=*first_type_word; *first_type_word=*second_type_word; *second_type_word=t;
    }
    if ((uint8_t)(*second_type_word & 0x00FFu) == 9u) {
        t=*first_type_word; *first_type_word=*second_type_word; *second_type_word=t;
    }
    return WP12FM_OK;
}

uint16_t wp12fm_tag_name_group(uint8_t identity, bool second_lane)
{
    if (identity >= WP12FM_IDENTITY_COUNT) return 0u;
    return k_tag_name_groups[identity][second_lane ? 1u : 0u];
}

uint16_t wp12fm_royal_name_group(uint8_t identity)
{
    if (identity >= WP12FM_IDENTITY_COUNT) return 0u;
    return k_royal_name_groups[identity];
}

wp12fm_status_t wp12fm_resource_pair(uint8_t identity, uint16_t *a, uint16_t *b)
{
    if (a == 0 || b == 0) return WP12FM_INVALID_ARGUMENT;
    if (identity >= WP12FM_IDENTITY_COUNT) return WP12FM_INVALID_IDENTITY;
    *a = k_resource_pairs[identity][0];
    *b = k_resource_pairs[identity][1];
    return WP12FM_OK;
}

wp12fm_status_t wp12fm_actor_setup(wp12fm_mode_t mode,
                                   uint8_t identity,
                                   bool tag_second_lane,
                                   wp12fm_actor_setup_t *out)
{
    uint16_t a, b;
    uint16_t set;
    if (out == 0) return WP12FM_INVALID_ARGUMENT;
    if (identity >= WP12FM_IDENTITY_COUNT) return WP12FM_INVALID_IDENTITY;
    if (wp12fm_resource_pair(identity,&a,&b) != WP12FM_OK) return WP12FM_INVALID_IDENTITY;
    set = (uint16_t)(identity + 0x40u);
    if (set == 0x004Bu) set = 0x004Au; /* $8008..$8016 */
    out->identity = identity;
    out->resource_a = a;
    out->resource_b = b;
    out->name_group = mode == WP12FM_MODE_ROYAL
        ? wp12fm_royal_name_group(identity)
        : wp12fm_tag_name_group(identity,tag_second_lane);
    out->entry_set = set;
    out->world_x = mode == WP12FM_MODE_ROYAL ? 0x01D8u : (tag_second_lane ? 0x01F0u : 0x01C0u);
    out->world_y = 0x0130u;
    out->world_z = 0x0100u;
    out->speed_word = 0x0008u;
    out->angle_word = 0x0080u;
    out->facing_word = 0x0000u; /* $74B6 clears +$2E before the prematch route. */
    return WP12FM_OK;
}

bool wp12fm_identity_uses_cycle_rng(uint8_t identity)
{
    return identity == 0u || identity == 1u || identity == 7u || identity == 9u;
}

wp12fm_status_t wp12fm_walk_update(uint8_t identity,
                                   bool rng_bit0_valid,
                                   uint8_t rng_bit0,
                                   wp12fm_walk_state_t *state)
{
    uint16_t next_phase;
    bool wrapping;
    if (state == 0) return WP12FM_INVALID_ARGUMENT;
    if (identity >= WP12FM_IDENTITY_COUNT) return WP12FM_INVALID_IDENTITY;

    next_phase = (uint16_t)(state->phase_plus24 + 0x0101u);
    wrapping = (uint16_t)(next_phase & 0xFF00u) >= 0x0400u;
    if ((uint16_t)(state->counter_plus22 + 1u) >= WP12FM_WALK_HOLD_UPDATES &&
        wrapping && wp12fm_identity_uses_cycle_rng(identity) && !rng_bit0_valid) {
        return WP12FM_NEED_SHARED_RNG_21B4;
    }

    state->selector_plus05 = k_walk_selectors[(uint8_t)(state->phase_plus24 & 0x00FFu) & 7u];
    state->counter_plus22 = (uint16_t)(state->counter_plus22 + 1u);
    if (state->counter_plus22 < WP12FM_WALK_HOLD_UPDATES) return WP12FM_OK;

    state->counter_plus22 = 0u;
    state->phase_plus24 = next_phase;
    if (!wrapping) return WP12FM_OK;

    state->phase_plus24 = 0u;
    if (wp12fm_identity_uses_cycle_rng(identity) && (rng_bit0 & 1u) == 0u) {
        state->phase_plus24 = 4u;
    }
    return WP12FM_OK;
}

int32_t wp12fm_world_y_after_updates(uint32_t updates)
{
    return (int32_t)0x01300000 - (int32_t)(updates * 0x00008000u);
}

bool wp12fm_terminal_after_updates(uint32_t updates)
{
    int32_t y = wp12fm_world_y_after_updates(updates);
    return (int16_t)((uint32_t)y >> 16) < (int16_t)0x0090;
}

int8_t wp12fm_x_offset_plus19(uint8_t identity, uint8_t selector_plus05)
{
    (void)identity;
    (void)selector_plus05;
    return 0;
}

int8_t wp12fm_y_offset_plus1b(uint8_t identity, uint8_t selector_plus05)
{
    if (identity >= WP12FM_IDENTITY_COUNT) return 0;
    return identity == 1u && selector_plus05 >= 4u ? (int8_t)0x30 : 0;
}

int16_t wp12fm_screen_x(uint16_t world_x, uint16_t scroll_x, int8_t x_offset, uint16_t facing_plus2e)
{
    int16_t off = (int16_t)x_offset;
    if ((facing_plus2e & 0x8000u) != 0u) off = (int16_t)-off;
    return (int16_t)((int32_t)(int16_t)world_x - (int32_t)(int16_t)scroll_x + off);
}

int16_t wp12fm_screen_y(uint16_t world_y, uint16_t world_z, uint16_t scroll_y, int8_t y_offset)
{
    return (int16_t)((int32_t)(int16_t)world_y + (int32_t)(int16_t)world_z -
                     (int32_t)(int16_t)scroll_y + (int16_t)y_offset);
}

bool wp12fm_overlay_owner(wp12fm_mode_t mode, bool tag_second_lane)
{
    return mode == WP12FM_MODE_ROYAL || !tag_second_lane;
}

uint16_t wp12fm_reveal_vblanks(void) { return 24u; }
uint16_t wp12fm_close_vblanks(void) { return 24u; }
uint16_t wp12fm_close_tail_waits(void) { return WP12FM_CLOSE_INTERNAL_WAIT; }
