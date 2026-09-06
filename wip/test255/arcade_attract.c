#include "arcade_attract.h"
#include <string.h>
const arcade_attract_state_desc_t arcade_attract_states[ARCADE_ATTRACT_STATE_COUNT]={
 {848,-1,ATTRACT_KIND_TAG_DEMO},{736,-1,ATTRACT_KIND_DEMOLITION_A},{1040,0,ATTRACT_KIND_WRESTLER_DEMO},{928,1,ATTRACT_KIND_WRESTLER_DEMO},{656,2,ATTRACT_KIND_WRESTLER_DEMO},{928,3,ATTRACT_KIND_WRESTLER_DEMO},{1200,6,ATTRACT_KIND_WRESTLER_DEMO},{944,7,ATTRACT_KIND_WRESTLER_DEMO},{1616,8,ATTRACT_KIND_WRESTLER_DEMO},{1152,9,ATTRACT_KIND_WRESTLER_DEMO},{800,10,ATTRACT_KIND_WRESTLER_DEMO},{816,11,ATTRACT_KIND_WRESTLER_DEMO},{832,4,ATTRACT_KIND_WRESTLER_DEMO},{864,5,ATTRACT_KIND_WRESTLER_DEMO},};
void arcade_attract_reset(arcade_attract_t*a){memset(a,0,sizeof(*a));}
const arcade_attract_state_desc_t*arcade_attract_current(const arcade_attract_t*a){return a&&a->state<ARCADE_ATTRACT_STATE_COUNT?&arcade_attract_states[a->state]:0;}
bool arcade_attract_tick(arcade_attract_t*a,unsigned ticks){if(!a||!ticks)return false;bool changed=false;while(ticks--){const arcade_attract_state_desc_t*d=arcade_attract_current(a);if(!d)return changed;if(++a->tick>=d->duration){a->tick=0;a->state=(uint8_t)((a->state+1)%ARCADE_ATTRACT_STATE_COUNT);changed=true;}}return changed;}
