# Prolonged native validation of the frozen candidate

**Runtime acceptance: FAIL.** The profile named `final-cddd-scalar9` is the
frozen candidate tested here, not a production or fidelity approval. The
complete collection contains 18 actual isolated native music captures of
600 seconds followed by original command `1F` and an eight-second tail, plus
one 32-second transition capture. Four music cases pass the independent
runtime gate; fourteen music cases and the transition case fail.

All 19 selected captures have complete independent evidence. The auditor
verifies the actual native F5 state, complete loaded 68000 firmware segments,
468 SH2 functions against each linked ELF, actual counters, monotonic observed
SCSP time, finalized WAVs, build/disc identities and capture history. No
physical Saturn measurement or gameplay-integration approval is asserted.
The machine-readable public evidence is
[`native-long-final-cddd-scalar9.json`](evidence/native-long-final-cddd-scalar9.json).

## Frozen experiment and timing

The firmware SHA256 is
`cddd1b01a4e277a8d8f82f16445dbc81dd8e50ab5a13c133fdeb25ef8353c090`.
The source snapshot contains 327 files, including selected SH2 scalar9 code
and the music sequencer compiled with `-Os`. Source, inputs and emulator
identities match before and after the collection. Later host analysis and
capture-helper changes do not change the tested executable or disc.

The selected unmodified standalone Mednafen 1.32.1 executable has SHA256
`a12628d798edf9ad1ec7497cec666ee64b583a09fc48a0c650702c8aeb3a7689`.
Its cartridge setting is `none`. Upstream presentation speed 4 was used on
reserved displays 94–97; hardware clocks were unchanged. That option was
previously checked against normal presentation with exactly equal stereo
PCM over 30 seconds. It is not an emulator speed or chip-clock adjustment.

The harness measures each window from the first actual public-API admission
using the SCSP clock. It requests `1F` at sample 26,460,000 and records its
actual admission at sample 26,460,248 for all 18 music cases: 600.005624 seconds,
a 248-sample service delay. Each music case admits both requested words.
The separate transition script requests `01,06,12,1F,03,1F,02,01,1F` at
three-second intervals; only its first four words are admitted before the
failure. The native run still reaches its 32-second observation endpoint.

## Final counter table

Music IDs before the slash are decimal filenames; original command IDs are
hexadecimal. “FM cancelled” counts records already queued when failure
cancels playback. “Ring rejected” counts events not published. The late-event
counter belongs to transport service; it is not a substitute for the separate
50 ms fatal FM/key deadlines. Every value comes from the selected closed
capture and the independent audit.

| Music / command | Runtime | Worker / API error | FM cancelled | Ring rejected | Late events | PCM underruns | Words admitted |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| 01 / `0x01` | FAIL | 15 / 5 | 1 | 0 | 0 | 0 | 2/2 |
| 02 / `0x02` | FAIL | 15 / 5 | 3 | 0 | 34 | 0 | 2/2 |
| 03 / `0x03` | FAIL | 15 / 5 | 2 | 0 | 4 | 0 | 2/2 |
| 04 / `0x04` | FAIL | 15 / 5 | 4 | 0 | 3 | 0 | 2/2 |
| 05 / `0x05` | PASS | 0 / 0 | 0 | 0 | 0 | 0 | 2/2 |
| 06 / `0x06` | FAIL | 15 / 5 | 1 | 0 | 2 | 0 | 2/2 |
| 07 / `0x07` | FAIL | 15 / 5 | 1 | 0 | 0 | 0 | 2/2 |
| 08 / `0x08` | PASS | 0 / 0 | 0 | 0 | 0 | 0 | 2/2 |
| 09 / `0x09` | PASS | 0 / 0 | 0 | 0 | 0 | 0 | 2/2 |
| 10 / `0x0A` | FAIL | 15 / 5 | 1 | 0 | 9 | 0 | 2/2 |
| 11 / `0x0B` | FAIL | 15 / 5 | 5 | 0 | 2 | 0 | 2/2 |
| 12 / `0x0C` | FAIL | 15 / 5 | 2 | 0 | 25 | 0 | 2/2 |
| 13 / `0x0D` | FAIL | 15 / 5 | 1 | 0 | 1 | 0 | 2/2 |
| 14 / `0x0E` | FAIL | 15 / 5 | 1 | 0 | 8 | 0 | 2/2 |
| 15 / `0x0F` | FAIL | 15 / 5 | 1 | 0 | 4 | 0 | 2/2 |
| 16 / `0x10` | PASS | 0 / 0 | 0 | 0 | 0 | 0 | 2/2 |
| 17 / `0x11` | FAIL | 15 / 6 | 2 | 0 | 4 | 0 | 2/2 |
| 18 / `0x12` | FAIL | 18 / 5 | 5 | 1 | 1 | 0 | 2/2 |
| Transitions | FAIL | 15 / 5 | 1 | 0 | 2 | 0 | 4/9 |

