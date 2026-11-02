# Native fade cost correction: historical checkpoint `24b84435`

**Status: the best verified selected correction at this checkpoint still
fails native acceptance.** Its 13 host regression groups pass, including all
18 commands for 600 seconds plus the original fade. Its separate native
screen passes **14 of 18 music cases at 38 seconds**; `04`, `06`, `07`, `12`
and the transition case fail. There is no passing 600-second native campaign
for this checkpoint in this record.

This document fixes the evidence boundary at `24b84435`. That label is the
prefix of a source-file SHA-256, **not a Git commit ID**. A later successful
candidate must receive its own identities and report; it must not turn this
historical failure into a PASS. The machine-readable
[checkpoint evidence](evidence/native-fade-cost-24b84435.json) contains only
aggregate measurements, counters and identity hashes.

## Identity and scope

| Item | SHA-256 |
|---|---|
| `src/reference_driver/z80-native.c` | `24b844357544e12cfa7b1a09f7d368d895a918b00c129815438827102b1ea0fa` |
| `SCSP.ELF` | `cddd1b01a4e277a8d8f82f16445dbc81dd8e50ab5a13c133fdeb25ef8353c090` |
| Original host report | `f58d69d9984d5431f6a081c24cd6cf0c1910776288e7850455806b26dd08d105` |
| Original native screen report | `cdd5baaabf67ec5dea61fb983f204ef763217114b348a1fdfde5e7dea6591a30` |

The selected native profile is
`cddd-scalar9-exact-fade-prefix-TL-return-24b84435-30s-screen`. It uses real
standalone Mednafen 1.32.1 Saturn execution, with no expansion cartridge.
The source, input and emulator identities remained stable during that
campaign. Requested presentation speed was 4; emulated hardware clocks were
unchanged. This is neither a physical-Saturn result nor a Graphics Final
integration result.

Against the recorded scalar9 baseline, the only changed common runtime
source is `z80-native.c`. The sequence, module, clock conversion, firmware,
YM and DSP source identities remain recorded as unchanged. The correction
does not enlarge buffers or the music lead, change clock ratios, relax
deadlines, or change the scheduler's timing policy. The selected build uses
15,196 hot-code bytes plus 3,220 state bytes within the existing 18,432-byte
limit. These facts describe this exact checkpoint and do not automatically
apply to later private variants.

## Why fade processing was investigated

The original causal case sends music `02`, then original command `1F` at
six seconds, with an eight-second tail. The original scalar instruction
profile shows a substantial increase in work after the fade starts:

| Original one-second interval | Scalar instructions, excluding idle skips | YM writes | TL writes |
|---|---:|---:|---:|
| Last second before fade | 94,791 | 1,176 | 0 |
| First fade second | 239,857 | 3,114 | 1,944 |

These are original-program work counts, not SH-2 execution-time measurements.
They identify the repeated level, multiply and YM-write paths worth testing.
A separate private native observer supplies the clock evidence: the observed
fade interval progressed **3,548,551 original Z80 cycles while 49,481 SCSP
samples elapsed**. At the respective 3,579,545 Hz and 44,100 Hz clocks, that
is **0.991341 seconds of original progress in 1.122018 seconds of SCSP time**.
The shorthand “about one original second in 1.12 SCSP seconds” refers to
this bounded interval; it is not a measurement of exactly 1.000 original
second. The last observed target was 444,138 original cycles ahead when
error 15 appeared.

The work increase and measured loss of processing progress support a native
CPU-cost/deadline diagnosis. They do not assign all cost to one helper or
predict every music command. The clock observer was a separate instrumented
diagnostic; its metadata and result hashes are preserved in the evidence.
Its state arrays are private, and the selected runtime did not acquire that
instrumentation. The public original-program profiler is reproducible with
the command below; it does not claim to reproduce a native throughput
measurement by itself.

## What the selected correction preserves

The correction accelerates guarded operations already performed by the
original Z80 program. It preserves their logical instruction counts, cycles,
refresh register, CPU flags, stack/RAM effects and observable callbacks at
the tested boundaries. Insufficient budget, interrupt, stack or operand
conditions retain the scalar fallback.

| Area | Selected work |
|---|---|
| Fade level and scale | Exact operations at `0BE7` and `0BD7` |
| Multiply continuation | Resume the original operation at `0F3A` |
| Level preparation | Exact preparation beginning at `0B68` |
| TL register writes | Four guarded prefixes at `0C09`, `0C16`, `0C21`, `0C2C`, stopping at `0F5F` |
| YM return | Exact return handling at `0F5F` |
| Dispatch | A PC hint avoids unnecessary dispatch work without changing program time |

