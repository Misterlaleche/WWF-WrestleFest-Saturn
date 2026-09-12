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
 uint8_t credit_cycle_units_004b;
 uint8_t credit_remainder_004e;
 uint8_t credit_count_004f;
 uint16_t config_0066;
 uint8_t frame_handshake[2];
 uint8_t gameplay_vblank_enable;
 uint16_t credited_route;
 uint32_t frame_counter;
 uint8_t pipeline_mode;
 uint8_t mode_0160;
 uint8_t mode_0161;
 uint16_t progression_0162;
 uint16_t global_0166;
 uint32_t global_168c;
 uint16_t global_1692;
 uint8_t controller_suppress;
 uint8_t global_1697;
 uint8_t global_169b;
 uint16_t global_169e;
 uint16_t global_16c4;
 uint8_t global_1678;
 uint8_t global_15d3;
 uint8_t global_15d5;
 uint8_t global_00a3;
 uint8_t cabinet_edge_15da;
 uint8_t cabinet_level_15db;
 uint8_t cabinet_edge_15dc;
 uint8_t cabinet_level_15dd;
 uint32_t rng_state;
 uint32_t rng_d2, rng_d3, rng_d4, rng_d5, rng_d6, rng_d7;
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
