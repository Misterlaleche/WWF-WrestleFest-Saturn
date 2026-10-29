#ifndef WF_R18_FRONTEND_FADE_H
#define WF_R18_FRONTEND_FADE_H
#include "wf_runtime.h"

/* Pure ROM $26642: one compact palette of 16 banks x 16 colors.
 * Values are host-order words packed $0RGB, with four bits per channel.
 * The source's high nibble is ignored. Phase 0 is black; phase 7 retains
 * the three channel nibbles. Intermediate phases use ROM $266B2 verbatim.
 * Exact in-place operation is supported; other overlapping ranges are not.
 * NULL arguments or phase > 7 return WF_BAD_STATE without any writes.
 * This function does not advance a fade controller or upload video data. */
wf_status_t wf_r18_frontend_fade_palette_26642(const uint16_t source[256],
                                             uint16_t output[256],
                                             unsigned phase);
#endif
