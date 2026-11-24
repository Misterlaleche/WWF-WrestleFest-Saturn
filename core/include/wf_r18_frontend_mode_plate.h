#ifndef WF_R18_FRONTEND_MODE_PLATE_H
#define WF_R18_FRONTEND_MODE_PLATE_H
#include "wf_r18_title.h"
typedef struct wf_r18_frontend_mode_plate { uint32_t pc,blocked_dependency; } wf_r18_frontend_mode_plate_t;
/* Exact GAME SELECT setup$52BE..$5339, stopping before$533A mode logic.
 * Reuses title RAM16KiB/D/A/CALL ABI. Mandatory helpers:2A06,2052,26E66,1F9E.
 * Keeps predecessor mode, credits and RNG; clears only original frameLONG80
 * and sets original flags/graphics positions. No title/selector input policy.
 * Returns TITLE_MORE0, HANDOFF2, BLOCKED-1 or BAD_CONTEXT-2. Missing/failed
 * effects block; CCR/A7 temporary stack are outside this ABI as for title.
 */
void wf_r18_frontend_mode_plate_init(wf_r18_frontend_mode_plate_t *state);
int wf_r18_frontend_mode_plate_step(wf_r18_frontend_mode_plate_t *state,
    wf_r18_title_context_t *context);
#endif
