# Native SCSP register renderer — host validation only

`render_scsp` is a standalone adapter around the unchanged SCSP audio engine
copied from the Mednafen-derived Beetle Saturn core. It accepts native SCSP
register writes and sound RAM, then produces stereo PCM at44,100Hz.

It is a **host validation tool**, not code for the Saturn executable. It does
not emulate either SH-2 or the sound68000, CPU contention, a CD drive, or the
wall-clock cost of submitting registers. Its output cannot certify physical
Saturn performance or eliminate differences between an emulator model and the
actual chip.

## Build and run

```sh
g++ -std=c++17 -O2 -Wall -Wextra -Wno-unused-parameter render_scsp.cpp -o render_scsp
./render_scsp RAM_BE.bin writes.csv output.wav 313219 debug.csv
```

`RAM_BE.bin` contains up to512KiB of big-endian sound-RAM bytes, beginning at
RAM address0. CSV rows have `sample,address,value`, where sample is a44.1kHz
output index and address is the byte offset from SCSP registerbase. Decimal
or `0x` values are accepted. Full addresses in `0x25B00000..0x25B00FFF` or
`0x100000..0x100FFF` are also accepted. Rows must remain in original order,
including multiple writes on the same sample.

The optional debug CSV records the24 FM slots' envelope, pitch, phase and
modulation selectors approximately everymillisecond. All register writes occur
before that sample's audio clock. Init and master-volume writes belong in the
CSV; the renderer itself does not silently add them.

`render_variant.py` adds the explicit test master-volume setting `0x400=0x020F`
and produces both the fullFMmix and six isolated-voice files. Isolation changes
only the direct output volume of the other groups; their operator and feedback
processing remains active.

## Core provenance and adapter boundaries

`primary/scsp_mdfn.h` and `.inc` are byte-for-byte copies of the source used by
the architecture agent; hashes are in `primary/provenance.json`. Upstream:

- https://github.com/libretro/beetle-saturn-libretro/blob/master/mednafen/ss/scsp.h
- https://github.com/libretro/beetle-saturn-libretro/blob/master/mednafen/ss/scsp.inc

The original files retain their GPL license notices. The adapter supplies
standard integer helpers, no-op save-state metadata, and interrupt-state
callbacks. It does **not** alter envelope, FM modulation, waveform, mixing,
timer, or DSP processing. JIT is disabled. Save/load is not used.

## Verification and comparisons

`smoke-verification.json` verifies a1024sample sine: the measured440.084839Hz
equals the frequency implied by SCSP pitchword6428, and output is silent after
the released envelope finishes. This is a wiring/model smoke check, not a
physical hardware comparison.

The baseline `mix.wav` and `ch*.wav` use the original native mapper's2314
writes plus explicit master volume. `comparison.json` compares them with the
independent MAME0.264 YMFM reference. No gain fit, pitch shift or time stretch
is applied. RMS blocks are10ms, so detuned beating also affects those metrics;
their errors must not be attributed solely to the envelope generator.

`interleaved/` contains the controlled alternative operator layout. The results
are mixed: small improvements on some voices and small regressions on others.
`layout-comparison.json` records spectral convergence and absolute log-spectral
errors. This evidence does not establish an overall fidelity improvement.

The scalar chip gain, modulation history, envelope quantization, and differing
native synthesis clocks remain relevant when interpreting differences. Native
SCSP audio is not asserted sample-identical to the original YM2151 output.