The exact fade regression covers **1,507,643 cases**. The TL regression
covers **793,344 cases**: 790,920 accepted operations and 2,424 rejected
admissions. It also compares 1,581,840 write-callback snapshots per engine.
The final source passes all **13 host test groups**. The serviced R01
differential covers 18 tracks at **600 seconds plus eight seconds of fade**,
10,962 checkpoints, 956,109 sequence calls and 3,151,318 service callbacks,
plus the live transition sequence and `00` rejection.

These checks establish the tested CPU/adapter semantics. They do not turn
host wall time into native performance, prove a synthesis loop, or certify
the resulting Saturn waveform. The host report and the native screen remain
separate evidence in the checkpoint JSON.

## Native screen: the remaining failures stay visible

Each music case requests 30 seconds of score, admits `1F` through the real
public API and captures through 38 seconds. Admission is observed in native
RAM; the fade is not assumed to occur at the requested sample. All 18 music
cases admit both words. The transition case admits only four of nine.

Passing music IDs are `01`, `02`, `03`, `05`, `08`, `09`, `0A`, `0B`, `0C`,
`0D`, `0E`, `0F`, `10` and `11`. Failed cases have these final counters:

| Case | Runtime | Error | `lateB028` | FM discards | API error | Admitted / requested |
|---|---|---:|---:|---:|---:|---:|
| `04` | FAIL | 15 | 28 | 4 | 5 | 2 / 2 |
| `06` | FAIL | 15 | 46 | 5 | 5 | 2 / 2 |
| `07` | FAIL | 15 | 31 | 9 | 5 | 2 / 2 |
| `12` | FAIL | 15 | 31 | 1 | 5 | 2 / 2 |
| Transitions | FAIL | 15 | 26 | 21 | 5 | 4 / 9 |

WAV availability and a silent final tail do not override those failures.
In particular, an engine that has stopped progressing can become silent.
`0F` passes runtime gates but has a final two-second tail peak of one PCM
unit; that tail observation is recorded separately and has not been used
to grant waveform fidelity approval. A 38-second screen also cannot stand
in for the requested complete compositions. Pass counts from it must not
be compared as equivalent coverage with an older 600-second campaign.

## Four measured private variants that were not selected

The following bounded native experiments use command `04`, 30 seconds plus
an eight-second tail. All remain FAIL in their recorded runs and are outside
the `24b84435` source:

| Private variant | Final error / API error | `lateB028` / FM discards | Evidence boundary |
|---|---:|---:|---|
| Timestamp conversion elision | 0 / 6 | 38 / 0 | Paired with its own placement-matched control; both move startup-copy out of the hot section |
| Additional carrier lift | 15 / 5 | 35 / 4 | 393,276 exact private carrier-state cases; no passing native campaign |
| Early service on TL/return baseline | 15 / 5 | 35 / 4 | 44 fragmented-grant cases; all 18 host sequences at 15 s plus fade, not 600 s |
| Early service plus timestamp elision | 15 / 5 | 49 / 10 | Additional synthetic clock/queue equivalence; native acceptance still fails |

The early-service semantic audit retains the maximum of 512 represented
original instructions between callbacks. Its timestamp increment reduces
synthetic time-advance calls from 10,103 to 4,579 while preserving the
recorded synthetic state/queue trace. That host observation is not a native
deadline result. Code placement is part of each complete native patch, and
the final counters span different lengths of failing playback; they are
not comparable throughput rates.

The `08C1` extension is deliberately outside these four measured results
and outside this checkpoint's acceptance claims. Any new candidate, including
that work, needs its own frozen source identity and validation. None of the
private variants has been integrated into Graphics Final by this work.

## Original composition structure is separate evidence

The coupled original-board investigation classifies all 18 commands as
**three natural FB endings, twelve complete visible-state recurrences and
three sets of independently recurring channel cycles**. The latter are
`0B`, `0C` and `11`, covering 23 active FM channels and three PCM score
channels. It does not manufacture a short common loop for channels whose
original cycles differ. See
[the complete original music structure guide](ARCADE-COMPLETE-MUSIC-STRUCTURE.md).

That original score-control evidence does not erase native runtime failures,
prove periodic PCM, or establish complete native composition acceptance.
Conversely, the absence of whole-synthesizer equality is not a reason to
discard the verified per-channel original cycles.

## Reproduce without hardcoded private paths

Use a source checkout/export whose hashes match the checkpoint. Set these
paths to your own private inputs and choose new output directories:

