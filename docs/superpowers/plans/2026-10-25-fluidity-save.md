# Fluidity and Saturn Save Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Improve sustained combat speed, advance Royal movements, and persist rankings and future options in Saturn backup memory.

**Architecture:** Keep rendering assets and combat logic unchanged except where measured evidence demands a change. Add a portable, versioned save codec and a Saturn BUP adapter, invoked only at frontend reset completion and finished result transitions.

**Tech Stack:** C11, Yaul Saturn SDK, Python asset builder and Beetle Saturn observer.

**Spec:** `docs/superpowers/specs/2026-10-25-fluidity-save.md`

## Global Constraints

- Preserve the stage9z delivery unchanged.
- Use internal Saturn backup memory through BUP; never autoformat.
- No BUP or CD asset loading in the live combat frame loop.
- Preserve exact arcade ranking table layout at RAM offsets C2..145.

## Review Focus

- Fresh backup memory: factory rankings survive and an initial save can be created.
- Corrupt or future-version save: reject it without overwriting factory rankings.
- Full backup memory: show a recoverable status and keep the game playable.
- Cold Royal sprite: stays within the bounded cache and emits a valid command.
- Existing TAG match: retains scene and progression after the Royal asset change.

---

### Task 1: Royal resident asset and speed

**Files:** Modify `src/runtime/audit/runtime18-cd/ROYSPR.BIN` (generated); test with `src/runtime/tests/r18/verify_hot_sprite_dictionary.py`; record metrics in `validation/local-20261025`.

- [ ] Confirm old pack's tile count and Royal trace misses; write the measured baseline.
- [ ] Regenerate R18D with `build_r18_hot_sprites.py` from the current manifest.
- [ ] Run pack verification and the same Royal observer route; compare VBlanks and CD reads.
- [ ] Record resident bytes, cold slots and any unused atlas IDs.

### Task 2: Royal movement continuation

**Files:** `src/runtime/src/wf_r17_raw23.c`, associated binding/action files, focused ROM oracle and route test.

- [ ] Reproduce frame6374 block and identify exact original-ROM branch and state.
- [ ] Add a failing focused test for that branch.
- [ ] Implement the single branch and pass the focused test and a longer Royal observer.

### Task 3: Portable save codec

**Files:** Create `src/runtime/include/wf_r18_save.h`, `src/runtime/src/wf_r18_save.c`, and `src/runtime/tests/r18/test_save.c`.

- [ ] Test defaults, ranking round trip, bad checksum/version/length, and option bounds; observe failure.
- [ ] Implement a fixed-size, versioned record with checksum; observe all tests passing.

### Task 4: Saturn backup adapter and lifecycle

**Files:** Create `src/runtime/src/wf_r18_save_saturn.c`; modify frontend driver hook, entry and Saturn Makefile; add host lifecycle test.

- [ ] Test load after factory ranking reset and save after results; observe failure.
- [ ] Implement BUP initialization/load/write/verify outside combat, retain nonfatal unavailable status.
- [ ] Run host tests, Saturn link, HWRAM budget and a boot/flow emulator route.

### Task 5: Final verification

- [ ] Run targeted ROM comparisons and combat/render tests.
- [ ] Validate disc sectors and payloads for an internal next-build candidate.
- [ ] Report measured gains, movement coverage, save status, and remaining physical checks.
