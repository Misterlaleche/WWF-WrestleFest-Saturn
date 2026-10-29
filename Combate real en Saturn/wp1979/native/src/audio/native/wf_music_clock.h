#ifndef WF_MUSIC_CLOCK_H
#define WF_MUSIC_CLOCK_H
#include <stdint.h>

/* The DSP counter uses all sixteen bits. A long bus stall may span half of
 * its period, so the elapsed calculation must preserve bit 15. */
static inline uint16_t wf_music_clock_delta(uint16_t *previous,uint16_t sample)
{
 uint16_t delta=(uint16_t)(sample-*previous);
 *previous=sample;
 return delta;
}
static inline int16_t wf_music_clock_due(uint16_t now,uint16_t when)
{
 return (int16_t)(now-when);
}
#endif
