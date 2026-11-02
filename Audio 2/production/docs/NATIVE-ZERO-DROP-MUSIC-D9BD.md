# Native music checkpoint `d9bd93a7`: short music PASS, mixed replay FAIL

**Completed short music matrix: PASS, 18 music cases plus one transition case.**
**Completed host regression: PASS, 14 groups.** The current profile still has
a runtime blocker: its separate **64-word, 138-second isolated mixed replay
fails with two transport-late events**. Both its worker and API error counters
remain zero. The short music result therefore does not establish global
zero-error acceptance for this profile.

The native campaign with 600 seconds before the original `1F` command is
**IN PROGRESS** at this record's boundary. No partial result from that campaign
is included or classified as PASS here. Combat has not been re-executed for
this checkpoint. These results do not grant production, Graphics Final,
physical-Saturn, listening or acoustic-fidelity approval.

The machine-readable record is
[`native-music-d9bd-short-and-host.json`](evidence/native-music-d9bd-short-and-host.json).
It contains aggregate counters, test results and identity hashes. ROM data,
WAV data, raw event traces and private filesystem locations are excluded.
The record date is 2026-11-02.

## Measured source and fixed limits

`d9bd93a7` is the prefix of a source-file SHA-256, not a Git commit ID. The
short native matrix, completed host run and causal proofs identify the
following source files. Their identities match the selected current files.

| Item | SHA-256 |
|---|---|
| `src/reference_driver/z80-native.c` | `d9bd93a7cfdf8e5dbb022bce6df1dc132e6caf7e9c3c05a18e75e2debd9602e0` |
| `src/wf_music_seq.c` | `8d833f42349267376e0d29ff5b2c0db0fa31af5c544252e7306e87ec7bfd66bc` |
| `src/wf_music_module.c` | `4514091b2ce9019b6ba7ed62ea8fcf65d9c658a6a857bae498a6a2fbc0052b69` |
| Fixed `SCSP.ELF` firmware | `cddd1b01a4e277a8d8f82f16445dbc81dd8e50ab5a13c133fdeb25ef8353c090` |
| Standalone Mednafen 1.32.1 | `a12628d798edf9ad1ec7497cec666ee64b583a09fc48a0c650702c8aeb3a7689` |

All short-matrix builds use **15,008 bytes of hot code plus 3,220 bytes of
state: 18,228 of 18,432 bytes, leaving 204 bytes**. The linker limit remains
active. The startup-copy helper resides in ordinary `.text` with `noinline`;
its body is unchanged. The fixed allocation has not been expanded.

The caller budget remains **4,096 original instructions**, and the service
grant remains **512 original instructions**. Timer B and target boundaries,
queue sizes and deadline thresholds are unchanged. The short native matrix
used no expansion cartridge. Requested presentation speed was 4; emulated
hardware clocks were unchanged. Source, input and emulator identities remained
stable throughout the completed matrix.

## Completed native results and the current blocker

Each of the 18 music cases requests its original command at the API epoch,
then `1F` at 30 seconds, followed by eight seconds of observation. Original
music command IDs are hexadecimal `0x01..0x12`. The separate 32-second
transition case requests `01,06,12,1F,03,1F,02,01,1F` at three-second intervals.

| Completed native workload | Cases | Requested words accepted | Worker / API errors | FM discarded | Transport late | Rejected FM / ring | PCM underruns | Result |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| Music, 30 s + 8 s each | 18 | 36/36 | 0 / 0 | 0 | 0 | 0 / 0 | 0 | PASS |
| Transitions, 32 s | 1 | 9/9 | 0 / 0 | 0 | 0 | 0 / 0 | 0 | PASS |
| Current mixed harness replay, 138 s | 1 | 64/64 | 0 / 0 | 0 | **2** | 0 / 0 | 0 | **FAIL** |

The short matrix has no evidence-integrity errors, no observed diagnostic
history errors and no nonzero discard-batch counters. Its independent gate
binds the actual native F5 state, loaded firmware, linked SH2 function bytes,
diagnostic mirror and capture/build identities. All 19 short cases completed
their requested windows and passed the replay-admission checks.

