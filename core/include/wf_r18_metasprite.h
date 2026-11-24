#ifndef WF_R18_METASPRITE_H
#define WF_R18_METASPRITE_H
#include "wf_runtime.h"
typedef wf_status_t (*wf_r18_read_rom_fn)(void *,uint32_t,void *,size_t);
typedef struct wf_r18_sprite_record { uint8_t bytes[16]; } wf_r18_sprite_record_t;
#define WF_R18_METASPRITE_CACHE_ENTRIES 32u
#define WF_R18_METASPRITE_CACHE_ROWS 32u
#define WF_R18_METASPRITE_CACHE_BUCKETS 64u
#define WF_R18_METASPRITE_POSE_CACHE_ENTRIES 32u
#define WF_R18_METASPRITE_WORD_CACHE_ENTRIES 64u
#define WF_R18_METASPRITE_PROGRAM_ENTRIES 16u
#define WF_R18_METASPRITE_PROGRAM_OPS 64u
#ifdef WF_JO_ENGINE
#define WF_R18_METASPRITE_HINT_SLOTS 64u
#endif

/* Cached immutable ROM composition, before screen clipping and live palette
 * lookup. Eight-byte operations contain a source row or a D6/D7 side effect. */
typedef struct wf_r18_metasprite_program {
 uint16_t set,pose,partner;
 uint8_t valid,count;
 uint32_t age;
 uint8_t op[WF_R18_METASPRITE_PROGRAM_OPS][8];
} wf_r18_metasprite_program_t;
typedef struct wf_r18_metasprite_cache_entry {
 uint32_t address,end;
 uint16_t header;
 uint8_t skip_prefix,valid;
 uint8_t rows[WF_R18_METASPRITE_CACHE_ROWS][6];
} wf_r18_metasprite_cache_entry_t;
typedef struct wf_r18_metasprite_set_cache_entry {
 uint32_t base;
 uint16_t rel;
 uint8_t valid,reserved;
} wf_r18_metasprite_set_cache_entry_t;
typedef struct wf_r18_metasprite_pose_cache_entry {
 uint16_t pose,offset;
 uint8_t set,valid;
} wf_r18_metasprite_pose_cache_entry_t;
typedef struct wf_r18_metasprite_cache {
 wf_r18_metasprite_cache_entry_t entries[WF_R18_METASPRITE_CACHE_ENTRIES];
 uint8_t buckets[WF_R18_METASPRITE_CACHE_BUCKETS]; /* One-based entry indices. */
 uint8_t links[WF_R18_METASPRITE_CACHE_ENTRIES];
 uint8_t next;
 wf_r18_metasprite_set_cache_entry_t wrestler_sets[12];
 wf_r18_metasprite_pose_cache_entry_t pose_offsets[WF_R18_METASPRITE_POSE_CACHE_ENTRIES];
 uint32_t word_tag[WF_R18_METASPRITE_WORD_CACHE_ENTRIES];
 uint16_t word_data[WF_R18_METASPRITE_WORD_CACHE_ENTRIES];
 wf_r18_metasprite_program_t programs[WF_R18_METASPRITE_PROGRAM_ENTRIES];
 uint32_t program_clock;
#ifdef WF_JO_ENGINE
 /* One-based entry hints only; every hit validates the full program key.
  * Keep all legacy cache bytes and their eviction policy unchanged. */
 uint8_t program_hint[WF_R18_METASPRITE_HINT_SLOTS];
#endif
} wf_r18_metasprite_cache_t;
/* Caller-owned, allocation-free decoded-body cache. The ROM source must
 * remain immutable until reset; reset before changing reader/user/source.
 * Entries contain raw rows only, never screen or palette decisions. */
void wf_r18_metasprite_cache_reset(wf_r18_metasprite_cache_t *);
typedef struct wf_r18_metasprite_ctx {
 wf_r18_read_rom_fn read_rom; void *user;
 const uint8_t *palette_map; /* actual $1C1610, 256 resource-to-bank bytes */
 wf_r18_sprite_record_t *records; size_t capacity,count;
 uint32_t d6,d7;
 wf_r18_metasprite_cache_t *cache; /* Optional; NULL keeps the original decoder. */
} wf_r18_metasprite_ctx_t;
/* Character is the GRAPHICS set WORD at actor+2, not roster internal ID.
 * partner_set is resolved from actor+26; required only for kind2.
 * Raw words are normalized to fields consumed by WP26: unused high bytes
 * and reserved words are zero. D6/D7 are source control-flow side effects.
 * Failure rolls count/D6/D7 back; written uncommitted output is ignored.
 */
wf_status_t wf_r18_metasprite_emit(wf_r18_metasprite_ctx_t *,uint16_t set,uint16_t pose,uint16_t screen_x,uint16_t screen_y,uint16_t partner_set);
#endif
