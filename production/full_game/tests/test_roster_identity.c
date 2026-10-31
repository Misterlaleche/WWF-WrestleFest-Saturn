#include "wf_roster_identity.h"
#include <assert.h>
#include <stdio.h>
int main(void){uint8_t raw=255;
 assert(wf_roster_raw_identity(1,&raw)&&raw==7); /* ROM $5DA0: Perfect, not raw1 Warrior. */
 assert(wf_roster_raw_identity(8,&raw)&&raw==10); /* Demolition, not Legion of Doom. */
 assert(wf_roster_raw_identity(9,&raw)&&raw==11);
 assert(!wf_roster_raw_identity(10,&raw)&&raw==11);
 assert(!wf_roster_raw_identity(0x100,&raw)&&raw==11);
 assert(!wf_roster_raw_identity(0,NULL));
 puts("identity: mapped ROM slots and rejected invalid words PASS");}
