#include "wf_title_visual_exact.h"
#include <string.h>

const wf_title_card_slot_t wf_title_card_sequence[WF_TITLE_CARD_COUNT] = {
    { WF_TITLE_CARD_HOGAN,      104,  88 },
    { WF_TITLE_CARD_WARRIOR,    168,  88 },
    { WF_TITLE_CARD_HAWK,        40,  88 },
    { WF_TITLE_CARD_ANIMAL,     232,  88 },
    { WF_TITLE_CARD_BOSSMAN,    104,   8 },
    { WF_TITLE_CARD_JAKE,       168, 168 },
    { WF_TITLE_CARD_SLAUGHTER,  168,   8 },
    { WF_TITLE_CARD_DIBIASE,    104, 168 },
    { WF_TITLE_CARD_EARTHQUAKE, 232,   8 },
    { WF_TITLE_CARD_SMASH,       40, 168 },
    { WF_TITLE_CARD_PERFECT,     40,   8 },
    { WF_TITLE_CARD_CRUSH,      232, 168 }
};

static wf_title_visual_event_t
event(wf_title_visual_event_type_t type, const wf_title_visual_state_t *state)
{
    wf_title_visual_event_t e;
    e.type = type;
    e.title_vblank = state != 0 ? state->total_vblank : 0;
    e.phase_vblank = state != 0 ? state->phase_vblank : 0;
    e.card_index = state != 0 ? state->card_index : 0;
    e.card_subphase = state != 0 ? state->card_subphase : 0;
    return e;
}

void
wf_title_visual_init(wf_title_visual_state_t *state)
{
    if (state == 0) {
        return;
    }
    memset(state, 0, sizeof(*state));
    state->phase = WF_TITLE_VISUAL_PREPHASE;
}

wf_title_visual_event_t
wf_title_visual_vblank(wf_title_visual_state_t *state)
{
    if (state == 0) {
        wf_title_visual_state_t dummy;
        memset(&dummy, 0, sizeof(dummy));
        dummy.phase = WF_TITLE_VISUAL_FAULT;
        return event(WF_TITLE_VISUAL_EVENT_FAULT, &dummy);
    }

    if (state->phase == WF_TITLE_VISUAL_DONE) {
        return event(WF_TITLE_VISUAL_EVENT_DONE, state);
    }
    if (state->phase == WF_TITLE_VISUAL_FAULT) {
        return event(WF_TITLE_VISUAL_EVENT_FAULT, state);
    }

    state->total_vblank++;
    state->phase_vblank++;

    if (state->phase == WF_TITLE_VISUAL_PREPHASE) {
        if (state->phase_vblank >= WF_TITLE_PREPHASE_VBLANKS) {
            state->phase = WF_TITLE_VISUAL_CARDS;
            state->phase_vblank = 0;
            state->card_index = 0;
            state->card_subphase = 0;
            state->card_subphase_vblank = 0;
        }
        return event(WF_TITLE_VISUAL_EVENT_BACKGROUND_TICK, state);
    }

    if (state->phase == WF_TITLE_VISUAL_CARDS) {
        wf_title_visual_event_t e = event(WF_TITLE_VISUAL_EVENT_CARD_PHASE, state);
        state->card_subphase_vblank++;

        if (state->card_subphase < WF_TITLE_CARD_SHORT_PHASES) {
            if (state->card_subphase_vblank >= WF_TITLE_CARD_SHORT_VBLANKS) {
                state->card_subphase++;
                state->card_subphase_vblank = 0;
            }
        } else if (state->card_subphase_vblank >= WF_TITLE_CARD_LONG_VBLANKS) {
            state->card_index++;
            state->card_subphase = 0;
            state->card_subphase_vblank = 0;

            if (state->card_index >= WF_TITLE_CARD_COUNT) {
                state->phase = WF_TITLE_VISUAL_MAIN;
                state->phase_vblank = 0;
            }
        }
        return e;
    }

    if (state->phase == WF_TITLE_VISUAL_MAIN) {
        if (state->phase_vblank >= WF_TITLE_MAIN_VBLANKS) {
            state->phase = WF_TITLE_VISUAL_DONE;
            return event(WF_TITLE_VISUAL_EVENT_DONE, state);
        }
        return event(WF_TITLE_VISUAL_EVENT_MAIN_TICK, state);
    }

    state->phase = WF_TITLE_VISUAL_FAULT;
    return event(WF_TITLE_VISUAL_EVENT_FAULT, state);
}
