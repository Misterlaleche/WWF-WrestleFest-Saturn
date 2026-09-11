#include <assert.h>
#include "../src/wf_title_visual_exact.h"

int main(void)
{
    wf_title_visual_state_t s;
    wf_title_visual_init(&s);

    unsigned background = 0;
    unsigned card = 0;
    unsigned main = 0;
    unsigned done = 0;

    for (;;) {
        wf_title_visual_event_t e = wf_title_visual_vblank(&s);
        if (e.type == WF_TITLE_VISUAL_EVENT_BACKGROUND_TICK) background++;
        else if (e.type == WF_TITLE_VISUAL_EVENT_CARD_PHASE) card++;
        else if (e.type == WF_TITLE_VISUAL_EVENT_MAIN_TICK) main++;
        else if (e.type == WF_TITLE_VISUAL_EVENT_DONE) { done++; break; }
        else assert(e.type != WF_TITLE_VISUAL_EVENT_FAULT);
    }

    assert(background == 128);
    assert(card == 12 * 42);
    assert(main == 1479);
    assert(done == 1);
    assert(s.total_vblank == 2112);

    assert(wf_title_card_sequence[0].card == WF_TITLE_CARD_HOGAN);
    assert(wf_title_card_sequence[1].card == WF_TITLE_CARD_WARRIOR);
    assert(wf_title_card_sequence[2].card == WF_TITLE_CARD_HAWK);
    assert(wf_title_card_sequence[11].card == WF_TITLE_CARD_CRUSH);

    assert(wf_title_card_sequence[0].x == 104);
    assert(wf_title_card_sequence[0].y == 88);
    assert(wf_title_card_sequence[11].x == 232);
    assert(wf_title_card_sequence[11].y == 168);
    return 0;
}
