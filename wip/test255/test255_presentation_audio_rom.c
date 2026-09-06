#include "arcade_presentation_audio.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void){
 static const uint8_t script[24]={
  0x00,0x00,0x7B,0x00,0xFF,0x00,0x7C,0x00,0xFF,0x00,0x00,0x7D,
  0x00,0xFF,0x00,0x7C,0x00,0xFF,0x00,0x00,0x00,0x00,0x00,0x00};
 static const uint8_t chars[12]={0x6D,0x6E,0x6F,0x74,0x73,0x73,0x71,0x70,0x72,0x75,0x73,0x73};
 static const uint8_t send_script[]={0x00,0x40,0x31,0x00,0x4e,0xb9,0x00,0x00,0x20,0x52};
 static const uint8_t char_lookup_send[]={0x10,0x31,0x00,0x00,0x00,0x40,0x31,0x00,0x4e,0xb9,0x00,0x00,0x20,0x52};
 static const uint8_t special73[]={0x0c,0x00,0x00,0x73,0x66,0x08,0x33,0xfc,0x80,0x00,0x00,0x1c,0x01,0x68};
 FILE*f=fopen("program.bin","rb");assert(f);
 uint8_t *rom=malloc(0xAC6E);assert(rom);assert(fread(rom,1,0xAC6E,f)==0xAC6E);fclose(f);
 assert(!memcmp(rom+0xAC56,script,sizeof script));
 assert(!memcmp(rom+0xAC4A,chars,sizeof chars));
 assert(!memcmp(arcade_presentation_script_commands,script,sizeof script));
 assert(!memcmp(arcade_presentation_character_commands,chars,sizeof chars));
 assert(!memcmp(rom+0xAB1E,send_script,sizeof send_script));
 assert(!memcmp(rom+0xAB3A,char_lookup_send,sizeof char_lookup_send));
 assert(!memcmp(rom+0xAB4E,special73,sizeof special73));
 for(unsigned i=0;i<24;i++)assert(arcade_presentation_script_command(i)==script[i]);
 for(unsigned i=0;i<12;i++)assert(arcade_presentation_character_command(i)==chars[i]);
 assert(arcade_presentation_character_command(4)==0x73);
 assert(arcade_presentation_character_command(5)==0x73);
 assert(arcade_presentation_character_command(10)==0x73);
 assert(arcade_presentation_character_command(11)==0x73);
 assert(arcade_presentation_command_sets_flag_0168(0x73));
 assert(!arcade_presentation_command_sets_flag_0168(0x7B));
 free(rom);
 puts("PASS Test255: presentation audio producer tables and $2052 command path match original ROM");
 return 0;
}
