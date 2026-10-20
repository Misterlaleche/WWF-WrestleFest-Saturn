#ifndef WF_R18_METASPRITE_H
#define WF_R18_METASPRITE_H
#include "wf_runtime.h"
typedef wf_status_t (*wf_r18_read_rom_fn)(void *,uint32_t,void *,size_t);
typedef struct wf_r18_sprite_record { uint8_t bytes[16]; } wf_r18_sprite_record_t;
typedef struct wf_r18_metasprite_ctx {
 wf_r18_read_rom_fn read_rom; void *user;
 const uint8_t *palette_map; /* actual $1C1610, 256 resource-to-bank bytes */
 wf_r18_sprite_record_t *records; size_t capacity,count;
 uint32_t d6,d7;
} wf_r18_metasprite_ctx_t;
/* Character is the GRAPHICS set WORD at actor+2, not roster internal ID.
 * partner_set is resolved from actor+26; required only for kind2.
 * Raw words are normalized to fields consumed by WP26: unused high bytes
 * and reserved words are zero. D6/D7 are source control-flow side effects.
 * Failure rolls count/D6/D7 back; written uncommitted output is ignored.
 */
wf_status_t wf_r18_metasprite_emit(wf_r18_metasprite_ctx_t *,uint16_t set,uint16_t pose,uint16_t screen_x,uint16_t screen_y,uint16_t partner_set);
#endif
