#ifndef WF_RINGOUT_H
#define WF_RINGOUT_H
#include <stdint.h>
#include "wf_actor.h"
typedef struct { uint8_t subcount; uint8_t count; } wf_outside_count_t;
void wf_ringout_toss_pair(wf_actor_raw_t *victim, wf_actor_raw_t *aggressor);
void wf_ringexit_clear_inside(wf_actor_raw_t *a);
void wf_ringentry_set_inside(wf_actor_raw_t *a);
void wf_outside_count_reset(wf_outside_count_t *c);
/* one referee update; returns 1 only when terminal count 20 is reached */
int wf_outside_count_tick(wf_outside_count_t *c);
#endif
