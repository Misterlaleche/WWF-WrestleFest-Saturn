#include "arcade_attract.h"
#include <string.h>
/* ROM-EXACT tables: durations $119A; state rosters $0F3E; featured wrestler $8E9A. */
const arcade_attract_state_desc_t arcade_attract_states[ARCADE_ATTRACT_STATE_COUNT]={
 {848,{0,1,4,5},-1},{736,{3,6,7,8},-1},{1040,{0,1,6,7},0},{928,{1,0,9,8},1},
 {656,{2,3,10,11},2},{928,{3,2,6,9},3},{1200,{6,7,0,1},6},{944,{7,9,2,0},7},
 {1616,{8,6,3,2},8},{1152,{9,8,1,3},9},{800,{10,11,2,1},10},{816,{11,10,2,1},11},
 {832,{4,5,0,1},4},{864,{5,4,0,1},5},};
void arcade_attract_reset(arcade_attract_t*a){memset(a,0,sizeof(*a));}
const arcade_attract_state_desc_t*arcade_attract_current(const arcade_attract_t*a){return a&&a->state<ARCADE_ATTRACT_STATE_COUNT?&arcade_attract_states[a->state]:0;}
bool arcade_attract_tick(arcade_attract_t*a,unsigned ticks){if(!a||!ticks)return false;bool changed=false;while(ticks--){const arcade_attract_state_desc_t*d=arcade_attract_current(a);if(!d)return changed;if(++a->tick>=d->duration){a->tick=0;a->state=(uint8_t)((a->state+1)%ARCADE_ATTRACT_STATE_COUNT);changed=true;}}return changed;}
