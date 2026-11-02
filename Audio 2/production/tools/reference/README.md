# Reproducible host references and native logic checks

This directory can reproduce all reference assets and the independent host gates from one original user ROM zip. No recovered handoff directory is required. Generated ROM data, command tables, PCM, WAVs, traces, fixtures and host executables belong in the explicitly selected output directory, outside this source package.

These tools are for development on a host computer. Nuked-OPM is never linked into the Saturn firmware. The native runtime generates FM with the SCSP and decodes original sample ADPCM; the WAVs below are comparison assets, not runtime music playback assets.

## Current primary reference: coupled full-board model

New comparisons use `full-board`: the original complete Z80 program coupled to
the pinned Nuked YM2151 with **Timer A and Timer B, busy/status and IRQ**, plus
an original-ROM OKI model with busy bits and a global bank applied to every
active encoded-byte fetch. This is the current software reference, not a
physical arcade recording. Its instruction-level bus scheduling and digital
sample/hold output remain explicit model limits.

| Profile | Use and boundary |
| --- | --- |
| `full-board` / `full-board-sfx` | Current music/SFX reference; coupled original CPU, both YM timers, YM/OKI status and active global ROM banking |
| `board-pcm16` | Preserved corrected-clock intermediate reference; Timer-B-only CPU/MMIO model and no original busy coupling |
| `divider-only-pcm8` | Historical causal clock diagnostic; retains the partial A53 MMIO and PCM8 behavior |
| A53 | Immutable historical prefix regression; doubled YM pitch and partial MMIO, not current arcade authority |
| A15 SFX model | Historical command/PCM regression; 17 commands produce extra physical starts under F1, so it is not an audible acceptance oracle |

Requirements: Python 3.10 or newer, a C99 host compiler and the original user
ROM zip containing `31a11-2.ic42` and `31j10.ic73`. Input and pinned dependency
SHA256 identities are verified. From `production`, use new private output
directories:

```sh
python3 tools/reference/reproduce_full_board_references.py \
  --rom-zip /path/to/01-wwfwfest.zip --out /path/to/private-full-board --jobs 2
python3 tools/reference/verify_full_board_sfx.py \
  --rom-zip /path/to/01-wwfwfest.zip --out /path/to/private-full-board-sfx --jobs 2
python3 tools/reference/export_full_board_sfx.py \
  --validation /path/to/private-full-board-sfx --out /path/to/private-full-board-sfx-wav
```

The music output contains 18 unnormalized 30-second mono PCM16/44.1 kHz WAVs.
The SFX validator generates its original-ROM fixtures privately, checks the
actual physical starts and preserves the comparison with the historical A15
model; the exporter verifies every source PCM identity before writing WAVs.
Both use the verified board clock, initial-zero OKI decode and documented
nominal mix gains. See the [clock/MMIO audit](../../docs/ARCADE-CLOCK-AUDIT.md),
[F1 audit](../../docs/F1-ROM-SEQUENCE-AUDIT.md) and
[active-bank blocker](../../docs/ACTIVE-OKI-BANK-AUDIT.md).

The current native engine does not reproduce the original global-bank case
proved by the mixed replay. Successful isolated starts, sample hashes or native
runtime counters therefore do not establish complete audible fidelity.

All reproduction outputs must use new directories. The coupled C producer
also refuses any existing output path, including when a recorded direct
command bypasses the Python wrapper. The
[output integrity and recovery audit](../../docs/REFERENCE-OUTPUT-INTEGRITY.md)
records the preserved truncated attempts, separate exact-hash recoveries,
causal overwrite regression and current full-board register-use summary.
Historical command arguments must not be replayed into their original files.

## Preserved intermediate board-clock reference: partial MMIO

The clock audit found that the preserved A53 renderer advances Nuked-OPM at twice the rate implied by the original WWF board clock. Its exact hashes remain useful historical regression evidence; its pitch is not the absolute arcade reference. The following intermediate profile corrected the YM divider and PCM source while retaining the Timer-B-only MMIO model. It is preserved for causal comparisons; use the coupled profile above for current listening references. See [the complete clock audit](../../docs/ARCADE-CLOCK-AUDIT.md) and `clock-source-manifest.json` for primary-source commits, Git blob IDs and SHA256 hashes.

Requirements: Python 3.10 or newer, a C99 host compiler (`cc` by default), and the original WrestleFest ROM zip containing `31a11-2.ic42` and `31j10.ic73`. Contents are identified by SHA256, so an unrelated or modified ROM fails before rendering.

