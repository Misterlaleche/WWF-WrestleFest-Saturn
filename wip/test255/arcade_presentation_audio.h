#ifndef WWF_ARCADE_PRESENTATION_AUDIO_H
#define WWF_ARCADE_PRESENTATION_AUDIO_H
#include <stdint.h>

enum {
 ARCADE_PRESENTATION_SCRIPT_COUNT = 24,
 ARCADE_PRESENTATION_CHARACTER_COUNT = 12
};

/* Test255 ROM-EXACT command producer data from program.bin.
 * These are raw sound-command identities. Semantic phrase/speaker mapping remains
 * deliberately unlabelled until its producer path is independently closed.
 */
extern const uint8_t arcade_presentation_script_commands[ARCADE_PRESENTATION_SCRIPT_COUNT];
extern const uint8_t arcade_presentation_character_commands[ARCADE_PRESENTATION_CHARACTER_COUNT];

uint8_t arcade_presentation_script_command(unsigned substate);
uint8_t arcade_presentation_character_command(unsigned character);
int arcade_presentation_command_sets_flag_0168(uint8_t command);

#endif
