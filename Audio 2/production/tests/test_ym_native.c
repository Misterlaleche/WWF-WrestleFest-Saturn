/* Host checks only. WF_TEST_PINNED_OPM adds an independent phase oracle from
 * the unmodified, SHA-verified A53 dependency; it is never a firmware input. */
#include "wf_scsp_ym.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef WF_TEST_PINNED_OPM
#include "opm.c"
#include "generated/wf_ym_envelope.h"
#endif

static uint16_t hardware[32][16];
static const unsigned signal_order[4] = {0, 2, 1, 3};
static unsigned long trace_writes, trace_keys;
static unsigned long instant_eligible, instant_encoded, instant_unrepresentable;
static double maximum_pitch_error;

static bool write16(uint32_t address, uint16_t value, void *unused)
{
    (void)unused;
    assert(address >= 0x100000 && address < 0x100400 && !(address & 1));
    hardware[(address - 0x100000) / 32][(address % 32) / 2] = value;
    return true;
}

static void check_tl(const wf_scsp_ym_t *ym)
{
    for (unsigned channel = 0; channel < 8; channel++)
        for (unsigned op = 0; op < 4; op++) {
            unsigned slot = channel * 4 + signal_order[op];
            unsigned expected = (ym->reg[0x60 + op * 8 + channel] & 127u) * 2u;
            if ((hardware[slot][6] & 255u) != expected) {
                fprintf(stderr, "TL cache mismatch ch=%u op=%u: hardware=%u expected=%u\n",
                        channel, op, hardware[slot][6] & 255u, expected);
                exit(1);
            }
        }
}

static void reset_and_repeat(void)
{
    wf_scsp_ym_t ym;
    wf_scsp_ym_init(&ym, write16, NULL);
    assert(!ym.fault);
    check_tl(&ym);
    uint32_t before = ym.writes;
    for (unsigned reg = 0x60; reg < 0x80; reg++)
        assert(wf_scsp_ym_write(&ym, (uint8_t)reg, 127));
    assert(ym.writes == before);
    check_tl(&ym);
    for (unsigned mask = 0; mask < 16; mask++) {
        assert(wf_scsp_ym_write(&ym, 8, (uint8_t)(mask << 3)));
        for (unsigned op = 0; op < 4; op++)
            assert(!!(hardware[op][0] & 0x800) == !!(mask & (1u << op)));
    }
}

#ifdef WF_TEST_PINNED_OPM
/* Independent SCSP modulation arithmetic, transcribed from the selected
 * Mednafen 1.32.1 src/ss/scsp.inc:1577-1588 (SHA256
 * 0a1e00647022c86c33edfa1601ae7a9fddde45884e34bf4a92015442e04f9906).
 * Sega SCSP manual, FM modulation control, also specifies MDL0..4 = zero.
 * Return phase within our 128-sample wave, in 1/64-sample units. This checks
 * depth scaling; it does not model the SCSP operator waveform or its delay. */
static unsigned reference_scsp_modulation(int first, int second, unsigned mdl)
{
    uint32_t sum = (uint32_t)(int32_t)(int16_t)first + (uint32_t)(int32_t)(int16_t)second;
    uint32_t offset = ((sum << 6) >> (16 - mdl)) & ~1u;
    if (mdl <= 4) offset = 0;
    return offset & (128u * 64u - 1u);
}

static unsigned reference_opm_modulation(int first, int second, unsigned feedback)
{
    opm_t chip = {0};
    chip.cycles = 14; /* Phase15 slot/channel zero. */
    chip.op_modtable[0] = chip.op_modtable[1] = 1;
    chip.op_m1[0][0] = (int16_t)first;
    chip.op_m1[0][1] = (int16_t)second;
    OPM_OperatorPhase15(&chip);
    chip.op_mod[2] = chip.op_mod[0];
    chip.op_fbshift = feedback ? 8 : 0;
    chip.op_fb[1] = (uint8_t)feedback;
    OPM_OperatorPhase1(&chip);
    /* One YM phase unit is 1/1024 cycle = 8 of our 1/64-sample units. */
    return ((uint32_t)(int32_t)chip.op_mod_in * 8u) & (128u * 64u - 1u);
}

