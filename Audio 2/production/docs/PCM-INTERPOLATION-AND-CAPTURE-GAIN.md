# One-command PCM interpolation and capture-level experiment

**The native linear interpolation changes the spectrum of original command
`0x21`. A separate, fixed Mednafen output factor explains most of its remaining
raw level difference.** These conclusions come from an isolated native A/B
test and the exact selected emulator source. The ZOH variant remains private;
the production DSP and all frozen acceptance matrices retain linear interpolation.

## What changed, and what was held fixed

The baseline is firmware
`cddd1b01a4e277a8d8f82f16445dbc81dd8e50ab5a13c133fdeb25ef8353c090`.
The private zero-order-hold variant is
`4bd228d923d05b8c264d5e11c3ef1fe8f70c95bd3b935c848faa86653f5ad10a`.
Both ELF files are 53,612 bytes with identical symbols and segment layouts.
Only **89 bytes differ**, all inside the same 1,024-byte DSP program array.
Every other byte of the two firmware files is identical.

For each of the four PCM voices, the existing program reads the current and
next original sample and computes a linear weighted sum. The private variant
retains the current sample until the next original sample interval. It replaces
the unused second read and its arithmetic with pipeline NOPs. Program length
remains 120 active steps; RAM operations per SCSP frame fall from 12 to 8.
That operation count is not a measured throughput improvement under game load.

The integer arithmetic check covers all 4,096 fractional positions, all nine
nonzero original volume levels and silence, using eight signed boundary pairs:
327,680 cases. It includes SCSP coefficient truncation, intermediate rounding
and saturation. The original program fails ZOH equality in 257,964 cases;
both programs agree with their separate integer equations in every case.

Both firmware variants then run original command `0x21` in the selected,
unmodified standalone Mednafen 1.32.1. Actual F5 states independently confirm:

- Each loaded DSP program agrees with the program in its own ELF.
- All 64 native coefficients, slot registers and DSP address registers agree
  between runs; master volume is 15 with multiplier 256.
- All four final PCM ring buffers are byte-identical.
- The native phase/clock constants are unchanged.
- Both runs pass their runtime gates with stable source, inputs and emulator.

The existing native PCM phase increment is `743/4096` per 44.1 kHz frame,
giving **7,999.5849609375 Hz**, a finite representation of the nominal 8 kHz.
The A/B preserves that rate. It does not secretly correct it or change sample
bytes, volume, FM routing, output mixing, clock or the original SFX command.

## Actual unmodified capture metrics

Each native run covers five seconds after the command. The listening/metric
pair uses the complete three-second original full-board reference window.
Native files remain stereo; the reference is mono. Metrics use the arithmetic
average of the native channels. There is no gain adjustment, resampling, time
stretching or onset fitting. DAC phase has not been independently calibrated.

| Measurement | Linear baseline | Private ZOH |
| --- | ---: | ---: |
| Native minus reference RMS | -1.569672 dB | -1.459966 dB |
| Native spectral centroid | 107.536577 Hz | 145.253586 Hz |
| Reference spectral centroid | 141.646871 Hz | 141.646871 Hz |
| Log-power spectral RMSE, 40 Hz–16 kHz | 17.082322 dB | 2.808335 dB |
| 10 ms envelope correlation | 0.998694 | 0.999239 |
| Raw waveform correlation | 0.653096 | 0.616346 |
| Clipped samples | 0 | 0 |

For this one original command, removing interpolation raises RMS by only
**0.109706 dB**. It therefore does not explain the majority of the observed
level deficit. The spectrum changes substantially in the expected direction
for a held original digital sample. Raw waveform correlation does not improve;
phase, launch timing and remaining quantization are not calibrated away.
No result here establishes complete waveform equivalence or all-98 behavior.

## A fixed output factor in the selected emulator

The selected upstream `src/ss/sound.cpp`, `RunSCSP()`, calls `SCSP.RunSample()`
and then applies the following expression to each output channel:

```cpp
bp[0] = (bp[0] * 27 + 16) >> 5;
bp[1] = (bp[1] * 27 + 16) >> 5;
```

The multiplicative factor is `27/32 = 0.84375`, or **-1.475724283 dB**,
with integer rounding. This is part of the unmodified emulator's digital
output path. It is not a property established for the original arcade PCB or
the physical Saturn analog output.

The private ZOH run's raw -1.459966 dB difference is **+0.015758 dB** relative
to that analytic factor. That subtraction is a separate diagnostic calculation;
no delivered WAV is scaled and the raw comparison numbers above remain intact.
Coefficient quantization and different held-sample phases can still affect
the remaining small discrepancy. The measured spectral result and this output
factor explain different parts of the comparison.

The source came from the pinned official Mednafen 1.32.1 source archive, SHA256
`de7eb94ab66212ae7758376524368a8ab208234b33796625ca630547dbc83832`.
Complete file identities are:

| Source | SHA256 |
| --- | --- |
| `src/ss/sound.cpp` | `2e83769d98a737a9e277aab7a42c6b14686aa0eb5df84e16490ed4bc195f0bbb` |
| `src/ss/scsp.inc` | `0a1e00647022c86c33edfa1601ae7a9fddde45884e34bf4a92015442e04f9906` |
| Selected executable | `a12628d798edf9ad1ec7497cec666ee64b583a09fc48a0c650702c8aeb3a7689` |

The archive provenance is recorded by the reproduced environment; source
identities are checked again by the experiment analyzer. The source-only
evidence in `tools/reference/evidence/dsp-zoh-causal.json` contains the raw
metrics, native-state gates and hashes. It contains no ROM bytes or audio.

## Reproduce the private A/B

After regenerating resources and full-board SFX references from the user's
original ROM and sourcing the reproduced environment, run from `production`:

```sh
python3 tools/reference/run_dsp_zoh_probe.py \
  --source . \
  --resources /private-output/full-board/inputs \
  --reference-report /private-output/full-board-sfx-wav/reference-sfx-report.json \
  --sdk /environment/sh2eb-elf --environment-root /environment \
  --bios /private-input/mpr-17933.bin \
  --runtime-source /environment/src/gcc-lb1sf68.S \
  --command 0x21 --display 87 \
  --expected-linear-elf-sha256 cddd1b01a4e277a8d8f82f16445dbc81dd8e50ab5a13c133fdeb25ef8353c090 \
  --out /private-output/dsp-zoh-causal
```

The wrapper freezes source, runs the arithmetic check, builds both firmware
variants with identical settings, captures the one original command twice,
verifies the ELF/native-state isolation and creates a `listening/` folder with
both untouched native excerpts and the original full-board reference. Exact
commands, build maps, symbols, firmware, sources, raw captures and logs remain
in that new private output directory. An existing completed pair can be checked
with `--analyze-existing /private-output/dsp-zoh-causal --environment-root /environment`.

Installing ZOH would require a separately identified runtime profile and new
acceptance measurements. This experiment does not fix active global banking,
the original fade/transition failures, or native FM synthesis differences.
Physical Saturn validation and listening acceptance remain open.
