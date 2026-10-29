# WP02 R02 audio hardware investigation

## Finding and confidence

R01 contains a reproducible service-cadence defect: the streamer refills exactly one 1,024-sample block per main-loop service call. That policy cannot sustain the fixed 44,100-sample/s SCSP stream if the scene renders once every two NTSC refreshes. With the actual R01 streamer and actual codec linked into a hardware-boundary simulation, it stops at the first ring wrap, 768,896 µs after simulated playback begins, after completing only 11 of the required 16 blocks.

This is a strong explanation for the supplied hardware recording, but the recording does not independently measure physical render cadence. It does establish successful initial BGM playback followed by the red failure screen around the first ring wrap. The parent's untouched-R01 full-cache emulator run did **not** reproduce the physical stop: at 12.335765 seconds of scene time it reported 740 renders, zero missed refreshes, 517 refills, zero failures, and 4,868 µs maximum service duration. Cache mode is therefore not established as the cause. The physical failure branch remains to be confirmed by the new diagnostic record.

The isolated elapsed-gap correction below removes this reproduced defect while preserving one block per normal 59.826 Hz call. It passes the complete original 29-second PCM stream at normal, alternate-refresh, and intermittent missed-refresh service patterns.

All changes made by this investigation are inside `wp02_r02/research_audio_r02`. No project, baseline, or root work source was edited.

## Local source evidence

Paths below are relative to the immutable `wp02_r02/baseline/WP02-Saturn-TAG-R01` archive recovery.

| Evidence | Location and consequence |
| --- | --- |
| `fatal(code)` disables NBG0 and sets the back color red. | `source/scene/src/main.c`, `fatal()`. VDP1 actors, names, and foreground can remain visible over red. |
| Failed `wp02_audio_service()` calls `fatal(14)`. | `source/scene/src/main.c`, scene loop. The supplied image is consistent with this deliberate failure path. |
| BGM uses a 65,536-byte PCM16 ring split into two 32,768-byte halves. | `source/research_audio/integration/wp02_bgm.h` and `wp02_bgm_load()`. There are 32,768 samples per ring and 16,384 per half. |
| Every block decodes 1,024 samples, and the R01 service has a single `if (filling)` decode. | `source/research_audio/integration/wp02_bgm.c`, `wp02_bgm_service()`. Sixteen successful calls are necessary for each inactive half. |
| A half transition while `filling` is still set calls `fail()`. | Same function. This is the reproduced first-wrap stop. |
| `fail()` stops the music and returns false. | Same file. This propagates to the scene's red failure screen. |
| Compressed music is fully resident before playback. | `wp02_bgm_load()` fills LWRAM and HWRAM overflow. Runtime cadence failures are not inherently CD/MODE transfer failures. |

The relevant fixed timing follows directly from those source constants:

- One block: `1024 / 44100 = 23,219.95465 µs`.
- One half: `16384 / 44100 = 371,519.27438 µs`.
- One whole ring: `32768 / 44100 = 743,038.54875 µs`.
- Minimum sustained one-block service rate: `44100 / 1024 = 43.06640625 calls/s`.

In the alternate-refresh simulation, the first half transition is observed at refresh 24, 401,163 µs. R01 fills one block on refreshes 24 through 44, eleven blocks total. At refresh 46, 768,896 µs, the monitor reports the wrapped half and the still-incomplete fill causes failure. The already-played samples still match the original PCM at that point; the fatal guard trips before the unwritten portion is read.

## Recording agreement and its limit

The video-analysis agent measured the supplied `upload/IMG_4358.mp4` at 30 camera frames/s:

| Observation | Recording time |
| --- | ---: |
| Last blank frame, frame 651 | 21.700000 s |
| First complete scene, frame 652 | 21.733333 s |
| Last normal ring frame, frame 673 | 22.433333 s |
| First red frame, frame 674 | 22.466667 s |
| BGM onset from source-waveform correlation | approximately 21.696780 s |

The nominal simulated failure offset added to the measured audio onset gives `21.696780 + 0.768896 = 22.465676 s`, within the first-red camera-frame boundary. This is supporting timing evidence, not a measurement of the physical loop's phase or cadence. The phone footage is dominated by CRT banding, title occlusion, and camera motion, so it cannot establish 30 Hz rendering by itself. Supporting measurements are in `wp02_r02/hardware_video_analysis`.

## SCSP interpretation checked against primary authority

