#ifndef WF_DOOMSDAY_H
#define WF_DOOMSDAY_H
#include "wf_actor.h"
int wf_doomsday_partner_ready(const wf_actor_raw_t *animal);
void wf_doomsday_launch(wf_actor_raw_t *hawk,int negate_xy);
int wf_doomsday_threshold_ready(const wf_actor_raw_t *hawk);
int wf_doomsday_commit(wf_actor_raw_t *hawk,wf_actor_raw_t *animal,wf_actor_raw_t *victim);
#endif
