#ifndef WF_R18_FRONTEND_RESET_H
#define WF_R18_FRONTEND_RESET_H
#include "wf_r18_frontend_irq.h"
typedef struct wf_r18_frontend_reset { uint32_t pc,blocked_dependency; } wf_r18_frontend_reset_t;
/* Hardware/bootstrap $654..$6FC, distinct from frontend re-entry$6FC.
 * Initial supervisor SR/SSP are explicit platform inputs. The reset does
 * not seed RNG$5C: clear$1F1E starts at$60. Native $1E1E reads physical
 * DIP words$140020..26 and packs their exact active-low configuration;
 * native $400 sets coinage/clears credits and $6D82 copies factory records.
 * CALL dependencies $1FC0 and $2006 remain external native owners, including
 * SR effects. Raw board writes are WRITE16 events. Stop before$6FC; pass
 * the same live RAM/D/A to the BOOT owner. Stack scratch convention and
 * callback failure contract are identical to wf_r18_frontend_irq.
 */
void wf_r18_frontend_reset_init(wf_r18_frontend_reset_t *state);
int wf_r18_frontend_reset_step(wf_r18_frontend_reset_t *state,
    wf_r18_frontend_control_context_t *context);
int wf_r18_frontend_reset_clear_1f1e(wf_r18_frontend_control_context_t *context);
int wf_r18_frontend_reset_dips_1e1e(wf_r18_frontend_control_context_t *context);
int wf_r18_frontend_reset_coinage_0400(wf_r18_frontend_control_context_t *context);
int wf_r18_frontend_reset_rankings_6d82(wf_r18_frontend_control_context_t *context);
#endif
