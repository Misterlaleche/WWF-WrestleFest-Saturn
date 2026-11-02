# Original board clock and native envelope audit

The preserved A53 renderer omitted the YM2151 input clock divider. The original native pitch conversion inherited the same factor, so agreement between those two paths did not establish correct arcade pitch. Production now uses the documented board clock, while the historical A53/R04 material remains intact. No physical arcade or Saturn recording was obtained for this audit.

## Subsequent control-model audit

The corrected `board-pcm16` references below retained A53's **TimerB-only,
YM-busy-ignored and OKI-status-zero** control model. They are preserved as that
specific historical software profile. They must not be described as a complete
original-board control execution. A53 also supports only OKI bank0.

The separate `full-board` reference couples the unchanged full original Z80
core to the pinned Nuked OPM TimerA+B/busy/status/IRQ outputs and an original
ROM-addressed OKI model with real digital busy bits and global bank changes
during active playback. Its compatibility mode reproduces every byte of the
old board-pcm16 command01 trace and1,323,000 PCM samples. Enabling the fuller
model retains the original order/values of the first54,469 non-control YM
writes but changes their timestamps and moves one further write inside the
30-second window. OKI writes fall from684 to410 because274 redundant attempts
are no longer issued after the driver reads busy correctly.

The full model also exposed17 extra physical starts in the legacy A15 SFX
model and previous native engine. The ROM-grounded bounded correction and
RED→GREEN98-command fixture are documented in
`F1-ROM-SEQUENCE-AUDIT.md`. Historical A15/A53 material remains unchanged.
`tools/reference/reproduce_full_board_references.py` regenerates the separate
18 corrected full-board reference windows. These remain host software audio,
with instruction-level bus boundaries and no measured analog filtering; they
are not physical arcade recordings or certification of native SCSP fidelity.

The full model also proves an active-bank fidelity failure in the observed
mixed replay: 2,641 encoded-byte fetches and 2,663 decoded voice samples differ
from the current start-latched native policy, changing 14,680 mixed output
frames. The [active-bank audit](ACTIVE-OKI-BANK-AUDIT.md) records the causal
check, exact storage coverage and remaining playback-state/deadline work.
Native waveform fidelity remains **BLOCKED** independently of runtime counters.

## Primary evidence

`tools/reference/clock-source-manifest.json` records complete SHA256, verified Git blob ID, repository commit and canonical URL for each upstream file. The source files themselves are development evidence outside the source package. No ROM data or audio is stored in this document or manifest.

