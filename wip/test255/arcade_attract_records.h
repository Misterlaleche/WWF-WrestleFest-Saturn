#ifndef WWF_ARCADE_ATTRACT_RECORDS_H
#define WWF_ARCADE_ATTRACT_RECORDS_H
#include <stdint.h>

enum { ARCADE_RECORD_ROWS=11, ARCADE_RECORD_NAME_LEN=3 };
typedef enum { ARCADE_RECORD_TAG=0, ARCADE_RECORD_ROYAL=1 } arcade_record_mode_t;
typedef struct {
 uint8_t metric_raw;          /* ROM byte 0: semantic label intentionally not promoted yet */
 char initials[ARCADE_RECORD_NAME_LEN];
 uint8_t wrestler_a;
 uint8_t wrestler_b;
} arcade_record_entry_t;
typedef struct {
 const arcade_record_entry_t *entries;
 uint16_t ram_address_low;    /* low 16 bits of $1C00C2 / $1C0104 */
 uint16_t hold_ticks;         /* setup at $0B52: $0100 */
 uint8_t tilemap_variant_x;   /* Royal adds 8 bytes to score column base */
} arcade_record_table_t;
extern const arcade_record_entry_t arcade_default_tag_records[ARCADE_RECORD_ROWS];
extern const arcade_record_entry_t arcade_default_royal_records[ARCADE_RECORD_ROWS];
extern const arcade_record_table_t arcade_record_tables[2];
extern const uint16_t arcade_record_name_offsets[ARCADE_RECORD_ROWS];
extern const uint16_t arcade_record_metric_offsets[ARCADE_RECORD_ROWS];
#endif
