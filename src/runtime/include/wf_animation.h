#ifndef WF_ANIMATION_H
#define WF_ANIMATION_H
#include <stdint.h>
#include "wf_actor.h"

typedef enum { WF_ANIM_MAIN=0, WF_ANIM_ACTION=1, WF_ANIM_STATE4=2 } wf_anim_domain_t;
typedef struct { uint16_t pose_raw; uint16_t counter_raw; } wf_anim_entry_t;
typedef struct { uint32_t descriptor_rom; uint32_t callback_rom; uint8_t kind; uint8_t entry_count; uint16_t entry_start; uint16_t stored_entries; } wf_anim_desc_t;
typedef struct { uint16_t desc_index; uint16_t pose_raw; uint8_t pose_flag15; uint8_t terminal; } wf_anim_view_t;
extern const wf_anim_entry_t wf_anim_entries[];
extern const unsigned wf_anim_entry_count;
extern const wf_anim_desc_t wf_anim_descs[];
extern const unsigned wf_anim_desc_count;
extern const uint16_t wf_anim_main_dispatch[13];
extern const uint16_t wf_anim_action_dispatch[143];
extern const uint16_t wf_anim_state4_dispatch[43];

int wf_anim_select(wf_actor_raw_t *a,wf_anim_domain_t dom,uint8_t id,wf_anim_view_t *v);
int wf_anim_tick(wf_actor_raw_t *a,wf_anim_view_t *v);
uint16_t wf_anim_current_pose(const wf_actor_raw_t *a,const wf_anim_view_t *v);
#endif
