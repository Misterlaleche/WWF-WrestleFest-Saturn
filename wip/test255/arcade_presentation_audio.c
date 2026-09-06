#include "arcade_presentation_audio.h"

/* program.bin $AC56, indexed by the local presentation substate byte.
 * 0x00/0xFF route through the selected-character command path at $AB2A.
 */
const uint8_t arcade_presentation_script_commands[ARCADE_PRESENTATION_SCRIPT_COUNT]={
 0x00,0x00,0x7B,0x00,0xFF,0x00,0x7C,0x00,
 0xFF,0x00,0x00,0x7D,0x00,0xFF,0x00,0x7C,
 0x00,0xFF,0x00,0x00,0x00,0x00,0x00,0x00
};

/* program.bin $AC4A, indexed by wrestler id at actor +$57.
 * IDs: Hogan, Warrior, Jake, Boss Man, Smash, Crush, Slaughter,
 * Perfect, Earthquake, DiBiase, Hawk, Animal.
 */
const uint8_t arcade_presentation_character_commands[ARCADE_PRESENTATION_CHARACTER_COUNT]={
 0x6D,0x6E,0x6F,0x74,0x73,0x73,0x71,0x70,0x72,0x75,0x73,0x73
};

uint8_t arcade_presentation_script_command(unsigned substate){
 return substate<ARCADE_PRESENTATION_SCRIPT_COUNT?arcade_presentation_script_commands[substate]:0;
}
uint8_t arcade_presentation_character_command(unsigned character){
 return character<ARCADE_PRESENTATION_CHARACTER_COUNT?arcade_presentation_character_commands[character]:0;
}
int arcade_presentation_command_sets_flag_0168(uint8_t command){return command==0x73;}
