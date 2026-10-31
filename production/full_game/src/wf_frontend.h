#ifndef WF_FRONTEND_H
#define WF_FRONTEND_H
#include <stdbool.h>
#include <stdint.h>
/* Step adapter of approved TEST03. Slots remain visual slots, NOT wrestler IDs.
 * Loading and its17 physical-field entry hold belong to the platform owner. */
typedef enum {WF_FRONTEND_MODE=1,WF_FRONTEND_LOAD_ROSTER,WF_FRONTEND_ROSTER,
 WF_FRONTEND_SCROLL,WF_FRONTEND_DONE,WF_FRONTEND_ERROR} wf_frontend_state_t;
typedef struct {
 wf_frontend_state_t state;
 uint32_t slot,page,scroll,phase_index,dwell,scroll_left,previous_direction;
 bool royal,to_right;
} wf_frontend_t;
typedef struct {
 bool palette,family,cursor,scroll,confirm;
 unsigned phase,old_slot;
} wf_frontend_effect_t;
void wf_frontend_init(wf_frontend_t *f);
void wf_frontend_roster_ready(wf_frontend_t *f);
bool wf_frontend_tick(wf_frontend_t *f,unsigned direction,bool confirm,wf_frontend_effect_t *effect);
#endif
