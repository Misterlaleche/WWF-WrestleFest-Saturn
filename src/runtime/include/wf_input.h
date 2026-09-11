#ifndef WF_INPUT_H
#define WF_INPUT_H
#include <stdint.h>
#include "wf_actor.h"
extern const uint8_t wf_action_matrix[12][21][3];
/* hardware_low is the cabinet/Saturn-adapter active-low low byte before ROM NOT.B. */
void wf_input_decode_514e(wf_actor_raw_t *a, uint8_t hardware_low, uint8_t pipeline_bit0);
uint8_t wf_input_raw_action_lookup(uint8_t character_id,uint8_t category,uint8_t fresh_mask);
uint16_t wf_input_direction_state1(uint8_t held_direction_nibble);
#endif
