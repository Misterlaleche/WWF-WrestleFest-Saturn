/* Host-only rate measurement of the SHA-pinned, unmodified Nuked-OPM core.
 * The included dependency is LGPL-2.1-or-later; never link this into Saturn.
 * These are envelope unit measurements, not synthetic runtime instruments. */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "opm.c"

static void envelope_clock(opm_t *chip)
{
    /* Same order as OPM_Clock; only envelope units are exercised. Their
     * register inputs and key scale are held constant for this unit test. */
    OPM_EnvelopeTimer(chip);
    OPM_EnvelopePhase6(chip);
    OPM_EnvelopePhase5(chip);
    OPM_EnvelopePhase4(chip);
    OPM_EnvelopePhase3(chip);
    OPM_EnvelopePhase2(chip);
    OPM_EnvelopePhase1(chip);
    OPM_EnvelopeClock(chip);
    chip->cycles = (chip->cycles + 1) & 31;
}

static uint64_t measure(unsigned effective, int attack)
{
    opm_t chip = {0};
    for (unsigned slot = 0; slot < 32; slot++) {
        chip.eg_level[slot] = attack ? 1023 : 0;
        chip.eg_state[slot] = attack ? eg_num_release : eg_num_sustain;
        chip.mode_kon[slot] = 1;
        chip.kon[slot] = chip.kon2[slot] = !attack;
        chip.sl_ar[slot] = chip.sl_d2r[slot] = effective >> 1;
        chip.sl_ks[slot] = 3;
        chip.pg_kcode[slot] = effective & 1;
    }
    for (uint64_t cycles = 1; cycles <= 100000000; cycles++) {
        envelope_clock(&chip);
        if (attack ? chip.eg_level[0] <= 9 : chip.eg_level[0] >= 128)
            return cycles;
    }
    return 0;
}

int main(void)
{
    puts("effective,attack_core_clocks,decay_core_clocks");
    for (unsigned rate = 2; rate < 64; rate++) {
        uint64_t attack = measure(rate, 1), decay = measure(rate, 0);
        if (!attack || !decay) return 1;
        printf("%u,%llu,%llu\n", rate, (unsigned long long)attack,
               (unsigned long long)decay);
        fflush(stdout);
    }
    return 0;
}
