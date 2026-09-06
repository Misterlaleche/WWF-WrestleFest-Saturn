#ifndef WWF_ARCADE_INTERSTITIAL_H
#define WWF_ARCADE_INTERSTITIAL_H
#include <stdint.h>

typedef struct { uint8_t frame; uint8_t duration; uint8_t audio_event; } arcade_interstitial_phase_t;
typedef struct {
 const arcade_interstitial_phase_t *phases;
 uint8_t phase_count;
 uint16_t active_ticks;
 uint8_t object_index;
 uint8_t x;
 uint8_t palette_bank;
 uint8_t first_frame;
 uint8_t last_frame;
} arcade_interstitial_object_t;

typedef struct {
 uint8_t mode;
 uint8_t attract_entry_state;
 uint8_t speaking_object;
 uint8_t initial_audio_table_offset;
} arcade_interstitial_scene_t;

enum { ARCADE_INTERSTITIAL_OBJECTS=3, ARCADE_INTERSTITIAL_SCENES=2, ARCADE_INTERSTITIAL_AUDIO_COMMANDS=4 };
extern const arcade_interstitial_object_t arcade_interstitial_objects[ARCADE_INTERSTITIAL_OBJECTS];
extern const arcade_interstitial_scene_t arcade_interstitial_scenes[ARCADE_INTERSTITIAL_SCENES];
extern const uint16_t arcade_interstitial_audio_commands[ARCADE_INTERSTITIAL_AUDIO_COMMANDS];
#endif
