#ifndef WF_RUNTIME_H
#define WF_RUNTIME_H
#include <stdint.h>
#include <stddef.h>
#include "wf_actor.h"

typedef enum wf_status { WF_OK=0, WF_BLOCKED=1, WF_BAD_STATE=2 } wf_status_t;
typedef struct wf_runtime wf_runtime_t;
typedef wf_status_t (*wf_phase_fn)(wf_runtime_t *rt, uint32_t rom_addr, void *user);

typedef struct wf_phase_binding { uint32_t rom_addr; wf_phase_fn fn; void *user; } wf_phase_binding_t;

typedef struct wf_globals_raw {
 /* Direct arcade globals retained by numeric address role where gameplay consumes them. */
 uint8_t credit_cycle_units_004b; /* $1C004B */
 uint8_t credit_remainder_004e;   /* $1C004E */
 uint8_t credit_count_004f;       /* $1C004F */
 uint16_t config_0066;            /* $1C0066/$0067 */
 uint8_t frame_handshake[2]; /* $1C006E/$6F equivalent */
 uint8_t gameplay_vblank_enable; /* $1C0076 */
 uint16_t credited_route; /* $1C007C */
 uint32_t frame_counter; /* $1C0080 */
 uint8_t pipeline_mode; /* $1C0083; bit0 selects A/B */
 uint8_t mode_0160; /* $1C0160 */
 uint8_t mode_0161; /* $1C0161 */
 uint16_t progression_0162; /* $1C0162: CPU progression selector used by state6/controller15 */
 uint16_t global_0166; /* $1C0166 word */
 uint8_t controller_suppress;
 uint8_t global_1697; /* $1C1697, only numeric bit contract is used */
 uint8_t global_169b; /* $1C169B */
 uint16_t global_169e; /* $1C169E */
 uint16_t global_16c4; /* $1C16C4 */
 uint8_t cabinet_edge_15da;  /* $1C15DA */
 uint8_t cabinet_level_15db; /* $1C15DB */
 uint8_t cabinet_edge_15dc;  /* $1C15DC */
 uint8_t cabinet_level_15dd; /* $1C15DD */
 uint32_t rng_state;  /* arcade $1C005C equivalent: value must be externally sourced */
 uint32_t rng_d2, rng_d3, rng_d4, rng_d5, rng_d6, rng_d7; /* caller register context for $21B4 */
 uint8_t rng_state_valid;
 uint8_t rng_register_context_valid;
} wf_globals_raw_t;

struct wf_runtime {
 wf_globals_raw_t g;
 wf_actor_raw_t actors[WF_ACTOR_SLOTS];
 wf_phase_binding_t *bindings;
 size_t binding_count;
 uint32_t trace[64];
 size_t trace_count;
 uint8_t frame_ready;
 uint8_t vblank_ack;
};

void wf_runtime_init(wf_runtime_t *rt, wf_phase_binding_t *bindings, size_t binding_count);
wf_status_t wf_runtime_frame(wf_runtime_t *rt);
wf_status_t wf_dispatch_phase(wf_runtime_t *rt,uint32_t addr);
void wf_for_each_active(wf_runtime_t *rt, void (*fn)(wf_runtime_t*,unsigned,wf_actor_raw_t*,void*), void *user);
wf_actor_raw_t *wf_actor_from_rom_ptr(wf_runtime_t *rt,uint32_t ptr,unsigned *slot_out);
const wf_actor_raw_t *wf_actor_from_rom_ptr_const(const wf_runtime_t *rt,uint32_t ptr,unsigned *slot_out);
void wf_apply_pending_damage_and_tier(wf_actor_raw_t *a);
#endif