The current mixed replay also completed its full 138-second window, accepted
all 64 words and has `evidence_errors=[]`. Its firmware and linked SH2 bytes
match the loaded state. The strict runtime gate fails on `lateB028=2`; the
associated history and actual-counter gates also fail. This is a current
`d9bd93a7` result, not a failure borrowed from an older profile. It is an
isolated replay harness, with no gameplay producer added, and is **not a
combat or Graphics Final integration rerun**. Its two late events remain an
acceptance blocker despite the absence of fatal worker/API errors.

The short matrix separately reports
`all_final_tails_exact_silence_observed=false`. Runtime success is not a
waveform comparison or an assertion that every final tail is exact digital
silence. Acoustic interpretation and listening approval remain separate.

## Why the new scheduling and timestamp work matter

### Preserve whole-operation admission when a service grant is fragmented

The existing native operations implement complete runs of original Z80
instructions. They admit only when the remaining instruction budget and
time boundary cover the whole operation. A nearly exhausted 512-instruction
grant can otherwise force the scalar path, even when the global caller
budget still has enough instructions. The causal RED cases reproduce this
condition at the unchanged original entry points.

After making progress, the batch now yields at a recognized operation entry
if its remaining grant is below the operation's conservative quota. On
resumption, the scheduler commits cycles and services pending work if the
global budget covers that quota, then resets only the grant's service count.
A batch with no progress does not repeatedly yield. An insufficient global
budget continues through the scalar path; a failed service callback stops
before the operation mutates CPU or RAM.

Entry `08C1` was missing from the earlier quota set. Its bound is:

```text
57 + (negative_offset ? 5 : 0)
   + (active_modulators ? 22 * active_modulators : 1)
```

Each active modulator costs at most 7 setup instructions plus 15 for the
subtraction/clamp path. Lower and upper clamps are mutually exclusive. With
an active modulator, the final extra instruction can occur only after the
shorter add-overflow path; that path's `7 + 7 + 1` cost is already covered by
22. With no active modulator, one final instruction is reserved. The maximum
is therefore **106**, reached by a negative offset and two subtract-clamp
paths. The scheduler threshold is `service_steps > 406`, and the batch
shortage threshold is 106. Invalid channels outside `0..7` and SP outside
`C408..C7FE` return quota zero without mutation or MMIO.

The isolated `04` comparison held TL operations, F5F return, early service
and timestamp elision fixed. Adding only the `08C1` quota and its matching
thresholds changed the 30 s + 8 s replay from **error 15, 10 FM discards and
49 late events** to **all-zero runtime acceptance**. Both versions used
18,228 bytes of hot code/state. This comparison isolates that complete source
increment, including its code placement; it does not turn a host timing
measurement into a native speedup claim. It also does not override the later
mixed-replay failure above.

### Avoid repeated timestamp conversion for already identical YM state

The mapper already treats eligible writes of the same register value as
unchanged state. The module now recognizes those writes before converting
their timestamp, provided the synth is not faulted. The eligible registers
are `reg >= 0x20`, `0x18` and `0x1B`; key commands retain their existing path.
Original Z80 execution and its event callbacks are preserved.

The portable module proof observes **10,103 timestamp conversions in RED and
4,579 in GREEN**, avoiding 5,524 conversions in that fixture. The targeted
duplicate changes from one conversion to zero. The complete **33,889,139-byte**
synthetic proof output—mapper state, FM queue and mailbox timestamps—is equal
byte for byte, with SHA-256
`003a537ee75ce0bb27dfba2e868436af141fa8e1700a98de3bdb7ad3b96cd844`.
That is a state/timestamp equality proof, not an acoustic-fidelity result.

## Host and independent proof coverage

The completed final host run passes **14/14 groups** with
`source_stable_during_tests=true`. It includes resource corruption and input
validation, API behavior, module and firmware deadlines, SFX refill, timestamp
elision, exact fade operations, exact TL writes, and original Z80 sequencing.
These results are bound to the closed report's input hashes. Subsequent
v1.1.0 exporter work is outside that run and receives separate verification;
the runtime identities listed above remain unchanged.
The exact fade suite reports **1,507,643** CPU/RAM/cycle/refresh cases. The TL
suite reports **790,920 accepted**, **2,424 rejected** and **1,581,840 callback
snapshots per engine**.

