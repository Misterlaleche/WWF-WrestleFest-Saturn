#include "wf_intro_title_ui.h"

bool
wf_intro_title_ui_render(const wf_intro_title_ui_snapshot_t *snapshot,
                         const wf_intro_title_ui_renderer_t *renderer,
                         void *user)
{
    if ((snapshot == 0) || (renderer == 0) ||
        (renderer->draw_credit_label == 0) ||
        (renderer->draw_credit_value == 0) ||
        (renderer->draw_prompt == 0) ||
        (renderer->hide_prompt == 0)) {
        return false;
    }

    renderer->draw_credit_label(user);
    renderer->draw_credit_value(user, snapshot->credit_value);

    if (snapshot->prompt_visible) {
        renderer->draw_prompt(user, snapshot->prompt_selector);
    } else {
        renderer->hide_prompt(user);
    }

    return true;
}
