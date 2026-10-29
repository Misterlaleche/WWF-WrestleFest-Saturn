# Integrated R02 review

Reviewed on 2026-10-29. Production and baseline files remain read-only for
this reviewer. A native review build, map, disassembly, and source hashes
are stored in `integrated_r02`.

## Final disposition

The final guarded source and exact release/reference ELFs have no remaining
concrete integration blocker from this review. The final source hashes,
symbols, and disassembly are captured in `final_guarded_r02`. The normal
full-disc integrated emulator capture remains the coordinator's active
verification gate; physical-console success has not been established.

The earlier all-zero-metrics capture was traced to an incomplete disc BIN,
not a startup or heap regression. Its exact state and executable evidence
are recorded in `INCOMPLETE_DISC_FORENSICS.md`.

## Resolved added-window finding: second-block boundary containment

The integrated adaptive service reads CA once, then may decode/copy two
blocks. It does not revalidate the active half between those operations.
R01 already had a one-block stale-monitor window; the new second block
extends it. The standard service-cadence tests assign no elapsed time to
decode/copy, so their inactive-half assertions do not exercise this case.

I asked the audio reviewer for a focused real-code timing test. Their
`research_audio_r02/test_bgm_service_duration.c` links the actual baseline or
candidate streamer and codec, and advances the modeled playback clock by
4 ms at each decoded block's SCSP-copy boundary. After prior calls at
200 ms and 400 ms, the next call begins at 738 ms. Its 338 ms gap is still
below the existing 371.519 ms gap rejection threshold.

| Variant | Call finishes | Writes into the active half |
| --- | ---: | ---: |
| R01, one block | 742 ms | 0 |
| Initial R02 adaptive candidate, two blocks | 746 ms | 1 |
| Final R02 candidate, last-bucket cap | 742 ms | 0 |

The actual ring boundary in this model is at 743.03855 ms. The second copy
crosses it. I independently read the saved result in
`research_audio_r02/service_duration_results.json`.

This is a demonstrated edge case in the added delayed-service handling,
not a claim that it occurs during steady 30/60 Hz operation. Those normal
schedules finish this refill around 635 ms and have ample margin. It also
does not identify the cause of the user's original physical failure.

The final integrated source declines the *second* block when sampled CA is
in the final bucket of the active half: `(ca & 3u)==3u`. I verified this in
source and final SH2 instructions: `and #3`, `cmp/eq #3`, then the branch
sets the budget register to one. It uses the existing monitor observation
and adds no register read.

The audio reviewer's final results show zero active-half writes in the
738 ms counterexample and full sample equality at 59.826 Hz, 29.913 Hz,
mixed missed refreshes, 45 Hz, and 50 Hz. I read the final result file and
checked that its candidate hash matches both integrated source copies.
The 45/50 Hz tests matter because sixteen one-block refills can legitimately
use the last bucket; a blanket fatal guard there would reject those rates.

A separate 740 ms call with the same 4 ms modeled block duration still
crosses the boundary for both R01 and final R02. This is the inherited
one-block stale-monitor window, retained explicitly by the chosen minimal
containment. Allowing a second block only in earlier buckets supplies at
least 4,096 samples, about 92.88 ms, of sampled time to the boundary. This
is useful margin, not a universal proof without a physical bound on
decode/copy/interrupt duration. This reviewer made no production edit.

## Main loop and error handling

The integrated main loop matches the reviewed overlap proposal. It keeps
the elapsed-time sample after the two initial sync waits, rejects an already
latched audio error before controller/scene progression, submits the next
VDP1 frame, records current scene counters, then runs BGM service.

The exact linked R01 proof of asynchronous rendering still applies: the
production graphics routines are unchanged and the same SDK is linked.
The loop does not mutate its VDP1 command list while the previous frame is
outstanding. The decoder works in its own CPU buffers and SCSP ring.

Normal completion still stops only BGM before the clear-presenter render;
voice/SFX tails retain their timer and VBlank service. START still exits
before the next scene/render and follows the existing audio stop/reload
path. First-frame service can begin its slot-acceptance timer one iteration
earlier, as documented in `PIPELINE_PROPOSAL.md`; verify the actual startup
recording and diagnostic injection.

## Fault record survives audio shutdown

The SH2 linker reports:

