#ifndef WF_ROYAL_H
#define WF_ROYAL_H
#include <stdint.h>
#include "wf_actor.h"
typedef enum { WF_ROYAL_SCAN_CONTINUE=0, WF_ROYAL_SCAN_ZERO=1, WF_ROYAL_SCAN_WINNER=2 } wf_royal_scan_result_t;
typedef struct wf_royal_scan { uint8_t eligible,noneligible; int8_t survivor_slot; } wf_royal_scan_t;
wf_royal_scan_result_t wf_royal_survivor_scan(wf_actor_raw_t actors[WF_ACTOR_SLOTS],wf_royal_scan_t*out,uint16_t cleanup_markers[WF_ACTOR_SLOTS]);
#endif
