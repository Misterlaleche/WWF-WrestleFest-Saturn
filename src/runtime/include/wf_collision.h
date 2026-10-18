#ifndef WF_COLLISION_H
#define WF_COLLISION_H
#include <stdint.h>
#include "wf_actor.h"

typedef struct wf_collision_profile {
 uint8_t flags, primary_box, secondary_box, attack_box, pending_damage, first_selector, second_selector;
} wf_collision_profile_t;

extern const wf_collision_profile_t wf_collision_profiles[34];
extern const uint32_t wf_first_callbacks[15];
extern const uint32_t wf_second_callbacks[13];

/* $24D7A: low-byte-only normalization; preserves high +$64 flags. */
uint8_t wf_collision_normalize_low(uint8_t subtype);
void wf_collision_normalize_receiver(wf_actor_raw_t *src,wf_actor_raw_t *dst);

/* Profile ID is low byte +$4D of WORD +$4C. */
static inline uint8_t wf_collision_profile_id(const wf_actor_raw_t *a){ return wf_u8(a,WF_A_PROFILE+1u); }

#endif