From the `production` directory:

```sh
python3 tools/reference/reproduce_board_clock_references.py \
  --rom-zip /path/to/01-wwfwfest.zip \
  --out /path/to/private-board-reference \
  --seconds 30 --jobs 2
```

This writes `wav/ARCADE-REFERENCIA-01.wav` through `-18.wav`, the unchanged original Z80 traces, and `reference-report.json`. It uses YM2151 external clock 3,579,545 Hz with a divide-by-two pipeline clock and 32 operators, plus original PCM16 at 8,000 Hz. The initial OKI signal and step are zero, and the final decoded sample is retained for its complete sample interval. Nominal routing remains YM left/right `10813/32768` each and OKI `21627/32768`. Audio is not normalized, time-stretched or reconstructed as an analog board signal. Output sample/hold at 44.1 kHz remains an explicitly documented model limitation.

`--profile divider-only-pcm8` creates a separately named causal diagnostic that changes only A53's YM clock divider, keeping its historical PCM8 input and tail behavior. The default is `board-pcm16`. The old A53 files and prefix checks are never rewritten.

## Historical A53 and independent logic gates

To reproduce the immutable historical baseline, its independent logic tests,
and the 98 historical command-model effects (including the known F1 mismatch):

```sh
python3 tools/reference/reproduce_all_references.py \
  --rom-zip /path/to/01-wwfwfest.zip \
  --out /path/to/private-reference-output \
  --seconds 30 --jobs 2
```

Both wrappers download `opm.c` and `opm.h` from Nuked-OPM commit `f209e6ed3712032b641d53ce8fb24824eae6adc3` and verify their complete SHA256 digests. An offline run can use an already downloaded directory with `--opm-dir /path/to/pinned-Nuked-OPM`. Dependency files are copied into the output directory after verification. Both pinned source headers specify **LGPL version 2.1 or later**; their original copyright and license notices remain intact. `--cc` selects another host compiler.

`--prepare-only` reconstructs and verifies the private inputs without downloading a dependency, building a tool, or rendering audio. This includes exact reconstruction of the independent A15 C model from the ROM, rather than storing the 98-command table in this package.

The ordinary run performs five stages:

1. Capture original Z80 control execution and render all 18 music commands using the preserved A53 software oracle. Check every first 12 seconds against the original archived PCM hashes.
2. Compare native YM phase arithmetic against pinned Nuked-OPM, normalized modulation depth against pinned OPM and the selected Mednafen SCSP arithmetic, and native serviced sequence output against every captured YM/OKI/bank event and cycle.
3. Audit actual YM register use in the captured windows, including LFO, noise, CSM, algorithms and key masks.
4. Decode all 114 complete original sample entries, check their independent historical hashes, verify monolithic and incremental prepared prefixes/decoder states, and compare all 98 native effect commands and mixed priority behavior with the archived A15 command model.
5. Render all 98 complete A15 model effects, including their PCM tails, followed by 250 ms of silence.

The wrapper stops on a failed stage and writes `reproduction-report.json`. A completed wrapper is evidence for these host checks, not a native audio acceptance result.

## Outputs and how to interpret them

| Output | Meaning |
| --- | --- |
| `music/wav/ARCADE-REFERENCIA-01.wav` through `-18.wav` | Historical A53 renders, 30 seconds by default; doubled YM pitch retained only for immutable baseline reproduction |
| `music/reference-report.json` | Input identities, compiler invocation, PCM and WAV hashes, all 18 archived 12-second prefix checks, trace origin and end cycles |
| `music/traces/music-01.csv` through `music-12.csv` | Original control traces; filenames use hexadecimal command numbers |
| `logic/ym-and-sequence-validation.json` | Exhaustive phase arithmetic and serviced native control versus full original Z80 trace checks |
| `ym-register-usage.json` | Features observed in these trace windows; absence is bounded by the window |
| `sfx-validation/sfx-validation.json` | Complete PCM digest comparison and native SFX behavior tests |
| `sfx/wav/ARCADE-SFX-MODELO-0x20.wav` through `-0x81.wav` | Complete independent A15 command-model effect renders |
| `sfx/events/*.csv` | Effect play/stop decisions at rational event times, including suppressed sample starts |
| `sfx/reference-sfx-report.json` | Effect format, exact rates and volume rule, input hashes, durations and sample statistics |
| `inputs/reference-inputs.json` | Identity of every reconstructed private input and an explicit distinction between the two PCM baselines |

