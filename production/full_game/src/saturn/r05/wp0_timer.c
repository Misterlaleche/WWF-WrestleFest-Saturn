/* Derived from supplied session14 R05 reference/source; original retained read-only.
 * Silent fixed-attract integration, 2026-11-02. See PROVENANCE.md. */
#include "wp0_timer.h"
#include <stddef.h>

/* SH7604 manual 11.3: FRCH latches FRCL in shared TEMP. A maskable ISR
 * must not read FRC/FICR or write FRC/OCR between these two byte loads.
 * One asm block and its memory clobber keep both MMIO reads inside the
 * interrupt mask and keep surrounding memory operations on their side.
 * Restore the full incoming SR, including an ISR caller's I bits.
 *
 * NMI cannot be masked by SR.I. R03's FRT-reading playback callbacks are
 * maskable SCU interrupts; it installs no FRT-reading NMI handler. A future
 * NMI handler must not access the FRT TEMP registers concurrently.
 */
uint16_t
wp0_timer_count_get(void)
{
    uint32_t saved_sr;
    uint32_t masked_sr = 0xF0U;
    uint32_t high;
    uint32_t low;
    uintptr_t address = 0xFFFFFE12U;

    __asm__ volatile (
        "stc sr,%[saved]\n\t"
        "or %[saved],%[masked]\n\t"
        "ldc %[masked],sr\n\t"
        "mov.b @%[address]+,%[high]\n\t"
        "mov.b @%[address],%[low]\n\t"
        "ldc %[saved],sr"
        : [saved] "=&r" (saved_sr),
          [masked] "+&r" (masked_sr),
          [address] "+&r" (address),
          [high] "=&r" (high),
          [low] "=&r" (low)
        :
        : "memory"
    );

    return (uint16_t)(((uint16_t)(uint8_t)high << 8) | (uint8_t)low);
}
