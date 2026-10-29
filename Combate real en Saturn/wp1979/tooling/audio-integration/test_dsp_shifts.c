/* Regression for the actual upstream DSP interpreter, compiled twice.
 * Expected SCSP shifter modes come from Sega's SCSP/DSP Assembler manual:
 * 0: x1 saturated, 1: x2 saturated, 2: x2 unsaturated, 3: x1 unsaturated.
 * This is a host unit test; native-disc execution is tested separately.
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#ifndef SCSP_DSP_SOURCE
#error Define SCSP_DSP_SOURCE to the interpreter source under test
#endif
#include SCSP_DSP_SOURCE

u8 *SoundRam;

static uint32_t expected(int32_t acc, unsigned mode)
{
    int64_t value = acc;
    if (mode == 1 || mode == 2) value *= 2;
    if (mode < 2) {
        if (value > 0x7fffff) value = 0x7fffff;
        if (value < -0x800000) value = -0x800000;
    }
    return (uint32_t)value & 0xffffff;
}

int main(void)
{
    static u8 ram[0x80000];
    static const int32_t values[] = {
        0, 1, -1, 0x3fffff, 0x400000, 0x7fffff,
        -0x400000, -0x400001, -0x800000, 0x800000, -0x800001
    };
    unsigned cases = 0, failures = 0, failures_by_mode[4] = {0};
    SoundRam = ram;
    for (unsigned mode = 0; mode < 4; ++mode) {
        for (unsigned n = 0; n < sizeof(values)/sizeof(values[0]); ++n) {
            scsp_dsp_int_init();
            union ScspDspInstruction inst = {0};
            inst.part.shift = mode;
            inst.part.twt = 1;
            inst.part.twa = 0;
            inst.part.zero = 1;
            scsp_dsp.mpro[0] = inst.all;
            scsp_dsp.acc = values[n];
            ScspDspExec(&scsp_dsp, 0, ram);
            ++cases;
            if ((uint32_t)scsp_dsp.temp[0] != expected(values[n], mode)) {
                ++failures;
                ++failures_by_mode[mode];
            }
        }
    }
    printf("{\"cases\":%u,\"failures\":%u,\"failures_by_mode\":[%u,%u,%u,%u]}\n",
        cases, failures, failures_by_mode[0], failures_by_mode[1],
        failures_by_mode[2], failures_by_mode[3]);
    return failures ? 1 : 0;
}