All reference WAV collections use 44,100 Hz, one channel, signed 16-bit PCM. No collection is amplitude-normalized. The audio levels use documented nominal reference routing, without per-track adjustment. The decimal music filename `18` corresponds to command `0x12`; effect names use command IDs only, without unverified speech labels.

### Music origin and scope

The capture tool executes a two-second reset/boot interval before placing the command in the sound latch and raising NMI. It records the exact cycle as `ORIGIN`. The renderer synthesizes the entire reset/boot history, begins its export clock at `ORIGIN`, and writes the first sample after its first 44.1 kHz accumulation interval. No attack is trimmed and no later onset alignment or normalization is applied. The precise origin and end cycles are recorded per track.

The 30-second output is an excerpt. It does not establish that a full composition has ended, that every loop boundary was traversed, or that a transition/fade path is identical. Longer windows up to 600 seconds can be requested, but time alone is not proof of musical completeness. Separate control tests may examine longer execution and fades; their results remain control-level evidence.

The A53 renderer preserves its original mono mix: YM left and right each use `10813/32768`; OKI uses `21627/32768`. This is inherited reference calibration. The unchanged renderer asserts bank zero for its music catalog; it is not used to render bank-one effects.

### The two PCM baselines are intentionally different

The original A53 combined renderer loads **signed PCM8** from `AUDPCM.PAK`. Its archived A15 pack starts the sample decoder at signal `-2`, emits `(signal >> 4)`, deduplicates the 95 distinct encoded samples, and aligns new samples to 2048-byte boundaries. It is 980,196 bytes. The complete PCM pack and full 114-row index are regenerated byte for byte and checked against their original hashes. A one-row test index from an old handoff is not a usable substitute.

The independent native decoder audit uses **PCM16BE**, starts signal at `0`, emits `signal * 16`, and aligns unique samples to 32-byte boundaries. Its pack is 1,755,496 bytes. All 114 sample-entry digests and the whole pack digest are independently fixed by `oracle_sfx/WF_OKI_AUDIT.json`.

Taking the high byte of the PCM16 pack does not reproduce the A53 PCM8 pack. Changing one baseline to resemble the other would invalidate the historical exact checks. The wrapper preserves both explicitly. The use of an initial `0` in the native path is also consistent with the standard OKI decoder in upstream MAME; the archived initial `-2` PCM8 pack is retained because it defines the A53 evidence, not because this package asserts it is a superior physical-chip model.

### Effect model and scope

The effect renderer uses the independent archived A15 sequence model, not the production effect sequencer. The original 98-command table is regenerated by `generate_resources.commands()` from the original sound ROM. The reconstructed whole C file must match the immutable archived SHA256, which prevents a silently altered model or table from passing.

Its source samples are the complete SHA-verified PCM16 originals. Playback uses zero-order hold at exactly 8,000 Hz. Timer A advances at `3579545/(64*253)` Hz, with rational event ordering and timer first on exact ties. Original volume numerators over 32 are `{32,22,16,11,8,6,4,3,2,0,0,0,0,0,0,0}`. The final output also applies the inherited R04 route gain `21627/32768`. A command is accepted at model time zero; this does not invent or claim the original Z80 command latency.

Rendering ends after the logical event sequence is complete and its physical sample tail has ended, then appends 11,025 silent output frames. Command `0x20` is the model's empty command. These renders provide sample content and command-model references. They are not native SCSP captures, original arcade recordings, or the same level of evidence as the full-Z80 A53 music oracle.

## What is preserved here

`oracle_a53/tools/capture_full_audio_a50.c`, `oracle_a53/tools/render_combined_a50.c`, and `oracle_a53/evidence/A53-EXACT-12S-RENDER-METRICS.csv` are byte-identical copies of the historical definitive handoff. `oracle-input-sha256.json` records their identities. The independent Z80 source and its license already live in `production/src/reference_driver`.

`oracle_sfx/wf_audio_sfx.c.in` preserves the independent A15 model's code with only its ROM-derived table replaced by a placeholder. Its header, the original ROM sample range/identity manifest, and the complete PCM digest audit are also included. These metadata files contain identities and ranges, not samples. The preparation tool checks the reconstructed model's whole-file hash before compiling it.

## Modulation-depth regression

