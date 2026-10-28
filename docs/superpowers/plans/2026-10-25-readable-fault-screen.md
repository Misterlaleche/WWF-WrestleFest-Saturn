# Readable Saturn Fault Screen Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Make a physical Saturn fault photo sufficient to locate a failed action and its context.

**Architecture:** Copy a compact, immutable runtime snapshot at failure. Render four text pages directly into the existing NBG0 bitmap and navigate them with left/right after gameplay is stopped.

**Tech Stack:** C11, Yaul, Saturn VDP2, existing native build and disc validator.

**Spec:** `docs/superpowers/specs/2026-10-25-readable-fault-screen.md`

## Tasks

- [ ] Replace the unlabeled number renderer in `src/runtime/include/wf_r18_fault.h` with a bounded bitmap font and four page renderers; add a host test for the first page and display bounds.
- [ ] Capture the dispatch trace, actor and target state, all nine actor summaries, pad and frame data in `src/runtime/src/wf_saturn_entry.c`; freeze the snapshot and allow left/right page navigation.
- [ ] Build with the Saturn toolchain, check HWRAM headroom, validate an exact BIN/CUE, and inspect the fault screen in an emulator before handing it to a physical Saturn test.
