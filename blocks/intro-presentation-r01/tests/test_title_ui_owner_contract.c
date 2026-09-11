#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include "../src/wf_intro_title_ui.h"

typedef struct capture {
    unsigned label_calls;
    unsigned value_calls;
    unsigned prompt_calls;
    unsigned hide_calls;
    uint8_t credit;
    uint8_t prompt;
} capture_t;

static void label(void *u) { ((capture_t *)u)->label_calls++; }
static void value(void *u, uint8_t v) {
    capture_t *c = (capture_t *)u;
    c->value_calls++;
    c->credit = v;
}
static void prompt(void *u, uint8_t p) {
    capture_t *c = (capture_t *)u;
    c->prompt_calls++;
    c->prompt = p;
}
static void hide(void *u) { ((capture_t *)u)->hide_calls++; }

int main(void)
{
    const wf_intro_title_ui_renderer_t r = {
        .draw_credit_label = label,
        .draw_credit_value = value,
        .draw_prompt = prompt,
        .hide_prompt = hide
    };
    capture_t c = {0};

    /* Non-zero credits must be passed through verbatim: no CREDIT:0 default. */
    const wf_intro_title_ui_snapshot_t a = {
        .credit_value = 7,
        .prompt_selector = 0xA5,
        .prompt_visible = true
    };
    assert(wf_intro_title_ui_render(&a, &r, &c));
    assert(c.label_calls == 1);
    assert(c.value_calls == 1);
    assert(c.credit == 7);
    assert(c.prompt_calls == 1);
    assert(c.prompt == 0xA5);
    assert(c.hide_calls == 0);

    /*
     * Prompt selector is independent of credit value. INTRO/TITLE must not
     * derive INSERT COIN (or any other prompt) from the credit count.
     */
    c = (capture_t){0};
    const wf_intro_title_ui_snapshot_t b = {
        .credit_value = 0,
        .prompt_selector = 0x37,
        .prompt_visible = false
    };
    assert(wf_intro_title_ui_render(&b, &r, &c));
    assert(c.credit == 0);
    assert(c.prompt_calls == 0);
    assert(c.hide_calls == 1);

    return 0;
}