The production bridge corrects a verified factor-of-four depth error in the historical native routing. This is a coefficient correction, not an instrument substitution. The native sine uses a nominal 16-bit operator range and a 128-sample cycle; pinned OPM uses a 14-bit operator range and 1024 phase steps.

Pinned `OPM_OperatorPhase15()` averages the selected operator inputs; `OPM_OperatorPhase1()` adds the feedback shift. The selected Mednafen 1.32.1 `src/ss/scsp.inc` computes SCSP modulation from the sum of two signed sound-stack inputs, shifted by `16-MDL`. The source file SHA256 is `0a1e00647022c86c33edfa1601ae7a9fddde45884e34bf4a92015442e04f9906`. With equal normalized operator inputs, the resulting settings are single-source `MDL9` (the source is selected twice), two-source `MDL10`, and feedback `MDL=FB+1`. Historical settings `11`, `12`, and `FB+3` applied four times the intended phase displacement.

The regression calls the exact pinned OPM functions and separately evaluates the selected SCSP arithmetic for 726 signed input combinations. It tolerates only the difference between the two phase quantizers, at most 6/64 of one native wave sample. The old routing fails this test before the source correction. This is a gain-scaling test with matched normalized inputs; it does not claim that the two waveform/envelope generators or modulation delays are identical.

