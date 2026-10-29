# WP01 native SCSP FM bridge

This is an instrument/event implementation. The sound RAM asset contains a repeated single-cycle oscillator waveform. It contains no rendered note, channel, or soundtrack.

## Scope and resources

The original entrance uses six active four-operator YM2151 channels: 0, 1, 2, 3, 4, and 7. They map to SCSP slots 0–23, in groups of four. Slots 24–27 are reserved for the original OKI sample voices, and 28–31 remain unused. Channels 5/6 receive initialization-only partial keys in the audited trace. The bridge flags a later complete voice on either omitted channel.

SCSP RAM layout:

| Content | Start | Length | Wave start address |
|---|---:|---:|---:|
| Modulator oscillator and guards | `0x00000` | `0x20800` | `0x10000` |
| Terminal carrier oscillator and guards | `0x20800` | `0x20800` | `0x30800` |
| Original OKI samples, separate module | `0x44000` | module-defined | module-defined |

The guard regions cover the documented maximum phase displacement of ±32768 samples around a 1024-sample sine cycle. They prevent FM addressing from reaching unrelated PCM memory. Native looping uses LSA=0, LEA=1024. This does not require the undocumented SCSP short-wave feature.

## Oscillator and pitch

The 1024-point waveform is generated from the OPM logarithmic sine and power tables in YMFM, not selected by ear. Its signed output is expanded fourfold for 16-bit precision. The terminal-carrier bank includes the original arcade mono routing gain of 0.90; the native carrier's DISDL=5 divides the fourfold waveform by four. In the three algorithms used by this cue, terminal carriers do not feed another operator, so this output gain does not change the modulation index.

OPM phase step uses the original KC/KF lookup table, DT1 table, DT2 deltas, and multiplier. Conversion then uses YM2151 clock 3,579,545 Hz and native SCSP sample rate 44,100 Hz. Only the final SCSP FNS is quantized to its 10-bit mantissa. Unsupported pitch range is explicitly flagged. All audited entrance events fit the range.

Operator register offsets in natural connection order are `[0,16,8,24]+channel`. Key-on mask bits 3–6 correspond to that natural order. This differs from the ascending address order of the four register banks.

## Modulation and output

For a fourfold oscillator, one modulator connected to both inputs uses MDL=12. Two different modulator inputs use MDL=13 to compensate the SCSP input averaging. Feedback uses MDL=`YM feedback+4`, with self latest generation `0x20` and self past generation `0x00`. For earlier operators in the same group, routing is `32 + source_operator - destination_operator`. The Sega manual describes the two-generation self-feedback connection explicitly.

The actual cue uses:

| YM algorithm | Connections | Terminal carriers |
|---|---|---|
| 2 | O2→O3, then O1+O3→O4 | O4 |
| 3 | O1→O2, then O2+O3→O4 | O4 |
| 4 | O1→O2 and O3→O4 | O2 and O4 |

The generic topology table also encodes the other OPM algorithms. Algorithm 7 with feedback would require a different output-gain strategy because O1 is simultaneously a carrier and its own feedback source. That case is outside this cue and is not a fidelity claim of this release.

## Envelopes

OPM and SCSP both represent the envelope as logarithmic attenuation, but the rate clocks, available rates, amplitude interpolation and retrigger state differ. The generator evaluates the mean attenuation slope of each OPM effective rate at its original clock and each native SCSP rate at 44.1 kHz. It selects the closest native rate by logarithmic relative error. OPM key scaling is applied before that selection. Native SCSP KRS is disabled to avoid applying key scaling twice.

TL converts from OPM 0.75 dB steps to SCSP 0.375 dB steps by doubling. Sustain level maps directly, including OPM level15→31. Instant OPM attacks use EGHOLD and maximum native attack, preserving the initial full-level step while the native attack-to-decay transition completes.

This is a documented native synthesis approximation, not sample-exact OPM emulation. Remaining differences include SCSP envelope quantization/shape, sound-stack delay, 44.1 kHz synthesis, interpolation and the first inherited 17.6 ms of audio state preceding the first entrance command. The audited cue has no active LFO or noise; the bridge flags those features if requested instead of silently ignoring an active use.

## Event timing

Original KEYOFF→KEYON separations can be only five SCSP samples after time conversion. Hardware KYONEX is latched until the next sound sample. Therefore a sequencer must not collapse opposing key edges into one native sound sample. A 64-sample fixed batch scheduler is insufficient without special handling. Preserve chronological order and at least one sound sample between opposing edges.

The callback API emits byte offsets relative to the SCSP register base. It can be used on the host to compile the original YM event stream into native register writes, leaving the Saturn CPU responsible only for timed writes while SCSP generates the sound.

## Verification completed

- Host C11 build with `-Wall -Wextra -Werror`.
- Routing, operator ordering, key mask, feedback generation and slot isolation test passed.
- Full captured entrance stream converted: 432 initialization writes and 1,882 runtime writes; 559 key-execution writes; highest slot23; unsupported flags0.
- All five YMFM data tables used here match the pinned MAME0.264 YMFM source used by the independent reference renderer.

Native emulator/hardware acoustic validation belongs to the integrated build and is not implied by these host checks.

## Primary references

- Sega, **SCSP User's Manual**, ST-077-R2-052594, register map and FM chapters: <https://antime.kapsi.fi/sega/files/ST-077-R2-052594.pdf>.
- YMFM, Aaron Giles, BSD-3-Clause: <https://github.com/aaronsgiles/ymfm>. Original license accompanies the derived tables.
- Mednafen SCSP implementation, mirrored by its libretro port: <https://github.com/libretro/beetle-saturn-libretro/blob/master/mednafen/ss/scsp.inc>. Used to verify actual register, envelope, modulation and timer semantics.

SHA-256 identities of the exact reference files used are recorded in `fm-mapping-manifest.json`.
