#ifndef WF_RECORDS_H
#define WF_RECORDS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define WF_RECORD_ENTRY_COUNT 11u
#define WF_RECORD_ENTRY_BYTES 6u
#define WF_RECORD_TABLE_BYTES 66u
#define WF_RECORD_COMBINED_BYTES 132u
#define WF_RECORD_INITIAL_BLANK 0x5Bu
#define WF_RECORD_NEW_MARKER 0x5Cu
#define WF_RECORD_INITIAL_SELECTOR_A 0x31u
#define WF_RECORD_INITIAL_SELECTOR_Z 0x4Au
#define WF_RECORD_INITIAL_SELECTOR_BLANK 0x4Bu
#define WF_RECORD_INITIALS_TIMEOUT 0x0700u
#define WF_RECORD_HORIZONTAL_REPEAT_UPDATES 8u

typedef enum wf_record_mode {
    WF_RECORD_MODE_TAG = 0,
    WF_RECORD_MODE_ROYAL = 1
} wf_record_mode_t;

typedef struct wf_record_entry {
    uint8_t value;
    uint8_t initials[3];
    uint8_t wrestler_1;
    uint8_t wrestler_2_or_shared;
} wf_record_entry_t;

typedef struct wf_record_tables {
    wf_record_entry_t tag[WF_RECORD_ENTRY_COUNT];
    wf_record_entry_t royal[WF_RECORD_ENTRY_COUNT];
} wf_record_tables_t;

void wf_records_reset_defaults(wf_record_tables_t *tables);
void wf_records_sort_rom_exact(wf_record_entry_t table[WF_RECORD_ENTRY_COUNT]);
bool wf_records_qualify(wf_record_entry_t table[WF_RECORD_ENTRY_COUNT],
                        wf_record_mode_t mode,
                        uint16_t candidate,
                        uint8_t player_id,
                        uint8_t wrestler_1,
                        uint8_t wrestler_2_or_shared,
                        uint8_t *rank_index_out);
uint16_t wf_records_tag_display_value(uint8_t stored_value);
uint16_t wf_records_royal_display_value(uint8_t stored_value);
uint8_t wf_records_initial_step_right(uint8_t selector_code);
uint8_t wf_records_initial_step_left(uint8_t selector_code);
bool wf_records_table_valid(const wf_record_entry_t table[WF_RECORD_ENTRY_COUNT]);

#endif
