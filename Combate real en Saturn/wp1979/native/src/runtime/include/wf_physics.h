#ifndef WF_PHYSICS_H
#define WF_PHYSICS_H
#include <stdint.h>
#include "wf_actor.h"
extern const uint16_t wf_qx[65],wf_qy[65];
extern const uint8_t wf_walk_speed[12],wf_walk_alt_speed[12],wf_run_speed[12];
void wf_physics_integrate_common(wf_actor_raw_t *a);
uint8_t wf_physics_walk_speed(uint8_t character,uint8_t flags32);
uint8_t wf_physics_run_speed(uint8_t character);
#endif
