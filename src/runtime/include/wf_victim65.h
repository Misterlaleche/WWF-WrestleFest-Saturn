#ifndef WF_VICTIM65_H
#define WF_VICTIM65_H
#include "wf_actor.h"
typedef enum wf_v65_result { WF_V65_STAY=0, WF_V65_RETYPE=1, WF_V65_EXIT=2 } wf_v65_result_t;
/* First-entry side effect of shared subtype05/2A callback. 2A bypasses the latch. */
void wf_victim65_first_entry(wf_actor_raw_t *a);
/* Applies only transitions whose complete predicate is represented by actor fields here. */
wf_v65_result_t wf_victim65_tick_transition(wf_actor_raw_t *a,uint8_t global_0161);
#endif
