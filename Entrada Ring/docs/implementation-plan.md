# WP01 R02 Native Audio Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development or superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Replace the mixed scene recording with six native SCSP FM voices and original individual OKI samples while retaining the original entrance graphics and timing.

**Architecture:** Capture original chip events, translate YM registers to SCSP operators and OKI commands to resident sample voices, and replay register writes with a sub-frame timer. The SCSP generates and mixes all musical operators and sample voices.

**Tech Stack:** Yaul SH-2 C, SCSP registers, Python asset generation, GCC host tests, MAME reference, Mednafen and Yabause validation.

**Spec:** `design.md`

## Global Constraints

- All changes are under `wp01_audio_r02/`; preserve original files, R01 and repositories.
- Preserve the 320×240 NTSC scene and original ROM events.
- No mixed-track or per-musical-channel recordings as runtime audio.
- Use six four-operator FM voices plus four original OKI voice slots.
- Report fidelity limitations and distinguish emulator from physical-console results.

## Review Focus

- Repeated KEYON while active must not restart unrelated slots.
- Signed modulation can address before/after the loop: provide and bound sine guards.
- Restart/STOP must disable the scheduler before replacing its state.
- Timer wrap and delayed interrupts must not alter tempo or reference freed state.
- Audio IRQ work must not introduce missed scene presentations.

## Tasks

- [x] 1. Isolate the source copy and hash all protected inputs. Capture original YM/OKI writes and independently verify decoded samples against MAME.
- [x] 2. Implement and test a callback-based YM→SCSP mapper: operator order, algorithms 2/3/4, feedback, pitch, key gates, envelope mapping, and guarded sine storage. Compare isolated patches in emulation before accepting its output.
- [x] 3. Implement and test OKI command routing, busy voices, sample limits and original volume levels, emitting SCSP register writes to slots24–27.
- [x] 4. Generate a bounded, timestamped native-register asset and resident sound RAM image. Assert all addresses and channels are within the chosen map and account for every meaningful original event.
- [x] 5. Add a timer-driven native scheduler in the isolated source copy. Test ordering, exact repeat/reset, timer wrap and lateness accounting before integrating it with the graphics loop.
- [x] 6. Build R02, capture native emulator sound and video, compare audio with the original, verify graphics/state cadence, and independently review hardware access and worst interrupt bursts.
- [x] 7. Package verified BIN/CUE, emulator video and full independent sources; verify original-file hashes.

No git actions are part of this plan because the user explicitly prohibited modifying repositories.
