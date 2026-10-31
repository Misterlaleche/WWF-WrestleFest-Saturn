#include "wp2015_vdp1_observer.h"
volatile uint32_t wp2015_vdp1_observer[5];
void wp2015_vdp1_observe_begin(void) {
    ++wp2015_vdp1_observer[0];
    wp2015_vdp1_observer[4] = 1;
}
void wp2015_vdp1_observe_dma(unsigned command) {
    /* EDSR.CEF is set when the current drawing operation reaches END.
     * This is a read of the original VDP1 register, without a wait. */
    if (!(*(volatile uint16_t *)0x25d00010u & 2u))
        ++wp2015_vdp1_observer[1u + command];
}
void wp2015_vdp1_observe_end(void) {
    wp2015_vdp1_observer[4] = 0;
}
void wp2015_vdp1_observe_vblank_out(void) {
    if (wp2015_vdp1_observer[4]) ++wp2015_vdp1_observer[3];
}
