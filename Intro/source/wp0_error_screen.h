#ifndef WP0_ERROR_SCREEN_H
#define WP0_ERROR_SCREEN_H
#include <stdint.h>
/* CODE,AUDIO,FRAME,MISSED,CPU_MAX,DRAW_MAX,REGION,GAP_US,
 * QUEUED,PLAYED,BLOCK,FLAGS. Called after the audio has stopped.
 * This is a fatal diagnostic screen; no allocations or original assets. */
void wp0_error_screen(const uint32_t values[12]);
#endif
