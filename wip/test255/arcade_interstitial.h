#ifndef WWF_ARCADE_INTERSTITIAL_H
#define WWF_ARCADE_INTERSTITIAL_H
#include <stdint.h>

typedef struct { uint8_t frame; uint8_t duration; uint8_t audio_event; } arcade_interstitial_phase_t;
typedef struct {
 const arcade_interstitial_phase_t *phases;
 uint8_t phase_count;
 uint16_t active_ticks;
 uint16_t voice_command;
 uint8_t attract_entry_state; /* routine is invoked while entering this attract state */
} arcade_interstitial_desc_t;

enum { ARCADE_INTERSTITIAL_COUNT=2 };
extern const arcade_interstitial_desc_t arcade_interstitials[ARCADE_INTERSTITIAL_COUNT];
#endif
