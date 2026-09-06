#ifndef WWF_ARCADE_ATTRACT_TRANSITION_H
#define WWF_ARCADE_ATTRACT_TRANSITION_H
#include <stdbool.h>
#include <stdint.h>
#include "arcade_attract_records.h"

typedef struct {
 bool active;
 uint8_t entry_state;
 uint8_t interstitial_scene;
 arcade_record_mode_t records_mode;
 uint16_t records_hold;
 uint8_t transition_wipes; /* calls to $264E2 around $69C8 */
} arcade_attract_entry_transition_t;

/* ROM-EXACT setup branch at $0B1C. Only entry states 1 and 2 execute the
 * Demolition/presenter -> records sequence. */
arcade_attract_entry_transition_t arcade_attract_entry_transition(unsigned entry_state);
#endif
