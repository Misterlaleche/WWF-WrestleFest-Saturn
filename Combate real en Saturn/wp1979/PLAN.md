# WP1979 Implementation Plan

> Agentic execution: native implementation by parent, independent tooling/oracle/host audits by delegated agents, followed by a fresh completion review. No commits or repository writes are authorized.

**Goal:** deliver a playable native four-wrestler TAG fixture and demonstrate its real emulated Saturn throughput against original arcade behavior.

**Architecture:** isolated pinned runtime18 source, direct original TAG bootstrap, VDP1/VDP2 presentation and native actor scheduler. Original ROM assets and an explicit diagnostic input sequence provide reproducible comparisons.

**Tech stack:** SH-2 GCC 13.2, Yaul, Python host validation, MAME oracle, Saturn emulator and FFmpeg.

**Spec:** DESIGN.md.

## Global constraints
- All outputs under wp1979. Original project ZIPs and GitHub remain read-only.
- Do not invent missing gameplay or make full ROM-exact/hardware claims from partial evidence.
- Desired lineup: actor IDs 0, 1, 9, 7, respecting TAG active/reserve ownership.
- Deliver emulator video and raw MODE1/2352 BIN/CUE; retain reproducible source and audit evidence.

## Review focus
1. Native game rate versus emulated video rate: distinguish fast emulation, repeated displayed frames and real logic cadence.
2. Four selected graphics/palettes and reserve actors: do not confuse selection ID and composition ID.
3. Framebuffer erase/swap and VRAM transfer scheduling on physical Saturn.
4. Cache/stack/asset bounds, CD sector rounding and unaligned DMA.
5. Unsupported gameplay branches under the actual demonstration and live controller input.

## Tasks
- [x] Establish pinned source snapshot and executable build tools; verify input hashes and SDK execution.
- [x] Native fixture: seed search through original bootstrap, ROM displacement/animation tests, sustained host combat and input schedule. Existing source behavior is tested before changing fixture/timing code.
- [x] Build and optimize independent Saturn presentation. Measure baseline before fixes; add focused regression tests for specific defects and verify changed behavior.
- [x] Boot and inspect actual emulator output; capture original and Saturn timing evidence and correct discovered faults. Candidate4 completes the match and controller capture; independent final review passes.

Delivery closure is recorded by `package_delivery.py` in
`release/wp1979-verificacion.json` and `release/SHA256SUMS.txt`: archive CRCs,
each archived source hash, closed-file copies, exact disc identity and final
video/audio stream metadata. These are the authoritative packaging receipts.

## Ledger
- WIP160 reviewed as historical reference; current native runtime18 selected because it has original owner translations instead of provisional TAG logic.
- Decimal arcade refresh is an emulator reference, not a claim of measured original PCB frequency.
- Physical Saturn validation remains pending the user's hardware test.
- Final whole-match cadence, including game frame0: 7,372 native steps/draws over7,376 VBlanks, 59.967462 FPS; minimum rolling1/5/10-second rates58.0/59.6/59.8. Both legal TAG exchanges, original endpoint7372, native sound, zero fatal owners and zero combat CD reads.
- Final controller route after the initial30 ticks:530 steps/draws over530 VBlanks,60.0 FPS. From tick0 it is560/562,59.786477 FPS. No fatal owner or combat CD read. The first513 completed states reproduce361,152 audited actor bytes exactly.
- Yabause's software VDP1 draw completion is not a physical raster/bus timing model. These figures establish the emulated native cadence, not a hardware guarantee.
