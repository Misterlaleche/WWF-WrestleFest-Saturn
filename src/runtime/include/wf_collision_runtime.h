#ifndef WF_COLLISION_RUNTIME_H
#define WF_COLLISION_RUNTIME_H
#include <stdint.h>
#include "wf_collision.h"
typedef struct wf_box4 { int8_t x0,z0,x1,z1; } wf_box4_t;
extern const wf_box4_t wf_source_boxes[11], wf_target_boxes[11];
int wf_collision_axis06_overlap(const wf_actor_raw_t*s,const wf_box4_t*sb,const wf_actor_raw_t*t,const wf_box4_t*tb);
int wf_collision_axis0e_overlap(const wf_actor_raw_t*s,const wf_box4_t*sb,const wf_actor_raw_t*t,const wf_box4_t*tb);
/* Generic success tail; slot ROM bases preserve the arcade reciprocal-link representation. */
void wf_collision_success_tail(wf_actor_raw_t*src,unsigned src_slot,wf_actor_raw_t*dst,unsigned dst_slot);
/* profile damage payload is BYTE write to +$69, preserving +$68 high byte */
void wf_collision_write_profile_damage(wf_actor_raw_t*dst,uint8_t amount);
#endif
