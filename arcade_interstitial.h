#ifndef WWF_ARCADE_INTERSTITIAL_H
#define WWF_ARCADE_INTERSTITIAL_H
#include <stdbool.h>
#include <stdint.h>

typedef struct { uint8_t frame; uint8_t duration; uint8_t audio_event; } arcade_interstitial_phase_t;
typedef struct {
 const arcade_interstitial_phase_t *phases;
 uint8_t phase_count;
 uint16_t raw_duration_sum; /* sum of ROM duration bytes, excluding $FE */
 uint16_t update_calls_to_terminal; /* exact $B0C8 calls including initial load */
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

typedef struct {
 uint8_t phase;
 uint8_t frame;
 uint8_t timer;
 uint8_t audio_table_offset;
 bool active;
} arcade_interstitial_player_t;

enum {
 ARCADE_INTERSTITIAL_OBJECTS=3,
 ARCADE_INTERSTITIAL_SCENES=2,
 ARCADE_INTERSTITIAL_AUDIO_COMMANDS=4,
 ARCADE_INTERSTITIAL_LEFT_OBJECT=0,
 ARCADE_INTERSTITIAL_PRESENTER_OBJECT=1,
 ARCADE_INTERSTITIAL_RIGHT_OBJECT=2
};
extern const arcade_interstitial_object_t arcade_interstitial_objects[ARCADE_INTERSTITIAL_OBJECTS];
extern const arcade_interstitial_scene_t arcade_interstitial_scenes[ARCADE_INTERSTITIAL_SCENES];
extern const uint16_t arcade_interstitial_audio_commands[ARCADE_INTERSTITIAL_AUDIO_COMMANDS];

/* Native mirror of the speaker-script part of arcade routine $B0C8.
 * Only the object whose +$1D bit0 is set is advanced by this path. */
void arcade_interstitial_player_begin(arcade_interstitial_player_t *p,unsigned scene_index);
/* One $B0C8 update. Returns a raw sound command when a high-bit frame marker
 * consumes the next word from $B14A; otherwise returns 0. */
uint16_t arcade_interstitial_player_step(arcade_interstitial_player_t *p,unsigned scene_index);
#endif
