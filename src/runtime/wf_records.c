#include "wf_records.h"
#include <string.h>

static const wf_record_entry_t k_tag_defaults[WF_RECORD_ENTRY_COUNT] = {
    {8, {'T','A','K'}, 0, 1},
    {6, {'H','A','G'}, 2, 3},
    {5, {'H','A','N'}, 6, 7},
    {5, {'S','I','N'}, 8, 9},
    {4, {'T','A','T'}, 2, 3},
    {4, {'K','I','N'}, 0, 1},
    {3, {'N','A','R'}, 2, 3},
    {3, {'M','A','E'}, 6, 7},
    {2, {'S','A','K'}, 8, 9},
    {2, {'I','S','I'}, 2, 3},
    {1, {'T','O','M'}, 0, 1}
};

static const wf_record_entry_t k_royal_defaults[WF_RECORD_ENTRY_COUNT] = {
    {9, {'T','A','K'}, 0, 1},
    {7, {'H','A','G'}, 2, 3},
    {6, {'H','A','N'}, 6, 7},
    {5, {'S','I','N'}, 8, 9},
    {4, {'T','A','T'}, 2, 3},
    {4, {'K','I','N'}, 0, 1},
    {3, {'N','A','R'}, 2, 3},
    {3, {'M','A','E'}, 6, 7},
    {2, {'S','A','K'}, 8, 9},
    {2, {'I','S','I'}, 2, 3},
    {1, {'T','O','M'}, 0, 1}
};

static bool is_new(const wf_record_entry_t *e)
{
    return e->initials[1] == WF_RECORD_NEW_MARKER;
}

void wf_records_reset_defaults(wf_record_tables_t *tables)
{
    if (!tables) return;
    memcpy(tables->tag, k_tag_defaults, sizeof(k_tag_defaults));
    memcpy(tables->royal, k_royal_defaults, sizeof(k_royal_defaults));
}

void wf_records_sort_rom_exact(wf_record_entry_t table[WF_RECORD_ENTRY_COUNT])
{
    bool changed;
    unsigned i;
    if (!table) return;
    do {
        changed = false;
        for (i = 0; i + 1u < WF_RECORD_ENTRY_COUNT; ++i) {
            wf_record_entry_t *a = &table[i];
            wf_record_entry_t *b = &table[i + 1u];
            bool swap = false;
            if (a->value < b->value) swap = true;
            else if (a->value == b->value && !is_new(a) && is_new(b)) swap = true;
            if (swap) {
                wf_record_entry_t tmp = *a;
                *a = *b;
                *b = tmp;
                changed = true;
            }
        }
    } while (changed);
}

bool wf_records_qualify(wf_record_entry_t table[WF_RECORD_ENTRY_COUNT],
                        wf_record_mode_t mode,
                        uint16_t candidate,
                        uint8_t player_id,
                        uint8_t wrestler_1,
                        uint8_t wrestler_2_or_shared,
                        uint8_t *rank_index_out)
{
    unsigned i;
    uint8_t value;
    if (!table) return false;
    if (mode != WF_RECORD_MODE_TAG && mode != WF_RECORD_MODE_ROYAL) return false;
    if (mode == WF_RECORD_MODE_ROYAL && candidate > 100u) candidate = 0u;
    value = (uint8_t)candidate;
    if (value < table[WF_RECORD_ENTRY_COUNT - 1u].value) return false;

    table[WF_RECORD_ENTRY_COUNT - 1u].value = value;
    table[WF_RECORD_ENTRY_COUNT - 1u].initials[0] = WF_RECORD_INITIAL_BLANK;
    table[WF_RECORD_ENTRY_COUNT - 1u].initials[1] = WF_RECORD_NEW_MARKER;
    table[WF_RECORD_ENTRY_COUNT - 1u].initials[2] = player_id;
    table[WF_RECORD_ENTRY_COUNT - 1u].wrestler_1 = wrestler_1;
    table[WF_RECORD_ENTRY_COUNT - 1u].wrestler_2_or_shared = wrestler_2_or_shared;

    wf_records_sort_rom_exact(table);

    for (i = 0; i < WF_RECORD_ENTRY_COUNT; ++i) {
        if (table[i].initials[1] == WF_RECORD_NEW_MARKER &&
            table[i].initials[2] == player_id) {
            table[i].initials[1] = WF_RECORD_INITIAL_BLANK;
            table[i].initials[2] = WF_RECORD_INITIAL_BLANK;
            if (rank_index_out) *rank_index_out = (uint8_t)i;
            return true;
        }
    }
    return false;
}

uint16_t wf_records_tag_display_value(uint8_t stored_value) { return (uint16_t)stored_value + 1u; }
uint16_t wf_records_royal_display_value(uint8_t stored_value) { return (uint16_t)stored_value; }

uint8_t wf_records_initial_step_right(uint8_t selector_code)
{
    if (selector_code < WF_RECORD_INITIAL_SELECTOR_A || selector_code > WF_RECORD_INITIAL_SELECTOR_BLANK)
        return WF_RECORD_INITIAL_SELECTOR_A;
    selector_code++;
    if (selector_code >= WF_RECORD_INITIAL_SELECTOR_BLANK) selector_code = WF_RECORD_INITIAL_SELECTOR_A;
    return selector_code;
}

uint8_t wf_records_initial_step_left(uint8_t selector_code)
{
    if (selector_code < WF_RECORD_INITIAL_SELECTOR_A || selector_code > WF_RECORD_INITIAL_SELECTOR_BLANK)
        return WF_RECORD_INITIAL_SELECTOR_A;
    if (selector_code == WF_RECORD_INITIAL_SELECTOR_A) return WF_RECORD_INITIAL_SELECTOR_BLANK;
    if (selector_code == WF_RECORD_INITIAL_SELECTOR_BLANK) return WF_RECORD_INITIAL_SELECTOR_Z;
    return (uint8_t)(selector_code - 1u);
}

bool wf_records_table_valid(const wf_record_entry_t table[WF_RECORD_ENTRY_COUNT])
{
    unsigned i;
    if (!table) return false;
    for (i = 1u; i < WF_RECORD_ENTRY_COUNT; ++i)
        if (table[i - 1u].value < table[i].value) return false;
    return true;
}
