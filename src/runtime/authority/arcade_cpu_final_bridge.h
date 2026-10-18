#ifndef WWF_ARCADE_CPU_FINAL_BRIDGE_H
#define WWF_ARCADE_CPU_FINAL_BRIDGE_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* ROM-EXACT numeric CPU execution bridge for WWF WrestleFest.
 * This module intentionally exposes raw arcade state/selector/callback values.
 * It does not assign unproven move names and does not alias native fighter_state_t. */

enum { ARCADE_CPU_MAIN_STATE_COUNT=13, ARCADE_CPU_STATE1_SUBSTATE_COUNT=13, ARCADE_CPU_SELECTOR_COUNT=143 };

typedef struct { uint32_t seed; uint32_t global_80; uint32_t d2,d3,d4,d5,d6,d7; } arcade_cpu_rng_t;
typedef struct { uint32_t raw_entry; uint32_t callback; bool force_call; } arcade_cpu_exec_entry_t;
typedef struct { uint16_t b0,b2; uint8_t b4,b5_low2; } arcade_cpu_geometry_t;

uint32_t arcade_cpu_main_handler(uint8_t raw_state_low);
uint32_t arcade_cpu_state1_handler(uint8_t raw_af);
arcade_cpu_exec_entry_t arcade_cpu_executor_entry(uint8_t selector);
bool arcade_cpu_executor_should_call(uint8_t selector,uint16_t raw_state20);
uint32_t arcade_cpu_rng_step(arcade_cpu_rng_t *r,uint32_t d1);
int arcade_cpu_weighted_select(arcade_cpu_rng_t *r,const uint8_t *weights,size_t count,uint16_t *sample_out);
arcade_cpu_geometry_t arcade_cpu_target_geometry(uint16_t actor_x,uint16_t actor_y,uint16_t target_x,uint16_t target_y,
 uint8_t target_state_low,uint8_t target65,uint8_t target2e,uint8_t target_selector61);
uint8_t arcade_cpu_stamina_bucket(uint16_t stamina);
bool arcade_cpu_state9_lod_partner_trigger(uint8_t bd_after_increment,uint8_t partner_state_low,uint8_t partner_selector61,
 uint16_t *raw_state20,uint16_t *raw60);
const uint8_t *arcade_cpu_state6_weight_pair(uint8_t progression,uint8_t stamina_tier);
uint8_t arcade_cpu_state9_selector(uint8_t internal_id,bool target_ground_or_selector61);
const uint32_t *arcade_cpu_main_table(void);
const uint32_t *arcade_cpu_state1_table(void);
const uint32_t *arcade_cpu_executor_table(void);

#endif