- `_wp02_fault`: 68 bytes (`0x44`).
- `_last_failure`: 24 bytes (`0x18`).
- `_wp02_diag_commands`: 8192 bytes (`0x2000`).

The integrated `fatal()` copies the BGM failure record and stores all fault
fields before calling `wp02_audio_stop()`. The exact generated assembly
confirms those stores precede the stop call, including the source position,
target, maximum service duration, and display words. The local display words
remain valid after the BGM reset clears its internal snapshot.

`sound_ready=false` is also emitted before the stop call. `audio_stop()`
masks interrupts during teardown and explicitly flushes the cease commands,
so suppressing subsequent audio VBlank callbacks does not prevent that stop
request from being sent.

The new diagnostic waits for outstanding VDP1 work before uploading its
own command list. It uses the existing 256-command VDP1 reservation, with a
244-command worst case. All drawn geometry stays inside 320x240. Its mode-2
CRAM entry is valid for the existing palette-only sprite configuration.
The injected-error capture has now been verified by the coordinator. I also
independently inspected `diagnostic-injection-screen.png` and the associated
JSON. The visible rows are exactly `0E090040`, `5800000B`, `002E0000`, and
`000012EF`: code 14/reason 9/raw monitor 64; 22,528 unfinished fill bytes and
11 refills; 46 renders and zero missed VBlanks; 4,847 us maximum service.
Those match the frozen fault record after audio shutdown. This establishes
the emulator diagnostic display and packing, while physical CRT readability
and hardware failure cause still require the console test.

Two diagnostic qualifications are nonblocking:

1. An asynchronous audio error with no BGM failure has reason zero and an
   unavailable raw monitor (`0xffff`). The failure record does not include
   `audio_state` or last sound command, so those cases should not be described
   as proved BGM failures.
2. Preload failures during a later START/restart can retain the prior run's
   elapsed/render counters because those counters are reset after loading.
   Code/reason still identify a startup/load failure. If exact per-attempt
   timing is needed, reset the diagnostics at outer-loop entry rather than
   interpreting those counters as the duration of the failed preload.

## Native memory and source integrity

An independent SH2 build of the initial integrated source succeeded using
the same compiler/specifications as R01. This reviewer did not run
`build.py`, which would refresh production source copies. Instead,
object/ELF output went only to `native_review/integrated_r02`. After the
last-bucket guard was integrated, I inspected the coordinator's exact final
release/reference ELFs and saved their symbols, disassembly, and hashes in
`final_guarded_r02`. The table below describes that final release ELF.

| Item | Result |
| --- | ---: |
| Text | 117780 bytes |
| Initialized data | 312 bytes |
| BSS | 849376 bytes |
| Linked end | `0x060F0334` |
| Space to HWRAM end | 64716 bytes |
| Space after 8192-byte runtime command payload | 56524 bytes before allocator overhead |

The final release still ends at the same address after alignment. The
reviewed release ELF SHA-256 is
`eb5467349d307bab7e8123480e34d7cea7b90473a812db46a3aad739b654e947`;
the reference ELF is
`06c710b150ab354930e20f0b51411f532625721ce33a792f28c02cc3e3f5d1ad`.

The added diagnostics fit without changing the LWRAM reserve, SCSP ring,
VDP1 texture allocation, or the original stack placement. The command heap
and SDK's static private pool remain distinct.

The reviewed scene copies of `wp02_bgm.c` and `.h` match the audio reviewer's
candidate hashes:

- C: `e2fbc37fd2c5f693d8215219a0de913c418770d0da00c498dec7511709bcbb59`.
- Header: `c76369e3e49481884851db4d55286153d20554968d99ce1e61c8b9ca397df92f`.

The scene and research integration copies match each other. `wp02_audio.c`,
`wp02_audio.h`, `wp02_scene.c`, and `wp02_assets.c` remain byte-identical to
the immutable R01 baseline. `build.py` now labels release and reference
artifacts R02 and does not reference a prior working repository.

## Full-cache baseline result

The untouched R01 full-cache run did not reproduce the physical error:
12.335765 s, 740 renders, zero missed VBlanks, zero BGM failures, and 4868 us
maximum service duration. I read the saved JSON directly. **Cache mode is
not established as a cause**, and no claim of a physical fix follows from
either the old or new emulator result.