The [Sega SCSP FM modulation manual](https://www.infochunk.com/saturn/segahtml_en/hard/scsp/hon/p04_23.htm) and the selected emulator both disable `MDL0..4`. Consequently `FB1..3` cannot be represented by this direct mapping. The 30-second original catalog traces use feedback `0`, `5`, and `7`; the independent feedback test covers representable settings `4..7`. It does not certify the low feedback settings.

## Fidelity limits

Passing these gates verifies input identity, sample decoding, command behavior, sequence output and phase arithmetic for the stated cases. The exhaustive phase test also checks the corrected board-clock native pitch across 4,194,304 cases, with a two-cent ceiling. The actual native SCSP bridge still quantizes pitch and envelope rates, uses a sampled sine and SCSP modulation, and has different chip timing. Noise, CSM, nonzero test mode and direct low-feedback `FB1..3` are unsupported. LFO conversion uses finite SCSP choices and does not recreate the YM's global LFO exactly.

`calibrate_native_envelopes.py` rebuilds the rate map using the unchanged pinned OPM envelope unit at the proper board clock and independently transcribed selected Mednafen 1.32.1 equations. It checks the full source hash before accepting `--scsp-source`. Its criterion is attack time to 90% of maximum amplitude and decay by 128 attenuation units, with a documented starting timer phase. This is a model fit, not a complete envelope equivalence proof. The bridge additionally encodes original instant attacks with positive native KRS when all other effective rates can be preserved; original rate-1 releases retain the ordinary fast attack. The complete audit explains this remaining static-encoding limit.

```sh
python3 tools/reference/calibrate_native_envelopes.py \
  --opm-dir /path/to/pinned-Nuked-OPM \
  --scsp-source /path/to/mednafen-1.32.1/src/ss/scsp.inc \
  --out /path/to/private-envelope-audit \
  --check-header src/generated/wf_ym_envelope.h
```

Native captures and comparisons against the raw reference levels are a separate required measurement. A favorable sequence hash or a reproduced A53 software hash cannot certify the native waveform or physical Saturn performance.

## Frozen native music matrix

`tools/run_music_matrix.py` snapshots the complete production source, input resources, firmware and its symbols, BIOS, primary reference report and WAVs before building any command. It runs the selected standalone Mednafen on one to four supplied displays and retains a failed runtime result even if an audio pair can still be exported. The default runs at normal presentation speed. It does not change emulated hardware clocks.

After sourcing the reproduced environment's `env.sh`, run from `production`:

```sh
python3 tools/run_music_matrix.py \
  --resources /path/to/private-full-board/inputs \
  --firmware /path/to/frozen-firmware/SCSP.ELF \
  --sdk /path/to/environment/sh2eb-elf \
  --environment-root /path/to/environment \
  --bios /path/to/user-owned/mpr-17933.bin \
  --reference-report /path/to/private-full-board/reference-report.json \
  --out /path/to/private-native-matrix \
  --profile-label exact-source-and-firmware-profile \
  --seconds 35 --displays 86,88
```

The report identifies all sources and inputs before and after the run, verifies that the expected firmware prefix is in native SCSP RAM, and checks native transport counters, finalized stereo WAV output and `ss.cart none`. `pairs/` contains decimal music filenames `01` through `18`; command directories use hexadecimal `01` through `12`. A 35-second runtime window supplies the 30-second reference comparison plus its alignment margin. Neither this window nor a green runtime gate proves a complete composition, gameplay stability or waveform acceptance.

## Reproduce the original feedback timing diagnostic

The corrected native captures localized a residual high-frequency difference to an original feedback branch. The following host-only unit diagnostic records the original command-01/channel-1 M1 phase, attenuation and output from the unchanged pinned core. It first verifies the scalar transfer and feedback recurrence against every observed core output, then changes only the feedback interval while retaining the original physical pitch and attenuation trajectory. It produces state/metric evidence, not a replacement reference WAV.

```sh
python3 tools/reference/analyze_original_feedback.py \
  --trace /path/to/private-board-reference/traces/music-01.csv \
  --opm-dir /path/to/pinned-Nuked-OPM \
  --out /path/to/private-feedback-diagnostic
```

The tool rejects altered dependencies or an incompatible first-note window. `feedback-interval-experiment.json` records all equality checks, exact rates, identities, spectral observations and limitations. The [clock and native audit](../../docs/ARCADE-CLOCK-AUDIT.md) relates this controlled experiment to the actual isolated Mednafen captures. No original note or instrument is replaced in production.
## Full-board mixed replay and long-run scope

Use the separate coupled profile for new listening comparisons. It executes
the original full Z80 with pinned Nuked TimerA+B, YMbusy/status/IRQ and a
ROM-addressed OKI model with busy bits and active global banking. Historical
A53 and `board-pcm16` remain reproducible, with their narrower MMIO scope.

```sh
python3 tools/reference/reproduce_full_board_references.py \
  --rom-zip /private-input/wwfwfest.zip --out /private-output/full-board --jobs 2
python3 tools/reference/verify_full_board_sfx.py \
  --rom-zip /private-input/wwfwfest.zip --out /private-output/full-board-sfx --jobs 2
python3 tools/reference/export_full_board_sfx.py \
  --validation /private-output/full-board-sfx --out /private-output/sfx-wav
```

Every output path must be new. `--opm-dir` can supply the already downloaded,
hash-verified dependency. The new SFX fixture and all ROM-derived state/audio
are generated privately. The gate compares actual native physical starts with
full original execution; it catches17 extra starts that the preserved A15
physical model makes under F1. See `docs/F1-ROM-SEQUENCE-AUDIT.md`.

For an isolated native replay, supply its original request plan, completed
build and capture to `reproduce_mix_reference.py`. That tool uses the measured
API admissions, refuses simultaneous or overlapping unread NMI/latches,
retains native runtime failures and musical lead, and exports complete and
fixed30-second WAV pairs without gain, time stretching or waveform alignment.
It does not reconstruct game video or PCB command timing.

The600-second control detector and native long-matrix workflow are documented
in `docs/MUSIC-STRUCTURE-AND-LONG-VALIDATION.md`. The detector's current evidence
uses the historical TimerB-only model; its recurrence tiers do not prove PCM
loops. Native stability results apply only to the exact frozen firmware/source
profile captured. Full-board software references are not physical recordings.

## Private PCM interpolation A/B

`prepare_dsp_zoh_probe.py` constructs a private DSP-only ZOH variant and checks
its exact integer arithmetic. `run_dsp_zoh_probe.py` freezes the source, builds
both firmware variants, captures one original SFX command twice and verifies
that every changed ELF byte is in the DSP program. Native F5 states also check
the loaded programs, unchanged coefficients, registers, phase constants and
PCM rings. The [experiment report](../../docs/PCM-INTERPOLATION-AND-CAPTURE-GAIN.md)
gives the complete reproduction command, raw metrics and the separately
identified Mednafen `27/32` output factor. This is a private experiment;
production and the main acceptance matrices retain their frozen DSP profile.


## Closed prolonged native result

The [frozen candidate long-run report](../../docs/NATIVE-LONG-FINAL-CANDIDATE.md)
records all 18 music windows of 600 seconds plus original fade/tail, the
transition case, and independent v2 capture audits. Runtime acceptance fails:
4 music cases pass, 14 music cases and the transition fail. Every selected
capture has complete identity evidence; the two ENOSPC attempts and their
unchanged-disc retries remain explicit. Final digital silence is not treated
as a successful fade after a runtime fault.
