# RUNTIME-18 layers and frame time Implementation Plan

> Execute inline with superpowers:executing-plans; fresh whole-diff review at the end.

**Goal:** Resolve the visible rope layering defect and improve measured frame time from a680f7a without changing gameplay.
**Spec:** User's new video and report: lower/right ropes overlap incorrectly and combat remains exasperatingly slow.
**Architecture:** Preserve original sprite order and ROM-selected video priority; map it correctly to Saturn. Measure SH-2 stages first, then optimize only the demonstrated transport/render bottleneck while proving identical output.
**Tech Stack:** C11, ROM/Musashi, Yaul SH-2, Mednafen, FRT timing.

## Global Constraints
- sources/ remains read-only. Continue existing runtime18/direct-combat, no frontend/audio changes, no provisional gameplay or forced outcome.
- HOST/SH-2 success does not establish pixel-perfect hardware behavior. No universal ROM-EXACT or physical-Saturn claim.
- Separate measured VBlanks/frame and CPU stage time from host emulator wall speed.

## Review Focus
- ROM priority7C must allow background-plane ropes to occlude sprites;7B/78 have different ordering.
- Sprite tile/record order, transparent pen0 and color15 must survive all optimizations.
- Cache residency must preserve exact bytes and cold reads must not be silently skipped.
- Ring scroll wrap, dirty overlays and alternate channels must remain correct.
- Frame logic, D0–D7/RNG and true match endpoints must remain unchanged.

### Task 1: Diagnose on SH-2
- [x] Inspect video and ROM priority writes; contrast board video semantics with primary MAME implementation and original workpack26.
- [x] Build a separate diagnostic entry with FRT stage timing. Measure deterministic30–120 baseline in Mednafen without modifying actor/game decisions.

### Task 2: Correct composition and bottleneck
- [x] Add a failing test of actual Saturn priority configuration using independently expected visible overlaps for78/7B/7C, then fix the demonstrated mapping.
- [x] Add failing/characterization tests at the measured bottleneck; optimize the exact operation without changing its output. Re-measure identical sequence.
- [x] Run complete gameplay regressions, affected ROM matrices, core tests and sanitizers.

### Task 3: Verify and deliver
- [x] Compare visual overlaps and complete-match behavior in Mednafen; obtain fresh review.
- [x] Build normal/replay BIN/CUE, document before/after measurements and limitations, commit and package reproducibly.

Execution evidence: `validation/RUNTIME18-LAYERS-PERF-CHECKPOINT.md` and checkpoint-layers-perf/execution-ledger.md in the delivered package. Branch retained locally as requested; no push/merge.
