#ifndef WF_MATCH_RULES_H
#define WF_MATCH_RULES_H
#include <stdint.h>
#include "wf_actor.h"
typedef enum { WF_MATCH_NONE=0, WF_MATCH_TAG_WIN=1, WF_MATCH_TAG_LOSS=2, WF_MATCH_NO_WINNER=3, WF_MATCH_ROYAL_CONTINUE=4 } wf_match_result_t;
int wf_tag_has_terminal_marker(const wf_actor_raw_t *a0,const wf_actor_raw_t *a2);
void wf_tag_countout_roles(wf_actor_raw_t *side_a,wf_actor_raw_t *side_b,int outside_a,int outside_b);
void wf_tag_result_markers_from_roles(wf_actor_raw_t *side_a,wf_actor_raw_t *side_b);
void wf_royal_pin_credit(wf_actor_raw_t *pinned);
#endif
