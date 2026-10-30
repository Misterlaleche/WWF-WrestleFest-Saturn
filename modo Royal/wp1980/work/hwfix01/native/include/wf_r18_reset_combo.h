#ifndef WF_R18_RESET_COMBO_H
#define WF_R18_RESET_COMBO_H

#include <stdint.h>

typedef struct wf_r18_reset_combo {
    uint8_t armed;
} wf_r18_reset_combo_t;

enum {
    WF_R18_RESET_NONE = 0,
    WF_R18_RESET_TITLE = 1,
    WF_R18_RESET_BIOS = 2
};

/* A release must separate the title and BIOS requests. Poll the same
 * physical port in every scene; the caller decides how to route an event. */
static inline unsigned
wf_r18_reset_combo_poll(wf_r18_reset_combo_t *combo, unsigned held,
                        unsigned on_title)
{
    if (!combo)
        return WF_R18_RESET_NONE;
    if (!held) {
        combo->armed = 1;
        return WF_R18_RESET_NONE;
    }
    if (!combo->armed)
        return WF_R18_RESET_NONE;
    combo->armed = 0;
    return on_title ? WF_R18_RESET_BIOS : WF_R18_RESET_TITLE;
}

#endif