static void check_modulation(unsigned mdl, int first, int second, unsigned feedback, int single)
{
    /* SCSP's nominal 16-bit operator range is four times OPM's 14-bit range.
     * The 128-sample cycle is eight times shorter than OPM's 1024 phase steps.
     * Equal normalized operator inputs isolate routing gain from waveform/EG
     * differences. Exact OPM and SCSP quantizers differ by at most 6/64 sample. */
    unsigned expected = reference_opm_modulation(first, single ? 0 : second, feedback);
    unsigned actual = reference_scsp_modulation(first * 4, (single ? first : second) * 4, mdl);
    unsigned difference = (actual - expected) & (128u * 64u - 1u);
    if (difference > 4096) difference = 8192 - difference;
    if (difference > 6) {
        fprintf(stderr, "Modulation depth mismatch: single=%d feedback=%u MDL=%u inputs=%d,%d SCSP_Q6=%u OPM_Q6=%u difference=%u\n",
                single, feedback, mdl, first, second, actual, expected, difference);
        exit(1);
    }
}

static void modulation_depth_oracle(void)
{
    static const int probes[] = {-8191,-4097,-1025,-17,-1,0,1,17,1025,4097,8191};
    wf_scsp_ym_t ym;
    unsigned cases = 0;
    wf_scsp_ym_init(&ym, write16, NULL);
    /* Algorithm0 C1 has one input; algorithm1 M2 has two distinct inputs. */
    for (unsigned single = 0; single < 2; single++) {
        assert(wf_scsp_ym_write(&ym, 0x20, (uint8_t)(0xc0u | (single ? 0 : 1))));
        unsigned mdl = hardware[single ? 1 : 2][7] >> 12;
        for (unsigned x = 0; x < sizeof probes / sizeof probes[0]; x++)
            for (unsigned y = 0; y < sizeof probes / sizeof probes[0]; y++) {
                check_modulation(mdl, probes[x], probes[y], 0, single);
                cases++;
            }
    }
    /* FB1..3 would require SCSP MDL2..4, which this hardware disables.
     * They are not falsely certified by the representable FB4..7 checks. */
    for (unsigned feedback = 4; feedback <= 7; feedback++) {
        assert(wf_scsp_ym_write(&ym, 0x20, (uint8_t)(0xc0u | (feedback << 3))));
        unsigned mdl = hardware[0][7] >> 12;
        for (unsigned x = 0; x < sizeof probes / sizeof probes[0]; x++)
            for (unsigned y = 0; y < sizeof probes / sizeof probes[0]; y++) {
                check_modulation(mdl, probes[x], probes[y], feedback, 0);
                cases++;
            }
    }
    printf("Pinned OPM / selected Mednafen normalized modulation: %u cases PASS\n", cases);
}

static uint32_t reference_increment(unsigned kc, unsigned kf, unsigned dtmul, unsigned dt2)
{
    opm_t chip = {0};
    int32_t code = OPM_CalcKCode((int32_t)((kc << 6) | (kf >> 2)), 0, 1, dt2);
    chip.pg_fnum[0] = OPM_KCToFNum(code);
    chip.pg_kcode[0] = (uint32_t)code >> 8;
    chip.sl_dt1[0] = dtmul >> 4;
    chip.sl_mul[0] = dtmul & 15;
    OPM_PhaseCalcIncrement(&chip);
    return chip.pg_inc[0];
}

static void exhaustive_phase_oracle(void)
{
    unsigned long count = 0;
    for (unsigned kc = 0; kc < 128; kc++)
        for (unsigned kf = 0; kf < 256; kf += 4)
            for (unsigned dt2 = 0; dt2 < 4; dt2++)
                for (unsigned dtmul = 0; dtmul < 128; dtmul++) {
                    uint32_t expected = reference_increment(kc, kf, dtmul, dt2);
                    uint32_t actual = wf_ym_phase_increment(kc, kf, dtmul, dt2);
                    if (actual != expected) {
                        fprintf(stderr, "Phase mismatch KC=%02x KF=%02x DT1/MUL=%02x DT2=%u: %u != %u\n",
                                kc, kf, dtmul, dt2, actual, expected);
                        exit(1);
                    }
                    if (expected) {
                        uint16_t word = wf_ym_pitch_word(actual);
                        int octave = (word >> 11) & 15;
                        if (octave & 8) octave -= 16;
                        double target = expected * (3579545.0 / (64.0 * 1048576.0));
                        double native = ldexp((44100.0 / 128.0) * (1.0 + (word & 1023) / 1024.0), octave);
                        double cents = fabs(1200.0 * log2(native / target));
                        if (cents > 2.0) {
                            fprintf(stderr, "Board-clock pitch quantization exceeds2cents: KC=%02x KF=%02x DT1/MUL=%02x DT2=%u error=%.9f\n",
                                    kc, kf, dtmul, dt2, cents);
                            exit(1);
                        }
                        if (cents > maximum_pitch_error) maximum_pitch_error = cents;
                    }
                    count++;
                }
    printf("Pinned Nuked-OPM phase increments: %lu exact cases PASS\n", count);
}

