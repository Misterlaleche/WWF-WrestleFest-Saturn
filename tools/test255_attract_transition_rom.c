#include "arcade_attract_transition.h"
#include "arcade_interstitial.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(void){
 FILE*f=fopen("program.bin","rb");assert(f);uint8_t*b=malloc(0xB80);assert(b);assert(fread(b,1,0xB80,f)==0xB80);fclose(f);
 /* $0B1C defaults D0=1. State1 branches directly to $B40. State2 changes
  * D0=2 and sets byte $1C0161=1 (Royal records selector) before the same path. */
 const uint8_t prefix[]={0x30,0x3c,0x00,0x01,0x0c,0x79,0x00,0x01,0x00,0x1c,0x01,0x66,0x67,0x16,0x0c,0x79,0x00,0x02,0x00,0x1c,0x01,0x66,0x66,0x36,0x30,0x3c,0x00,0x02,0x13,0xfc,0x00,0x01,0x00,0x1c,0x01,0x61};
 assert(!memcmp(b+0xB1C,prefix,sizeof prefix));
 /* Common blocking order: AE20, wipe, records $69C8, hold $0100, wipe. */
 const uint8_t chain[]={0x4e,0xb9,0x00,0x00,0xae,0x20,0x4e,0xb9,0x00,0x02,0x64,0xe2,0x4e,0xb9,0x00,0x00,0x69,0xc8,0x30,0x3c,0x01,0x00,0x4e,0xb9,0x00,0x00,0x21,0xe6,0x4e,0xb9,0x00,0x02,0x64,0xe2};
 assert(!memcmp(b+0xB40,chain,sizeof chain));
 arcade_attract_entry_transition_t a=arcade_attract_entry_transition(1),r=arcade_attract_entry_transition(2),n=arcade_attract_entry_transition(3);
 assert(a.active&&a.interstitial_scene==0&&a.records_mode==ARCADE_RECORD_TAG&&a.records_hold==0x100&&a.transition_wipes==2);
 assert(r.active&&r.interstitial_scene==1&&r.records_mode==ARCADE_RECORD_ROYAL&&r.records_hold==0x100&&r.transition_wipes==2);
 assert(!n.active);
 assert(arcade_interstitial_scenes[a.interstitial_scene].speaking_object==ARCADE_INTERSTITIAL_RIGHT_OBJECT);
 assert(arcade_interstitial_scenes[r.interstitial_scene].speaking_object==ARCADE_INTERSTITIAL_LEFT_OBJECT);
 free(b);puts("PASS Test255: state1/state2 entry sequence is exact Demolition -> TAG/Royal records -> $0100 hold");return 0;
}
