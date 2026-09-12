#include "wf_frontend_title.h"

#include <string.h>

bool
wf_frontend_title_view_build(const wf_credit_state_t *credit_state,
                             uint16_t config,
                             uint16_t prompt_counter,
                             bool dedicated_2p_second_prompt_phase,
                             wf_frontend_title_view_t *out_view)
{
    if (credit_state == 0 || out_view == 0) {
        return false;
    }

    memset(out_view, 0, sizeof(*out_view));
    out_view->credits = credit_state->credits;
    out_view->partial = credit_state->partial;
    out_view->prompt_id = wf_title_prompt_id(credit_state,
                                             config,
                                             dedicated_2p_second_prompt_phase);
    out_view->prompt_blink_phase = wf_title_prompt_blink_phase(prompt_counter);
    out_view->dedicated_2p_second_prompt_phase = dedicated_2p_second_prompt_phase;
    return true;
}

bool
wf_frontend_title_credit_visuals_are_frontend_owned(void)
{
    return true;
}

bool
wf_frontend_title_mutates_credit_state(void)
{
    return false;
}
