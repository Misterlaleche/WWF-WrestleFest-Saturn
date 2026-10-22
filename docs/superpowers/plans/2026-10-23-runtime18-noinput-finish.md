# RUNTIME-18 no-input finish Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans inline.

**Goal:** Continue3435a09 through the real end of the no-input match.
**Architecture:** Translate only reached ROM branches into the existing native owners. Compare each fix against the unmodified MC68000 ROM before advancing integration.
**Tech Stack:** C11, Musashi oracle, SH-2 GCC/Yaul, Mednafen.
**Spec:** Latest user request: resolve19274/raw59 with direct ROM evidence, complete the no-input sequence, no provisional code or frontend.

## Global Constraints
- sources/ remains read-only. Continue the named existing runtime18/direct-combat checkout.
- No forced actors, stamina, CPU choices, result or timer. Unknown dependencies remain explicit until implemented with evidence.
- HOST success is separate from Saturn execution; never claim ROM-EXACT or physical-hardware suitability without proof.

## Review Focus
- A released/null link must not block animation frames that do not dereference A2.
- Actual A2 writes to ROM must be ignored; no synthetic actor substitutes.
- Distinguish literal null sentinel7F000 from tagged physical zero and preserve D0-D7.
- Subsequent CPU/action dependencies require a failing ROM comparison before implementation.
- Completion must use the actual11B6 result boundary, not a frame-budget success.

### Task1: Resolve raw59 linkage
Files: src/runtime/src/wf_r17_actions_r11.c; new tests/r18/raw59_oracle_bridge.c and tools/verify_raw59_links_rom.py; src/runtime/Makefile.
- [x] Add full bounded-dispatch oracle cases for null, tagged-null, real and unused unmapped links; init, idle animation, release and terminal phases. Run and record RED19274.
- [x] Translate only the ROM-observed read/write branches and run the oracle to GREEN.
- [x] Run tag-finish regression and the no-input integration probe; record the next result or dependency.

### Task2: Follow no-input integration
- [x] Repeat direct ROM disassembly, RED test, implementation, GREEN for each actually reached dependency until a real match endpoint appears.
- [x] Add strict no-input completion and ASan/UBSan targets; retain the controlled replay regression.

### Task3: Validate and deliver
- [x] Run full HOST suite, affected/original ROM matrices and sanitizers; obtain independent whole-diff review.
- [x] Build normal and deterministic no-input SH-2 discs; run Mednafen to the actual endpoint and retain exact binary hashes.
- [x] Prepare current-state docs and reproducible source/assets/discs/evidence; rebuild the complete match from the exported source. Final commit/package identifiers are recorded by CHECKPOINT.txt and the delivered hashes.
