#ifndef WF_PIN_H
#define WF_PIN_H
#include <stdint.h>
#include "wf_actor.h"
typedef struct wf_ref_count { uint8_t action_state, phase, timer; uint16_t audio_cmd; uint8_t display_acc; } wf_ref_count_t;
void wf_pin_assert_trigger(wf_actor_raw_t *wrestler);
int wf_pin_trigger(const wf_actor_raw_t *wrestler);
void wf_ref_count_enter(wf_ref_count_t *r,uint8_t inherited_phase);
/* one callback tick of exact $20022 timer/phase mechanics; visual $2067C is surfaced as event */
typedef struct wf_ref_event { uint8_t count_event, terminal; uint16_t audio_cmd; } wf_ref_event_t;
wf_ref_event_t wf_ref_count_tick(wf_ref_count_t *r);
int wf_ref_prelock_cancel(const wf_ref_count_t*r,const wf_actor_raw_t*linked);
#endif
