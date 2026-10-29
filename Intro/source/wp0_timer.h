#ifndef WP0_TIMER_H
#define WP0_TIMER_H
#include <stdint.h>
/* Atomic FRCH/FRCL snapshot; preserves incoming SR, including ISR masks. */
uint16_t wp0_timer_count_get(void);
#endif
