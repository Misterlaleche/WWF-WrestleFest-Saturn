#ifndef WWF_ARCADE_ATTRACT_H
#define WWF_ARCADE_ATTRACT_H
#include <stdbool.h>
#include <stdint.h>
enum { ARCADE_ATTRACT_STATE_COUNT = 14 };
typedef enum { ATTRACT_KIND_TAG_DEMO=0, ATTRACT_KIND_DEMOLITION_A, ATTRACT_KIND_WRESTLER_DEMO, ATTRACT_KIND_TAG_RECORDS, ATTRACT_KIND_ROYAL_DEMO, ATTRACT_KIND_DEMOLITION_B, ATTRACT_KIND_ROYAL_RECORDS } arcade_attract_kind_t;
typedef struct { uint16_t duration; int8_t featured_character; arcade_attract_kind_t kind; } arcade_attract_state_desc_t;
typedef struct { uint8_t state; uint16_t tick; } arcade_attract_t;
extern const arcade_attract_state_desc_t arcade_attract_states[ARCADE_ATTRACT_STATE_COUNT];
void arcade_attract_reset(arcade_attract_t *a);
bool arcade_attract_tick(arcade_attract_t *a,unsigned ticks);
const arcade_attract_state_desc_t *arcade_attract_current(const arcade_attract_t *a);
#endif