The Sega *Saturn SCSP User's Manual*, ST-077-R2-052594, printed page 89 (PDF page index 100), defines CA as an offset from SA in units of **4,096 waveform samples**. Its common-register map (PDF page index 39) places MSLC in bits 15:11 and CA in bits 10:7. R01's `target << 11`, `(*monitor >> 7) & 15`, and `ca >> 2` half selection agree with that definition. PCM16 does not justify doubling CA into byte units. [Sega manual](https://antime.kapsi.fi/sega/files/ST-077-R2-052594.pdf)

Mednafen 1.32.1's SCSP implementation also builds CA from its sample address before the PCM8/PCM16 fetch branches. The R01 selection delay of at least 25 µs accommodates a sample-period monitor latch. There is no evidence here for changing the monitor address, slot shift, CA units, or loop endpoint. [Mednafen SCSP source](https://sources.debian.org/data/main/m/mednafen/1.32.1%2Bdfsg-2/src/ss/scsp.inc)

SH-2 assembly generated from the recovered SDK confirms that the live `icsr_target` descriptor field is read as a byte each service, and the SCSP monitor is read as a word. Saved inspection outputs are `wp02_bgm_r01.s` and `ponesound_min_r01.s`. The descriptor's pointed-to type is volatile; no stale compile-time slot value was found.

## Emulator performance check and remaining uncertainty

Mednafen's Saturn documentation says the default SH-2 cache mode is data-only, and explicitly cautions that emulated games generally have less slowdown than physical Saturn games. Its earlier R01 launch did not specify a full-cache override. A normal-cadence emulator pass therefore does not establish physical refill headroom. [Mednafen Saturn documentation](https://mednafen.github.io/documentation/ss.html)

The 1.32.1 source includes an undocumented, nonpersistent `ss.dbg_cem` override outside the developer-build guard. The enum value `full` forces instruction-and-data cache emulation. Run with `-ss.dbg_cem full` and verify the startup log says `CPU Cache Emulation Mode: Full`. Parent integration testing is using this stricter mode. [Mednafen ss.cpp](https://sources.debian.org/src/mednafen/1.32.1%2Bdfsg-2/src/ss/ss.cpp/)

The parent ran the untouched delivered R01 disc with Mednafen 1.32.1 and verified that log line. This run continued normally beyond twelve seconds of scene time, with the metrics given above. I read the resulting `wp02_r02/verification/r01-full-cache.json` and log. This disproves any claim that selecting full cache alone reproduces this incident. The cadence test establishes a real refill-policy limitation; its applicability to this particular physical stop is still an inference from the recording's timing.

## Small correction tested in isolation

The initial budget-only experiment is `wp02_bgm_gap_budget_probe.c`, with a two-hunk diff in `gap_budget_candidate.patch`. The final candidate requested by the parent is `candidate/wp02_bgm.c` and `candidate/wp02_bgm.h`, adding the same budget and the compact first-failure diagnostics described below. Its full diff is `candidate.patch`.

1. Keep the existing 371,519 µs late-service rejection.
2. Set the per-call decode budget to one block.
3. If elapsed time since the previous service is at least 23,220 µs, permit two blocks, except in the final 4,096-sample CA bucket of the current half, which retains one block.
4. Stop immediately when the inactive half is full, as before.

The 23,220 µs threshold is the ceiling of one block's playback duration. It gives a normal 16.7 ms call R01's existing cost and compensates for a 33.4 ms call. At the simulated alternate-refresh cadence the candidate finishes the first inactive half at refresh 38, 635,175 µs, ahead of the 743,039 µs physical consumption boundary.

This correction does not change any audio asset, decoded sample, source ordering, pitch, volume, logical voice, ring geometry, or CA interpretation. It is deliberately bounded to two blocks, not a general recovery algorithm for arbitrarily long stalls. Separately, moving audio service after VDP1 submission can overlap SH-2 decode work with rendering; that integration change needs root's visual/audio timing checks.

A later duration review found and contained an edge in the initial candidate: after a permitted long service gap, a second block could cross a half boundary while using the earlier CA observation. The final candidate caps the last-bucket budget at one, preserving R01's one-block policy there. A focused duration regression proves the added second-block risk is contained in that case, and separately documents the inherited one-block window. Full details and limits are in `SERVICE-TIMING-REVIEW.md`.

## Reproducible red/green verification

Run from the workspace root:

```sh
python3 wp02_r02/research_audio_r02/run_gap_budget_probe.py
```

The runner compiles the actual baseline and candidate streamer against the same actual codec with `cc -std=c11 -O2 -Wall -Wextra -Werror`. It only replaces the hardware boundary: CD sector reads, SCSP RAM, slot allocation, and the advancing CA. The source PCM is the immutable R01 `WP02BGM.decoded.pcm`.

Fresh output:

```text
r01: PASS cadence=1 samples_verified=1278900 refills=1234 runtime_disc_reads=0 max_service_blocks=1
r01: FAIL cadence=2 refresh=46 time_us=768896 played=33908 refills=11 failures=1 first_mismatch=4294967295
r01: PASS cadence=3 samples_verified=1278900 refills=1234 runtime_disc_reads=0 max_service_blocks=1
gap_budget_probe: PASS cadence=1 samples_verified=1278900 refills=1234 runtime_disc_reads=0 max_service_blocks=1
gap_budget_probe: PASS cadence=2 samples_verified=1278900 refills=1236 runtime_disc_reads=0 max_service_blocks=2
gap_budget_probe: PASS cadence=3 samples_verified=1278900 refills=1234 runtime_disc_reads=0 max_service_blocks=2
PASS resident BGM: every emitted PCM16 sample, ring seams, inactive writes, cached reload, no runtime CD, delayed monitor, late-service stop
```

Cadence 1 is every NTSC refresh; cadence 2 is every second refresh; cadence 3 skips one refresh every ninth service call. Each passing cadence verifies all 1,278,900 original samples, including ring seams and the finite final block. Every simulated SCSP ring write is asserted to touch the currently inactive half, with the existing two-byte endpoint guard treated separately. No playback-time disc reads occur. The original baseline `tests/test_bgm_stream.c` runs unchanged against the candidate and retains its delayed-monitor, reload, slot-loss, and excessive-gap assertions.

`gap_budget_probe_results.json` contains the exact build/run commands and SHA-256 values for baseline source, candidate source, pack, and PCM. The runner treats the original alternate-refresh failure as the expected red test and exits zero only when all expected results hold.

Limits: this host simulation does not model SH-2 execution duration or bus contention. Its inactive-half assertion applies at each simulated service instant. Integrated full-cache emulation checks real emulated decode duration; a physical rerun is still needed to establish the final console result.

The final candidate, including diagnostics, was then verified with:

```sh
python3 wp02_r02/research_audio_r02/run_candidate_tests.py
```

This exits zero after all five original host suites pass, all five complete-PCM cadence cases pass (59.826 Hz, 29.913 Hz, mixed missed refreshes, 45 Hz, and 50 Hz), the ten diagnostic failure reasons and reset/immutability assertions pass, the duration-edge regressions pass, original R01 retains the expected alternate-refresh red result, and the actual SH-2 SDK successfully compiles the final candidate. Exact commands, candidate hashes, and all outputs are in `candidate_test_results.json`. The SH-2 assembly in `candidate-tests/wp02_bgm_sh2.s` retains one byte read of the live slot and one word read of the monitor on its normal path.

## Implemented compact first-failure diagnostics

The candidate replaces undifferentiated `fail()` calls with a reason and the values already sampled by the service. It distinguishes existing paths without changing their conditions:

| Value | Reason | Existing path |
| ---: | --- | --- |
| 1 | `LOAD` | Invalid request, CD read failure, or invalid file header |
| 2 | `DECODE` | A source block cannot be copied, decoded, or validated |
| 3 | `SERVICE_GAP` | Time since prior call is at least one half |
| 4 | `SLOT_TIMEOUT` | Initial allocation has not completed within 100 ms |
| 5 | `SLOT_LOST` | An already observed physical slot becomes negative |
| 6 | `SLOT_INVALID` | Physical slot exceeds 31 |
| 7 | `SLOT_CHANGED` | A monitor-ready playing voice changes physical slot |
| 8 | `MONITOR_RANGE` | CA exceeds seven for this ring |
| 9 | `REFILL_LATE` | A half changes while the previous fill is incomplete |
| 10 | `SOURCE_EXHAUSTED` | Playback leaves the finite final source half |

Value zero means no failure. The combined `filling || (saw_eof && active_half == eof_half)` branch is split so late refill and source exhaustion have separate reasons.

Before `wp02_bgm_stop()` clears state, the first failure populates one 24-byte record. Four 32-bit fields hold service gap, source sample count, source offset, and refill count; four 16-bit fields hold raw monitor, fill bytes, signed target, and reason. It has no per-service snapshot writes. `raw_monitor` is `0xffff` if the failing call did not read the monitor, and target is `-1` when unavailable. Fill bytes is zero when there is no unfinished fill. Service gaps too large for 32 bits saturate. A `LOAD` failure reports zero source/refill counts so a previous track's counters are not misrepresented as this load's progress.

`wp02_bgm_failure_reason()` and `wp02_bgm_failure_snapshot()` read only the frozen RAM record. The latter returns a const pointer. `wp02_bgm_reset()` clears it. There are no extra SCSP reads or driver changes. Parent integration adds the visible failure record; the original `dbgio` device is Mednafen-only.

The fault test exercises all ten reasons, including a malformed runtime block, a sustained unsupported 10 Hz service cadence, and end of the finite padded source. It checks that the late-fill record contains 14,336 unfinished bytes, seven refills, a 100,000 µs service gap, target six, CA zero, and the exact source position. Repeated service/stop/start calls preserve the first record; reset clears it.

## Investigated alternatives without causal evidence

- **Startup readiness:** the pinned driver publishes a ready signature after initialization, but the recording's successful source-matching playback through the first ring makes startup failure a poor fit for this event. No startup rewrite is proposed.
- **Descriptor access widths:** the Sega manual documents 16-bit main-CPU sound-space accesses. The inherited poneSound implementation has byte descriptor accesses, also present upstream. That deserves separate driver-level treatment if observed evidence points there; it does not explain the reproduced cadence defect and is excluded from this correction.
- **Loop endpoint:** no primary evidence established that R01's endpoint/guard must change on hardware. Altering it would change transport timing and add a second hypothesis.
- **Decoder/source memory:** both resident source regions, all stream blocks, and the entire decoded PCM pass the host verification. Hardware RAM corruption has not been demonstrated.

The tested narrow recommendation is the elapsed-gap refill budget, with the parent's render/decode scheduling correction and full-cache integration verification. Preserve the existing transport and original content while collecting a specific failure record if a console still stops.
