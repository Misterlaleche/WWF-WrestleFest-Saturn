#ifndef WWF_ARCADE_ATTRACT_H
#define WWF_ARCADE_ATTRACT_H
#include <stdbool.h>
#include <stdint.h>
enum { ARCADE_ATTRACT_STATE_COUNT=14, ARCADE_ATTRACT_ROSTER_SIZE=4 };
typedef struct { uint16_t duration; uint8_t roster[ARCADE_ATTRACT_ROSTER_SIZE]; int8_t featured_character; } arcade_attract_state_desc_t;
typedef struct { uint8_t state; uint16_t tick; } arcade_attract_t;
extern const arcade_attract_state_desc_t arcade_attract_states[ARCADE_ATTRACT_STATE_COUNT];
void arcade_attract_reset(arcade_attract_t *a);
bool arcade_attract_tick(arcade_attract_t *a,unsigned ticks);
const arcade_attract_state_desc_t *arcade_attract_current(const arcade_attract_t *a);
#endif