Across the 18 music cases there are **30 cancelled FM records, 97 late events,
one rejected ring event and zero PCM underruns**. Including the transition
case gives 31 cancelled FM records and 99 late events. These totals describe
separate cases; they are not a continuous mixed-gameplay counter.

Commands `05`, `08` and `09` were identified as naturally ending in the
separately scoped original-control observation. Their successful 608-second
windows do not demonstrate sustained music work for ten minutes. Command
`0x10` is the additional passing music/fade case. Its last nonzero captured
sample occurs approximately 4.904535 seconds after the measured `1F`
admission, with the final two seconds exactly silent. This measures its
digital tail without certifying the original waveform.

## What the observed failures mean

In the frozen [`wf_music_module.c`](../src/wf_music_module.c), error 15 is
`MUSIC_ERROR_FM_DEADLINE`: an FM register write is more than 2,205 SCSP frames
(50 ms) overdue. Error 18 is `MUSIC_ERROR_KEY_DEADLINE`: a key metadata event
is more than the same 50 ms limit overdue before publication. The corresponding
fault handler cancels queued FM records and stops the native slots. The 14
failing music cases comprise 13 error 15 cases and command `0x12` with error 18.
The transition case also ends with error 15.

Command `0x11` retains worker error 15, API error 6 (`WF_AUDIO2_ERROR_NATIVE`)
and mailbox state 3. Its first observed transport lateness is at 600.273197 s;
the first observation showing worker 15/API 6/mailbox 3 is at 600.306485 s.
Command `0x12` retains worker 18, API 5 (`WF_AUDIO2_ERROR_WORKER`), mailbox 2,
one rejected ring event and five cancelled FM records. These distinct values
must not be collapsed to one generic error. The observer does not establish
the instruction-level causal order of the worker and native/mailbox failures.

Every valid sampled observation before the first measured fade admission has
zero error/lateness/rejection/underrun counters. This is an observation-window
statement, not continuous instrumentation. The following intervals retain
both the last clean read and first failing read. Times are seconds from the
first API admission; “after fade” uses the first accepted `1F`.

| Command | Last clean observation | First failing observation | First failure observed after fade |
| --- | ---: | ---: | ---: |
| `0x01` | 600.306508 | 601.643696 | 1.638073 |
| `0x02` | 600.674240 | 600.707664 | 0.702041 |
| `0x03` | 599.069569 | 601.092109 | 1.086485 |
| `0x04` | 599.838481 | 602.044853 | 2.039229 |
| `0x06` | 600.172766 | 600.206213 | 0.200590 |
| `0x07` | 600.273197 | 600.323197 | 0.317574 |
| `0x0A` | 600.323220 | 600.373356 | 0.367732 |
| `0x0B` | 600.339932 | 600.373356 | 0.367732 |
| `0x0C` | 600.674240 | 600.707664 | 0.702041 |
| `0x0D` | 600.573946 | 600.607370 | 0.601746 |
| `0x0E` | 600.323220 | 600.356644 | 0.351020 |
| `0x0F` | 600.423515 | 600.456939 | 0.451315 |
| `0x11` | 600.239637 | 600.273197 | 0.267574 |
| `0x12` | 600.189501 | 601.209093 | 1.203469 |
| Transitions | 9.176599 | 9.210023 | 0.200590 |

For `0x03` and `0x04`, the observation gap crosses 600 seconds, so these data
do not prove the exact failure onset was after 600 seconds. In the other
failing music cases, a clean observation at or after 600 seconds precedes the
first observed error. Observed diagnostic reads do not identify an exact bus
cycle at which a deadline was missed.