| Primary source | Pinned revision | Relevant evidence |
| --- | --- | --- |
| [MAME WWF machine configuration](https://github.com/mamedev/mame/blob/5c60f61d321dd7869f6cc2e7a6f78191a16b4d84/src/mame/technos/ddragon3.cpp) | `5c60f61d321dd7869f6cc2e7a6f78191a16b4d84` | `wwfwfest()` gives Z80 and YM2151 a 3,579,545 Hz external clock. OKI runs at 1.056 MHz with pin 7 high. Routes are nominal 0.33 per YM side and 0.66 for OKI. |
| [Furnace Nuked integration](https://github.com/tildearrow/furnace/blob/b087b28325298952aff552f9227ba83ab2afffcd/src/engine/platform/arcade.cpp) | `b087b28325298952aff552f9227ba83ab2afffcd` | `acquire_nuked()` calls `OPM_Clock` 32 times per generated sample. `setFlags()` sets the sample rate to external clock/64. |
| [Aaron Giles YM clock documentation](https://github.com/aaronsgiles/ymfm/blob/81aec25ccbb98f4873a255f7551ac4dadac59b4a/README.md) | `81aec25ccbb98f4873a255f7551ac4dadac59b4a` | The YM2151 divides its external clock by two before processing its 32 operators. |
| [MAME OKI ADPCM state](https://github.com/mamedev/mame/blob/5c60f61d321dd7869f6cc2e7a6f78191a16b4d84/src/devices/sound/okiadpcm.cpp) | Same MAME revision | `oki_adpcm_state::reset()` starts signal and step at zero. |
| [MAME MSM6295 voice handling](https://github.com/mamedev/mame/blob/5c60f61d321dd7869f6cc2e7a6f78191a16b4d84/src/devices/sound/okim6295.cpp) | Same MAME revision | A voice start resets its ADPCM state; pin 7 high selects clock/132, which is 8,000 Hz for this board. |

The unchanged Nuked-OPM dependency is commit `f209e6ed3712032b641d53ce8fb24824eae6adc3`. `opm.c` SHA256 is `983beecc471aebd8884fcc3aa48c50b1a26d4280ddbf1025b5196b540cf24188`; `opm.h` SHA256 is `186e042d1cd80c8fdf2e1a07d7c123bbe9a0d42e39518c50889db140a9ec4c39`. Both pinned source headers specify **LGPL version 2.1 or later**. The host tools preserve those notices; they do not link the synthesizer into the Saturn runtime.

In that core, `OPM_Clock()` advances its operator cycle modulo 32 and `OPM_PhaseGenerate()` advances each operator's 20-bit phase once per 32 calls. Therefore the caller must invoke the core at half the external board clock. The original A53 renderer called it on every external tick.

## Original-ROM pitch example and regression

Original music command `0x01`, channel 1, writes KC `0x55`, KF `0x00` immediately after its first key-on. Its C1 carrier has DT1/MUL `0x41`, DT2 zero. Pinned Nuked computes phase increment 13,096. Its correct nominal frequency is:

`13096 × 3579545 / (64 × 1048576) = 698.532481790 Hz`.

The historical renderer produces `1397.064963579 Hz` for that increment. The old native pitch word yields `1396.966552734 Hz`, an error of `1199.878045609` cents from the board-clock target. A red test records that failure before the production correction. The corrected native result is `698.483276367 Hz`, an error of `0.121954391` cents from this target.

A separate divider-only diagnostic replayed the same unmodified original trace, with all original notes and operator values intact. Channel 1's first-note window has a spectral peak near 698.273 Hz at the corrected clock. The historical A53 window has a peak near 1392.887 Hz and a stronger second harmonic near 2794.606 Hz. Detuned operators and overlapping harmonics make the composite FFT supporting evidence; the exact phase increment and clock calculation establish the factor.

Production `wf_ym_pitch_word()` starts one native octave lower. It does not change the original register sequence, tempo, phase-increment facts, modulation coefficient, original notes or instruments. The original control timer already uses 1,024 external clocks per Timer B count and is unchanged. The LFO frequency fact generator already uses external clock/64 and was not blindly halved.

The host regression checks 4,194,304 KC/KF/DT1/MUL/DT2 combinations against the pinned core. Each phase increment matches exactly; each nonzero frequency is also checked against the external-clock/64 target, with a two-cent ceiling. The observed maximum native pitch quantization error is 1.467590375 cents across this exhaustive set.

## Envelope correction and its limits

The old rate calibration had two independent problems: its OPM side used the doubled pipeline rate, and its native side used a different SCSP envelope implementation. Production now generates its rate map from pinned OPM envelope unit measurements at the corrected board clock and the equations in the selected unmodified Mednafen 1.32.1 `src/ss/scsp.inc`. That source's SHA256 is `0a1e00647022c86c33edfa1601ae7a9fddde45884e34bf4a92015442e04f9906`.

`tools/reference/measure_opm_envelopes.c` calls the exact pinned envelope functions in the same order as `OPM_Clock`, holding their unit-test register inputs constant. `calibrate_native_envelopes.py` independently evaluates native key-on, envelope-clock and amplitude equations. It verifies both dependency identities before measuring. The chosen rate minimizes the absolute log ratio of threshold times: attack to 90% of maximum amplitude, or decay through 128 attenuation units. The starting envelope timer phase is explicit. This is a deterministic model calibration; it does not establish exact full-curve equivalence or physical SCSP accuracy.

The previous header fails 99 of the 128 rate-map entries against this criterion. Some examples of the corrected model fit are:

| Original effective rate | Attack target | Native attack | Decay target | Native decay |
| --- | ---: | ---: | ---: | ---: |
| 32 | 54.5343 ms | 51.5193 ms | 109.4597 ms | 123.3560 ms |
| 36 | 27.2861 ms | 25.7596 ms | 54.7489 ms | 61.6780 ms |
| 40 | 13.6621 ms | 12.8798 ms | 27.3934 ms | 30.8390 ms |
| 48 | 3.4172 ms | 3.2200 ms | 6.8500 ms | 7.7098 ms |
| 60 | 0.3598 ms | 0.3628 ms | 0.8426 ms | 0.7256 ms |

Original effective attack rates 62/63 start at zero attenuation. With native KRS disabled, the SCSP starts at attenuation `0x280` and its fastest attack reaches the selected threshold in about 0.2721 ms. The selected native key-on equation starts directly at zero when raw AR plus native key scaling reaches 32. The bridge now uses that encoding where it can exactly compensate the other nonzero native rates. Their effective decay/release rate numbers remain unchanged; zero rates remain disabled. A regression using an original command-01 patch fails before this correction and passes afterward.

Across all 18 original 30-second traces, 17,271 rising operator key edges request the original instant attack. The current encoding represents 16,966 directly; 305 use the ordinary fast native attack because their desired native release rate is 1. A positive native key scale cannot preserve that release with a nonzero raw rate. These 305 cases correspond to 26 original parameter configurations, all with original release nibble zero. This is a limit of the current static encoding, not proof of an unavoidable hardware limit. Restoring KRS specifically at key-off is a possible further change, but would require new register-order/timing and native-capture validation and is not part of this profile.

All 914,095 YM writes in those trace windows pass the bridge checks, including the effective decay/release invariants. All 923,055 YM/OKI/bank events from the serviced native sequence still match the full original Z80 oracle in value, order and cycle. These counts bound the checked windows; they do not establish complete musical loops or native waveform equality.

## Current and preserved reference profiles

`tools/reference/reproduce_full_board_references.py` is the current primary
software renderer. Given the original ROM zip and hash-verified pinned Nuked
dependency, it reconstructs inputs and captures the original Z80 with both
timers, busy/status/IRQ and global ROM banking. It writes 18 reference WAVs and
an explicit `full-board` report. `verify_full_board_sfx.py` and
`export_full_board_sfx.py` provide the corresponding 98-command validation and
`full-board-sfx` WAVs. No recovered handoff directory is required.

`reproduce_board_clock_references.py` is retained as the intermediate
**Timer-B-only** clock/PCM comparison. Its complete scope is described at the
start of this document. The following `board-pcm16` measurements belong to
that preserved profile.

The default `board-pcm16` profile uses the corrected YM clock, original initial-zero PCM16, an 8,000 Hz OKI clock, and the complete final sample interval. It writes `ARCADE-REFERENCIA-01.wav` through `-18.wav`. The completed 30-second set has 1,323,000 frames per track and no clipped samples. The original decoded pack is 1,755,496 bytes with SHA256 `45a22577841fc0ca9bdf84f6ec8acf72f2c87602c9a50eb42fce395a57131bee`; all 114 entries also have independent original-sample digest checks.

The optional `divider-only-pcm8` profile changes only the YM divider in the historical renderer and retains its original PCM8 and tail behavior for causal diagnostics. `reproduce_all_references.py` separately reproduces the immutable historical A53 sources, exact 12-second hashes, independent logic gates and 98 command-model effect references. A corrected reference is never made to masquerade as an original A53 byte match.

All reference WAVs use 44,100 Hz mono signed PCM16 and nominal fixed routing, with no normalization or time stretching. The host output uses sample/hold conversion and does not reconstruct the original analog DAC/filter path. These WAVs are software references, not physical arcade recordings and not Saturn runtime assets.

## Actual corrected native captures

The corrected profile has now run all 18 original music commands for 35 seconds each in unmodified standalone Mednafen 1.32.1. Each command has a complete 30-second native stereo/reference mono WAV pair. All 18 captures report zero worker/API errors, discarded or rejected events, transport lateness and PCM underruns. The source snapshot, firmware, resources, BIOS, references and emulator identities remained stable. This is an isolated cold-boot public-API profile, not a gameplay, listening or physical-hardware acceptance result.

The tested intermediate firmware SHA256 is `621b5dfcfe1b2a1841ff09e32de62b8198c63cd5d5c12d1f660d5c8426c06634`. The YM source SHA256 is `c12ebda831820dcdf04b0be98830f52fda0843179e378385bd92638dfbfb3c76`; the envelope header SHA256 is `5f2e75edb5de0d5f3c1979f9e86b5d51602106a5cdad67671abfce4b4ca9bcf8`. The complete private report is `music-matrix-boardclock-intermediate/native-music-matrix.json`, SHA256 `e523149700925bcee9309d2571eb214e063934f354a023224b7c8e81d6235fac`. A metadata-only summary is included as `tools/reference/evidence/native-music-board-clock-intermediate.json`. A later decoder or firmware build needs its own runtime gates; this result cannot be transferred to it.

For command `0x01`, the 30-second matrix pair measures a reference spectral centroid of 373.27 Hz and a native centroid of 505.05 Hz, a log-power spectral RMSE of 3.495 dB and a native RMS level 1.123 dB lower. These measurements use raw levels, with no normalization, resampling or time stretching. Waveform phase is not independently calibrated. These differences remain material and are not a fidelity pass.

The later [PCM/capture-level audit](PCM-INTERPOLATION-AND-CAPTURE-GAIN.md)
identified a fixed `27/32` multiplier after `SCSP.RunSample()` in the selected
unmodified Mednafen output path, equivalent to -1.475724 dB. All raw native
captures include that factor. It is a separate analytic consideration when
interpreting levels, not a reason to rescale delivered audio or alter the
runtime to compensate for an emulator-specific factor. It does not remove
the measured spectral, envelope, phase or control-model differences.

### Original-operator isolation

Separate 8-second diagnostics used the same corrected YM/envelope source with frozen experimental firmware SHA256 `044484de2418b145c05e068d6a0eaf2b442990d3b0cb1a66919c05c94e474f7c`. Only mixer output gates were changed. Original register writes, note timing, operator phase, feedback and envelope state remained active. PCM output was muted. All four isolated captures completed without the runtime errors listed above.

| Original command-01/channel-1 output | Reference centroid | Native centroid | 10 ms envelope correlation |
| --- | ---: | ---: | ---: |
| Both original carriers | 3190.24 Hz | 4256.32 Hz | 0.866 |
| C1, fed by M1 with feedback 7 | 3847.90 Hz | 5520.19 Hz | 0.839 |
| C2, fed by M2 without feedback | 1260.56 Hz | 1221.53 Hz | 0.965 |
| Internal M1 output, made audible only for diagnosis | 1100.68 Hz | 1216.30 Hz | 0.695 |

The two separate host carrier taps sum back to the complete isolated channel with a maximum difference of four PCM16 units and RMS difference of 0.982 units, consistent with output rounding. This validates the tap interpretation without altering the pinned synthesis core. The actual native results localize much of the high-frequency excess to the feedback branch; they do not identify every remaining difference.

### A demonstrated feedback timing difference

`tools/reference/analyze_original_feedback.py` inspects the unchanged pinned core while replaying the original full-Z80 command-01 trace. It records the actual first M1 operator's unmodulated phase, attenuation, feedback history and output. For 61,524 observed samples, independently evaluated logarithmic-sine/exponential transfer and feedback formulas have zero mismatches. A free-running scalar recurrence over the first note also reproduces every pinned-core output exactly at the original rate.

The diagnostic then changes only the recurrence interval to 44.1 kHz. It interpolates the original physical phase trajectory and attenuation onto that grid, keeping feedback 7. This is a counterfactual unit experiment, not a Saturn capture or an alternate primary reference. The original recurrence rate is `3579545/64 = 55930.390625 Hz`.

In the 40–210 ms first-note window, the strongest high-frequency peaks move from approximately 18.154/17.455 kHz in the original recurrence to 13.964/14.662 kHz in the interval-only counterfactual. Actual native M1/C1 captures contain corresponding peaks near 13.972/14.672 kHz. The small frequency offset is consistent with the separately measured native pitch quantization. This demonstrates that the feedback sample interval contributes to the observed timbre difference. It does not prove that it explains the whole error, or that every possible native architecture is incapable of reducing it.

The scalar amplitude transfer also differs: at original M1 TL22 and zero envelope attenuation, the selected native equation gives a peak of 5055, while the pinned OPM peak multiplied by four is 4856, a difference of 0.349 dB. Choosing another static TL to fit this one point would not preserve the whole envelope transfer. No such level adjustment has been applied to production.

Earlier A53 comparisons remain preserved as historical evidence, including their known doubled-clock profile. A passing phase test, a smaller spectral metric or an unchanged original command trace does not establish full native fidelity. Envelope curves, sampled-sine quantization, modulation/feedback timing, resampling and the analog path remain distinct questions. Physical Saturn validation remains pending.
