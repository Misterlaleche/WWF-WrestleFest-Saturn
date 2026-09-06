#include "arcade_attract.h"
#include <string.h>
/* Test255: program.bin ROM-EXACT tables/control flow.
 * durations: $119A (14 big-endian words)
 * ordinary four-wrestler state rosters: $0F3E (14 x 4 bytes)
 * state 1 (Royal) takes the special $0ECC path and instantiates six actors
 * from $0F76: Boss Man, Slaughter, Perfect, Earthquake, DiBiase, Jake.
 * featured portrait wrestler: $8E9A (14 big-endian words; states 0/1 = $FFFF)
 */
const arcade_attract_state_desc_t arcade_attract_states[ARCADE_ATTRACT_STATE_COUNT]={
 { 848,4,{ 0, 1, 4, 5,0,0},-1},
 { 736,6,{ 3, 6, 7, 8,9,2},-1},
 {1040,4,{ 0, 1, 6, 7,0,0}, 0},
 { 928,4,{ 1, 0, 9, 8,0,0}, 1},
 { 656,4,{ 2, 3,10,11,0,0}, 2},
 { 928,4,{ 3, 2, 6, 9,0,0}, 3},
 {1200,4,{ 6, 7, 0, 1,0,0}, 6},
 { 944,4,{ 7, 9, 2, 0,0,0}, 7},
 {1616,4,{ 8, 6, 3, 2,0,0}, 8},
 {1152,4,{ 9, 8, 1, 3,0,0}, 9},
 { 800,4,{10,11, 2, 1,0,0},10},
 { 816,4,{11,10, 2, 1,0,0},11},
 { 832,4,{ 4, 5, 0, 1,0,0}, 4},
 { 864,4,{ 5, 4, 0, 1,0,0}, 5},
};
void arcade_attract_reset(arcade_attract_t*a){memset(a,0,sizeof(*a));}
const arcade_attract_state_desc_t*arcade_attract_current(const arcade_attract_t*a){return a&&a->state<ARCADE_ATTRACT_STATE_COUNT?&arcade_attract_states[a->state]:0;}
bool arcade_attract_tick(arcade_attract_t*a,unsigned ticks){
 if(!a||!ticks)return false;
 bool changed=false;
 while(ticks--){const arcade_attract_state_desc_t*d=arcade_attract_current(a);if(!d)return changed;
  if(++a->tick>=d->duration){a->tick=0;a->state=(uint8_t)((a->state+1)%ARCADE_ATTRACT_STATE_COUNT);changed=true;}
 }
 return changed;
}