```sh
CHECKPOINT_SOURCE=/private/source-checkpoint-24b84435
VALIDATION_TOOLS=/private/source-with-complete-audio-tools
ROM_ZIP=/private/original-rom-archive.zip
RESOURCES=/private/generated-resources
FIRMWARE=/private/firmware-cddd/SCSP.ELF
ENVIRONMENT_ROOT=/private/reproduced-environment
BIOS=/private/saturn-bios.bin
OPM_DIR=/private/pinned-opm-source
OUT=/private/new-fade-checkpoint-results

cd "$CHECKPOINT_SOURCE"
python3 tools/run_host_tests.py \
  --rom-zip "$ROM_ZIP" --resources "$RESOURCES" \
  --firmware "$FIRMWARE" --out "$OUT/host" --seconds 600
```

For the focused semantic tests or original scalar work profile:

```sh
python3 tools/verify_sh2_fade_lifts.py \
  --rom "$RESOURCES/SOUND.ROM" --out "$OUT/fade-semantics"
python3 tools/verify_sh2_tl_writes.py \
  --rom "$RESOURCES/SOUND.ROM" --out "$OUT/tl-semantics"
python3 tools/reference/profile_original_fade.py \
  --rom "$RESOURCES/SOUND.ROM" --out "$OUT/original-fade-profile"
```

Build the original full-board reference once. The 30-second references below
are the matrix's prefix-comparison inputs, not a complete-composition claim.
The same built executable can later replay complete observed admissions:

```sh
python3 tools/reference/reproduce_full_board_references.py \
  --rom-zip "$ROM_ZIP" --opm-dir "$OPM_DIR" \
  --out "$OUT/full-board" --seconds 30 --jobs 2

. "$ENVIRONMENT_ROOT/env.sh"
python3 tools/run_music_matrix.py \
  --package "$CHECKPOINT_SOURCE" --resources "$RESOURCES" \
  --firmware "$FIRMWARE" --sdk "$ENVIRONMENT_ROOT/sh2eb-elf" \
  --environment-root "$ENVIRONMENT_ROOT" --bios "$BIOS" \
  --reference-report "$OUT/full-board/reference-report.json" \
  --out "$OUT/native-screen" --score-seconds 30 --tail-seconds 8 \
  --transitions --presentation-speed 4 --displays 94,96 \
  --wall-seconds 240 \
  --profile-label cddd-scalar9-exact-fade-prefix-TL-return-24b84435-30s-screen
```

Use available display numbers in your environment. A failed runtime matrix
correctly returns a nonzero exit status while retaining its evidence.
The historical 38-second results above do not authorize describing a future
600-second run as passed before it has actually completed its gates.

## Complete audio preparation already available

`tools/prepare_music_audio_pairs.py` **v1.0.1** exports full native windows
from real soundrecord PCM and replays the original full board at the actual
native command admissions. It retains the native **4096-sample lead**, with
no normalization, gain adjustment, resampling or waveform-fit alignment.
Reference output is explicitly a host board model; native output is actual
Mednafen Saturn execution, not a physical-Saturn recording.

The real `01` diagnostic already verifies a 38-second stereo window byte
for byte against its source capture, with `1F` at observed sample 1,323,159.
Its [public manifest](../tools/reference/evidence/native-audio-pairs-command01-38s-diagnostic.json)
keeps the matrix failures and marks the complete collection and fidelity as
unapproved. Nine synthetic regressions and the 83-test Python suite passed
for the generator version. The generator does not change the runtime.

For one diagnostic case, use `VALIDATION_TOOLS` for the published generator
and structure evidence. This permits the historical runtime snapshot to
remain unchanged even if it predates those tools:

```sh
python3 "$VALIDATION_TOOLS/tools/prepare_music_audio_pairs.py" \
  --matrix-report "$OUT/native-screen/native-music-matrix.json" \
  --reference-root "$OUT/full-board" \
  --structure-summary "$VALIDATION_TOOLS/tools/reference/evidence/full-board-complete-music-structure.json" \
  --out "$OUT/diagnostic-command01" --diagnostic-only --case command-01
```

For the final collection, first obtain a separately identified matrix with
`--score-seconds 600 --tail-seconds 8 --transitions` and all runtime gates
passing. Assign its report to `MATRIX_600_PASS_REPORT`, then run:

```sh
python3 "$VALIDATION_TOOLS/tools/prepare_music_audio_pairs.py" \
  --matrix-report "$MATRIX_600_PASS_REPORT" \
  --reference-root "$OUT/full-board" \
  --structure-summary "$VALIDATION_TOOLS/tools/reference/evidence/full-board-complete-music-structure.json" \
  --out "$OUT/complete-audio-pairs"
```

The default export includes all 18 music cases and the complete transition
case. It rejects short or failed matrices and missing admissions, rather
than inventing unobserved commands. This checkpoint cannot pass that final
export gate on the strength of its short screen. Audio remains private;
only aggregate manifests, code and hashes belong in the public package.
