#ifndef WF_FRONTEND_TITLE_H
#define WF_FRONTEND_TITLE_H

#include <stdbool.h>
#include <stdint.h>

#include "wf_operator_credits.h"

typedef struct wf_frontend_title_view {
    uint8_t credits;
    uint8_t partial;
    uint8_t prompt_id;
    bool prompt_blink_phase;
    bool dedicated_2p_second_prompt_phase;
} wf_frontend_title_view_t;

bool wf_frontend_title_view_build(const wf_credit_state_t *credit_state,
                                  uint16_t config,
                                  uint16_t prompt_counter,
                                  bool dedicated_2p_second_prompt_phase,
                                  wf_frontend_title_view_t *out_view);

bool wf_frontend_title_credit_visuals_are_frontend_owned(void);
bool wf_frontend_title_mutates_credit_state(void);

#endif