All 19 selected WAVs have exactly silent final two-second windows. In failed
cases the fault handler itself stops sound. **That silence is not a successful
original fade or evidence of musical completeness.** Runtime failure remains
failure even when a 30-second comparison pair can be exported.

## Capture interruption and preserved retries

The original collection closed 16 music cases and the transition case before
filesystem exhaustion interrupted `0x11` and `0x12`. Those two attempts retain
their logs, invalid WAV containers, empty/incomplete reports and hashes.
They were never repaired or overwritten. After space recovery, only those two
captures were repeated, using their byte-identical original ELF and ISO with
a newer host capture/audit helper. The native runtime and emulator stayed the
same. Both retries exit normally and have complete WAV/F5/report evidence;
both still fail the engine runtime gate.

The first retry matches every available PCM byte of its interrupted attempt:
108,765,140 bytes. The second matches 70,717,396 bytes. No header was repaired
and no audio was combined across attempts. WAV length minus measured SCSP
elapsed time and final clock residual gives the same observed raw-origin
band, 663,361–663,362 frames, across all 18 selected music captures. This is
additional capture-integrity evidence; it does not calibrate the residual
DAC phase or certify waveform fidelity. The interrupted attempts remain
invalid even though their available payload prefixes match.

The original reconciliation, independent audit and final catalogue are
separate immutable artifacts:

| Artifact | SHA256 |
| --- | --- |
| Reconciled 19-case matrix | `db1f7716a3e8c754ebf6099b54c269df98e98924198bdb0057c9bc8eb33ebcc1` |
| Independent v2 audit matrix | `efc84bf18e8a6dc08407067ab5ea1b0ea74ca500780c4b602de0bf28324349b7` |
| Final catalogue with v2 gates and every attempt | `9111da25b5ee740fe7935c68462e73d959c5570214b62287d9975b96f32c8902` |
| Retry report and unchanged-original checks | `f519a40a6a6da175765596dabf03c2147886a718775336e76531547c569c111d` |
| Retry sample-integrity metadata | `253176130f9a3edce37bb7ec4d7e27aa96fcdf03807c0a36136495ff804087d2` |

The catalogue holds 19 rows with the independent v2 gate result and its SHA256
per row, and preserves both capture attempts for `0x11`/`0x12`. Its sibling
`pairs` directory contains 18 comparison reports and 36 unnormalized WAVs.
Those comparison WAVs are 30-second excerpts, not the complete 608-second
native records. The reference profile is the coupled full-board software
model, not an analog arcade recording.

## Reproduction and remaining acceptance boundaries

`tools/run_music_matrix.py --score-seconds 600 --tail-seconds 8 --transitions`
creates the complete declared workload from one immutable package/input
snapshot. Its normal environment/resource/firmware options are described in
[`tools/reference/README.md`](../tools/reference/README.md). Use a new output
directory and sufficient free space. Per-stage commands, source/input
identities and capture decisions are retained privately in the matrix.

The independent read-only audit can be repeated without launching an emulator
or modifying a closed capture:

```sh
python3 tools/run_native_test.py --audit-only \
  --build /path/to/selected-build --out /path/to/selected-capture \
  --audit-output /path/to/new-audit.json
```

The audit output must be a new file outside the build and capture directories.
The v2 auditor used here has SHA256
`30e354e024f7c5ec3d546b5579cf0733a444616a8ca4292621f448e0f1e854e2`.
A separate host-label regression now prevents unwritten zero-filled replay
rows from being called accepted commands. The older transition report already
failed its overall gate; the corrected label preserves that failure and
explicitly records 4/9 admissions.

These finite windows do not prove every complete composition or a sample-exact
loop. The earlier 600-second score-state detector retains its explicit
Timer-B-only MMIO scope in
[`MUSIC-STRUCTURE-AND-LONG-VALIDATION.md`](MUSIC-STRUCTURE-AND-LONG-VALIDATION.md).
The independently proved active-global-OKI-bank mismatch remains a fidelity
blocker in [`ACTIVE-OKI-BANK-AUDIT.md`](ACTIVE-OKI-BANK-AUDIT.md). FM waveform
and envelope differences, original MMIO coverage, listening acceptance and
physical hardware validation also remain separate from this runtime gate.
