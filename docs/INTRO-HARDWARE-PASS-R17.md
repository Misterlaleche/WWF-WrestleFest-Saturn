# INTRO Hardware-Pass Handoff R17

Date: 2026-09-12

## Status

Canonical external INTRO checkpoint accepted by Integration Master.

User-confirmed hardware result:
**PASS on real Sega Saturn NTSC-U + Terraonion MODE** for the exact package/binary identified below.

This hardware confirmation supersedes the package's pre-test `hardware_boot_test: false` metadata; that metadata was generated before the user's real-hardware test.

## Package identity

Package:
`WF-SATURN-INTRO-ROMEXACT-FINAL-HWTEST(1).zip`

Package SHA256:
`0fb488514ae233c429058834b8ced73c757b0224d819d6ba4dc3015042f81c98`

BIN:
`wrestlefest-saturn-intro-romexact-final.bin`

BIN SHA256:
`c6f0b5817969fae5ac476633a1349328ddcbf777697f8eae42d248357bccd862`

CUE SHA256:
`64eea33a674dcdeda3ca7f838521a8734e854d8c89f561a2bdfe163674066562`

Main 68000 authority SHA1:
`a726c46f350723ea73158bc78055d17d418b14e2`

Canonical INTRO assets:
- INTRO.RLE SHA256 `16452b1bfd5559ff6ea33ec265e60c794763deba61f7492de121147afcffd3dc`
- TITLE.ANM SHA256 `982098f0772adc3b6bdbb2e77794037e550a473cb25d55c83b4947ff48ac9c6c`
- TITLE.ANM size = 946452 bytes
- TITLE_COUNT = **451 stored asset frames**

## Critical clarification about 451

**451 is NOT the native duration of the arcade INTRO/TITLE.**

451 is only the number of stored frames in the reconstructed visual asset `TITLE.ANM` used by the Saturn implementation.

Therefore:
- do not convert 451 into an arcade scene duration;
- do not document "the arcade intro lasts 451 frames";
- do not derive native timing from 451 alone;
- timing authority remains the ROM-derived/source-sync cadence and the recovered event spacing.

The hardware-tested block deliberately walks the 451 stored frames once because that is the recovered asset representation.

## Hardware-proven visible sequence

`FBI -> Legal -> Technos/Tecmo -> black transition -> moving starfield -> sequential 12-wrestler build -> WWF WrestleFest logo/copyright -> recovered final INTRO/TITLE visual state`

The user reports the sequence works perfectly on real hardware.

## Timing contract

Boot plates:
- hold = 128 source syncs;
- common fade = 8 levels x 3 source syncs = 24 source syncs.

Source timing is not mapped as 1 arcade sync = 1 Saturn VBlank.
The block uses the demonstrated 57.445-Hz logical source cadence on NTSC Saturn via 57445/60000 accumulation.

TITLE.ANM visual cadence:
- source progression numerator: 57445;
- stored-frame denominator: 120000;
- equivalent to one stored asset frame per two source syncs;
- 21 stored asset frames per title object = 42 source syncs per object.

Again, the 451-frame asset count is a storage/reconstruction count, not an arcade-native duration statement.

## Saturn implementation facts

Hardware-tested harness uses:
- TITLE.ANM preloaded once into LWRAM;
- hash validation before playback;
- LZ4 decode to HWRAM buffer;
- XOR reconstruction of the accumulated title bitmap;
- VDP2 NBG0 double-page presentation;
- DMA for bitmap/CRAM updates;
- no per-frame CD streaming;
- VDP1 not used by this block.

These are proven implementation facts for the hardware-tested block, but Integration Master must transplant the behavior into the master ownership model rather than copy the isolated harness architecture blindly.

## Critical boundary: FRONTEND owns CREDIT / INSERT COIN

The INTRO source contains **no coin, credit, START, MODE or prompt logic**.

The previous experimental Test05 behavior:
- Saturn A = Coin A;
- private local credit counter;
- START -> MODE;
- static MODE jump;

is explicitly rejected and must never be reintroduced.

The visible/UI strings:
- `CREDIT:x`
- `INSERT COIN`
- START/credit prompt state

belong to **FRONTEND**, not INTRO.

Ownership split:
1. WP19 / R11 owns the authoritative logical credit state and prompt selection.
2. Frontend presentation owns drawing/updating `CREDIT:x`, `INSERT COIN`, START-related prompt presentation and the title/attract interaction.
3. INTRO owns only its boot/title-build visual sequence and hands off to Frontend.
4. INTRO must never maintain a private credit counter or hard-code `CREDIT: 0`.

This ownership is now a master integration invariant.

## Harness-only behavior that must NOT be integrated

The final-frame infinite hold is inspection-only.
It is not arcade behavior.

The master must replace that hold with the exact Frontend/TITLE continuation:
credit/prompt/START/attract/mode flow according to ROM authority.

Do not copy the harness `main.c` into the master.

## Audit

Package SHA256: PASS.
TITLE.ANM hash: PASS.
INTRO.RLE hash: PASS.
BIN/CUE hashes: PASS.
TITLE_COUNT=451 confirmed in package source.
Package source grep confirms no coin/credit/START/MODE logic.
Real Saturn hardware boot/render: PASS by user test.

## Integration state

INTRO visual block: **HARDWARE-PASS**.
Frontend continuation after INTRO: **OPEN / master-owned**.
Credit/prompt presentation: **FRONTEND-owned**.
WP19 credit state/payment: **master R11-owned**.
Full integrated master hardware pass: **NOT YET CLAIMED**.
