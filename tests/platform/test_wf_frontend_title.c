#include <assert.h>
#include <string.h>

#include "../../src/runtime/wf_frontend_title.h"

static void
assert_view_does_not_mutate_credit_state(uint16_t config,
                                         wf_credit_state_t state,
                                         bool second_phase,
                                         uint8_t expected_prompt)
{
    wf_credit_state_t before = state;
    wf_frontend_title_view_t view;

    assert(wf_frontend_title_view_build(&state, config, 0x0040u, second_phase, &view));
    assert(memcmp(&before, &state, sizeof(state)) == 0);
    assert(view.credits == state.credits);
    assert(view.partial == state.partial);
    assert(view.prompt_id == expected_prompt);
    assert(view.prompt_blink_phase);
    assert(view.dedicated_2p_second_prompt_phase == second_phase);
}

int
main(void)
{
    wf_credit_state_t state;

    memset(&state, 0, sizeof(state));
    state.coin_threshold = 1u;
    state.credit_award = 1u;
    assert_view_does_not_mutate_credit_state(0u, state, false, 0x02u);

    state.credits = 1u;
    assert_view_does_not_mutate_credit_state(0u, state, false, 0x03u);

    state.credits = 1u;
    assert_view_does_not_mutate_credit_state(WF_OPERATOR_CONFIG_PLAYERS_2P,
                                             state,
                                             false,
                                             0x55u);

    state.credits = 2u;
    assert_view_does_not_mutate_credit_state(WF_OPERATOR_CONFIG_PLAYERS_2P,
                                             state,
                                             true,
                                             0x56u);

    assert(wf_frontend_title_credit_visuals_are_frontend_owned());
    assert(!wf_frontend_title_mutates_credit_state());
    assert(!wf_frontend_title_view_build(0, 0u, 0u, false, &(wf_frontend_title_view_t){0}));
    assert(!wf_frontend_title_view_build(&state, 0u, 0u, false, 0));

    return 0;
}