static void external_clock_pitch_oracle(void)
{
    /* Original command01 channel1's first carrier: KC55, KF00, DT1/MUL41,
     * DT2=0. WWF board YM2151 is3,579,545Hz; its input divider is2, and
     * Nuked advances each operator once per32 OPM_Clock calls. The A53
     * historical renderer omitted the input divider; it is preserved as
     * historical evidence, never the absolute pitch authority. */
    unsigned inc = reference_increment(0x55, 0, 0x41, 0);
    assert(inc == 13096);
    uint16_t word = wf_ym_pitch_word(inc);
    int octave = (word >> 11) & 15;
    if (octave & 8) octave -= 16;
    double expected = inc * (3579545.0 / (64.0 * 1048576.0));
    double actual = (44100.0 / 128.0) * pow(2.0, octave) * (1.0 + (word & 1023) / 1024.0);
    double cents = fabs(1200.0 * log2(actual / expected));
    if (cents > 2.0) {
        fprintf(stderr, "External YM clock divider mismatch: original KC55 inc=%u expected=%.9fHz native=%.9fHz error=%.9f cents\n",
                inc, expected, actual, cents);
        exit(1);
    }
    printf("Original ROM KC55 /64 board clock: expected %.9fHz native %.9fHz error %.9f cents PASS\n",
           expected, actual, cents);
}

static void instant_attack_oracle(void)
{
    /* Original command01 channel0 M1 patch, at its first actual pitch.
     * This is a real ROM instrument, not a new runtime test instrument. */
    static const unsigned char writes[][2] = {
        {0x20,0xfa},{0x28,0x0e},{0x30,0},{0x40,15},
        {0x80,31},{0xa0,16},{0xc0,192},{0xe0,0xdf}
    };
    wf_scsp_ym_t ym;
    wf_scsp_ym_init(&ym, write16, NULL);
    for (unsigned i = 0; i < sizeof writes / sizeof writes[0]; i++)
        assert(wf_scsp_ym_write(&ym, writes[i][0], writes[i][1]));
    unsigned ar = hardware[0][4] & 31, krs = (hardware[0][5] >> 10) & 15;
    int octave = (hardware[0][8] >> 11) & 15;
    if (octave & 8) octave -= 16;
    int scale = krs == 15 ? 0 : (int)krs + octave;
    if (scale < 0) scale = 0;
    if (scale > 15) scale = 15;
    /* Mednafen1.32.1 key-on equation: AR+native key scale>=32 starts
     * at attenuation0; otherwise it starts at0x280. OPM effective62/63
     * starts at0, so the representable native encoding must do likewise. */
    if (ar + scale < 32) {
        fprintf(stderr, "Original instant-attack mismatch: AR=%u KRS=%u OCT=%d scale=%d; native key-on starts at0x280 instead of0\n",
                ar, krs, octave, scale);
        exit(1);
    }
    puts("Original ROM instant attack uses native zero-attenuation key-on PASS");
}

static unsigned expected_rate(unsigned raw, unsigned scale, const uint8_t *map)
{
    if (!raw) return 0;
    unsigned effective = raw * 2 + scale;
    return map[effective > 63 ? 63 : effective];
}

