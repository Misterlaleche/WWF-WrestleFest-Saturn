#ifndef WF_INTRO_TITLE_UI_H
#define WF_INTRO_TITLE_UI_H

#include <stdbool.h>
#include <stdint.h>

/*
 * TITLE credit/prompt display contract.
 *
 * Ownership rule:
 *   WP19/master owns credit accounting and prompt selection.
 *   INTRO/TITLE is a read-only consumer of this snapshot.
 *
 * This module MUST NOT increment/decrement credits, infer coinage,
 * select INSERT COIN from credit_value, or manufacture a local default.
 */
typedef struct wf_intro_title_ui_snapshot {
    uint8_t credit_value;
    uint8_t prompt_selector;
    bool prompt_visible;
} wf_intro_title_ui_snapshot_t;

typedef struct wf_intro_title_ui_renderer {
    void (*draw_credit_label)(void *user);
    void (*draw_credit_value)(void *user, uint8_t credit_value);
    void (*draw_prompt)(void *user, uint8_t prompt_selector);
    void (*hide_prompt)(void *user);
} wf_intro_title_ui_renderer_t;

/*
 * Emits only visual operations. The snapshot is never modified.
 * Coordinates, glyph/tile IDs, palette and layer priority remain renderer
 * data owned by the ROM-derived WP25/WP26 title-text implementation.
 */
bool wf_intro_title_ui_render(const wf_intro_title_ui_snapshot_t *snapshot,
                              const wf_intro_title_ui_renderer_t *renderer,
                              void *user);

#endif
