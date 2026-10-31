#ifndef WP2015_VDP1_OBSERVER_H
#define WP2015_VDP1_OBSERVER_H
#include <stdint.h>
/* Frames, texture-DMA busy samples, command-DMA busy samples,
 * VBlankOut during mutation, mutation active. Read only diagnostics. */
extern volatile uint32_t wp2015_vdp1_observer[5];
void wp2015_vdp1_observe_begin(void);
void wp2015_vdp1_observe_dma(unsigned command);
void wp2015_vdp1_observe_end(void);
void wp2015_vdp1_observe_vblank_out(void);
#endif