The original R01 comparison completes all 18 commands for **600 seconds plus
eight seconds of fade**, with **10,962 checkpoints**, 956,109 serviced calls
and 3,200,182 services. CPU/RAM/timer states and event counts/hashes match.
The live transition test also passes with 89,011 events and hash
`e4e46eb2779d4f80`, including command `00` rejection. This is completed **host**
evidence; the separate **native** 600-second campaign remains in progress.

The self-contained
[`verify_early_service.py`](../tools/reference/verify_early_service.py)
uses frozen pre-service source oracles. It observes **38 quota RED cases and
11 earlier-entry RED cases**, then passes **65 new original-state cases**,
**602,369 quota/guard combinations** and **44 earlier-entry cases** in GREEN.
Its 18-track 15 s + 8 s R01 test and live transitions audit **62,497,765
original instructions** and 144,812 services, with maxima of **512 instructions
per grant and between service callbacks**. Instructions represented by an
operation are included; existing `idle_skip` retains its separate skipped
counter. The current timestamp proof is reproduced by
[`verify_timestamp_elision.py`](../tools/verify_timestamp_elision.py).

Independent review adds **9,216 original-state comparisons**, **92,415
nonmutating rejections**, **720 target/timer/wrap boundary cases** and **360
IRQ callback CPU checks**. It also compares all **94,542 event tuples and
complete callback CPU states** of command `04` for 30 s + 8 s, literally,
with no differences and maximum service/grant counts of 512. Its event hash
is `a633a1b50e7b4935`. That callback count is distinct from the native API's
`musicEvents` metadata counter.

## Historical boundary and outstanding work

The historical
[`24b84435` checkpoint](NATIVE-FADE-COST-CORRECTION.md) and its
[evidence](evidence/native-fade-cost-24b84435.json) remain unchanged. It passed
14 of 18 short music cases; `04`, `06`, `07`, `12` and transitions failed.
The new checkpoint has separate identities and does not relabel that failure.
The two transport-late events of the current mixed replay are also retained
as FAIL rather than folded into the successful short-music aggregate.

At this document's evidence boundary:

- The native 600 s + 8 s campaign is **IN PROGRESS**. No partial cases have
  been used here to declare that campaign successful.
- The current 64-word mixed replay is **FAIL** and blocks all-zero runtime
  acceptance for the overall profile.
- Combat/Graphics Final has **not been re-executed** for this checkpoint.
- Physical hardware, listening, acoustic fidelity and production acceptance
  are **not established** by the results recorded here.

## Evidence identities

The private locations are omitted. These hashes bind the aggregate record
to the completed source reports; none is a hash invented from a partial run.

| Evidence record | SHA-256 |
|---|---|
| Completed 18-music + transition short matrix | `45ec80c066051fac6057f6333019f8735c72f7954f4291d45e864035c7a10f7d` |
| Final 14-group host run | `ce430e76431530883acfdd5934b329cbc2df72b38ba5ee4d8e53906442f64f42` |
| Portable early-service RED/GREEN proof | `e2ca68d62cbe371b938046b712f7a4088532d0710a860d82cb8f95e644bcad62` |
| Stable portable timestamp-elision proof | `8aac1b51d406af48c54fd6de88666ff2b764459488decc75748556daf2ef615e` |
| Independent 08C1 review | `c32656cba9ad5bff9fff198340e5cdb5d3d87a34e5049ed9659c9aafa3f22d75` |
| Isolated 08C1 native A/B report | `9eed51e017cfc81740e9507ab0b3bb58013be5f8fce8d4a3501f2f232da3c87b` |
| Current 64-word / 138 s mixed native gate, FAIL | `c1d7ec9dd352fc5381cb1ac06ad5760e57a4468e9b05c8b5294b05264c672128` |

## Completed long native matrix

The later immutable600-second score matrix has completed:18 music cases plus
the nine-word transition all PASS. Every FM discard, batch discard, rejection,
PCM underrun, lateness, worker-error and API-error counter is zero. Source,
input and Mednafen identities remained stable; all19 captures have valid
ELF/F5/RAM/WAV evidence. The short/host JSON above is a preserved earlier
checkpoint, whose IN_PROGRESS field is not the current long-run result.

See [the complete600-second evidence](evidence/native-music-d9bd-complete600s.json).
This does not clear the isolated mixed-replay two warnings, active-bank
fidelity, owner listening, combat integration or hardware gates.