static void envelope_encoding_oracle(const wf_scsp_ym_t *ym, unsigned ch, unsigned op, int rising_key)
{
    unsigned offset = op * 8 + ch, slot = ch * 4 + signal_order[op];
    unsigned a = ym->reg[0x80 + offset], d1 = ym->reg[0xa0 + offset];
    unsigned d2 = ym->reg[0xc0 + offset], rr = ym->reg[0xe0 + offset];
    unsigned code = OPM_CalcKCode(((ym->reg[0x28 + ch] & 127u) << 6) |
                                 (ym->reg[0x30 + ch] >> 2), 0, 1, d2 >> 6);
    unsigned scale = (code >> 8) >> (3 - (a >> 6));
    unsigned wanted[] = {expected_rate(a & 31, scale, wf_ym_attack_rate),
                         expected_rate(d1 & 31, scale, wf_ym_decay_rate),
                         expected_rate(d2 & 31, scale, wf_ym_decay_rate),
                         expected_rate((rr & 15) * 2 + 1, scale, wf_ym_decay_rate)};
    unsigned raw[] = {hardware[slot][4] & 31, (hardware[slot][4] >> 6) & 31,
                      (hardware[slot][4] >> 11) & 31, hardware[slot][5] & 31};
    unsigned krs = (hardware[slot][5] >> 10) & 15;
    int octave = (hardware[slot][8] >> 11) & 15;
    if (octave & 8) octave -= 16;
    int native_scale = krs == 15 ? 0 : (int)krs + octave;
    if (native_scale < 0) native_scale = 0;
    if (native_scale > 15) native_scale = 15;
    unsigned minimum_scale = octave > 0 ? (unsigned)octave : 1;
    int instant = (a & 31) * 2 + scale >= 62;
    int representable = instant;
    for (unsigned i = 1; i < 4; i++)
        if (wanted[i] && wanted[i] <= minimum_scale) representable = 0;
    for (unsigned i = 0; i < 4; i++) {
        unsigned actual = raw[i] ? raw[i] + native_scale : 0;
        if (i == 0 && representable) {
            if (actual < 32) { fputs("Representable original instant attack was not encoded\n", stderr); exit(1); }
            continue;
        }
        if (actual > 31) actual = 31;
        if (actual != wanted[i]) {
            fprintf(stderr, "Envelope scale changed original target: ch%u op%u stage%u raw%u scale%d actual%u wanted%u\n",
                    ch, op, i, raw[i], native_scale, actual, wanted[i]);
            exit(1);
        }
    }
    if (instant && rising_key) {
        instant_eligible++;
        if (representable) instant_encoded++; else instant_unrepresentable++;
#ifdef WF_TEST_ENVELOPE_DIAGNOSTICS
        if (!representable)
            fprintf(stderr, "INSTANT_FALLBACK ch=%u op=%u KC=%02x KF=%02x DTMUL=%02x AR=%02x D1=%02x D2=%02x RR=%02x native_octave=%d wanted=%u,%u,%u,%u\n",
                    ch, op, ym->reg[0x28+ch], ym->reg[0x30+ch], ym->reg[0x40+offset], a,d1,d2,rr,
                    octave,wanted[0],wanted[1],wanted[2],wanted[3]);
#endif
    }
}
#endif

static void replay(const char *path)
{
    FILE *stream = fopen(path, "r");
    if (!stream) { perror(path); exit(2); }
    wf_scsp_ym_t ym;
    wf_scsp_ym_init(&ym, write16, NULL);
    char line[256], kind[8];
    unsigned long long cycle;
    unsigned reg, value;
    while (fgets(line, sizeof line, stream)) {
        if (sscanf(line, "%llu,%7[^,],%u,%u", &cycle, kind, &reg, &value) != 4 ||
            strcmp(kind, "YM_D")) continue;
        unsigned previous_key = reg == 8 ? ym.key[value & 7] : 0;
        if (!wf_scsp_ym_write(&ym, (uint8_t)reg, (uint8_t)value)) {
            fprintf(stderr, "%s:%llu unsupported/fault register %02x=%02x\n", path, cycle, reg, value);
            exit(1);
        }
        trace_writes++;
        check_tl(&ym);
        if (reg == 8 && (value & 0x78)) {
            trace_keys++;
            unsigned channel = value & 7;
            for (unsigned op = 0; op < 4; op++) {
#ifdef WF_TEST_PINNED_OPM
                envelope_encoding_oracle(&ym, channel, op,
                    (((value >> 3) & ~previous_key) & (1u << signal_order[op])) != 0);
#endif
                unsigned offset = op * 8 + channel;
                uint32_t inc = wf_ym_phase_increment(ym.reg[0x28 + channel],
                    ym.reg[0x30 + channel], ym.reg[0x40 + offset], ym.reg[0xc0 + offset] >> 6);
                if (!inc) continue;
                uint16_t pitch = hardware[channel * 4 + signal_order[op]][8];
                int octave = (pitch >> 11) & 15;
                if (octave & 8) octave -= 16;
                double target = inc * (3579545.0 / (64.0 * 1048576.0));
                double actual = (44100.0 / 128.0) * pow(2.0, octave) * (1.0 + (pitch & 1023) / 1024.0);
                double cents = fabs(1200.0 * log2(actual / target));
                if (cents > maximum_pitch_error) maximum_pitch_error = cents;
            }
        }
    }
    fclose(stream);
    assert(!ym.fault);
}

int main(int argc, char **argv)
{
    reset_and_repeat();
#ifdef WF_TEST_PINNED_OPM
    modulation_depth_oracle();
    external_clock_pitch_oracle();
    instant_attack_oracle();
    exhaustive_phase_oracle();
#endif
    for (int i = 1; i < argc; i++) replay(argv[i]);
    printf("TL reset/idempotence and partial key masks PASS; %lu ROM YM writes, %lu key-ons; max static pitch error %.9f cents\n",
           trace_writes, trace_keys, maximum_pitch_error);
    printf("Original rising-key envelope encodings: %lu instant-eligible operators; %lu encoded; %lu constrained by other native rates\n",
           instant_eligible, instant_encoded, instant_unrepresentable);
    return 0;
}
