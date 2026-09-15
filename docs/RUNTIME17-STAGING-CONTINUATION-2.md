# RUNTIME-17 STAGING — CONTINUATION-2

> **SOURCE MIRROR NOTE:** this branch currently records the accepted staging state and provenance. Its inherited GitHub source tree is not claimed to contain every post-`$7A00` Continuation-2 edit yet. The exact active post-`$7A00` worktree must be imported from Gameplay's final staging checkpoint before this branch is used as a build authority. Do not reconstruct those edits from this document.

Status: **ADVANCED / PARTIAL CONSOLIDATION**
Classification: **NOT CANONICAL / NOT RUNTIME-18 / NOT HARDWARE PASS**

## Active implementation base

`BLOCK-GAMEPLAY-MASTER-01-RUNTIME-17-7A00-OWNER-INTEGRATED-AUDIT-PASS`

SHA256:

`ebcd26f52a5fa5c4aa5d0f0b71dea8b667c9e5cc5a9e02b8dcd0d037739ab555`

Supreme ROM authority remains `wwfwfest.zip`.
Main MC68000 SHA1:

`a726c46f350723ea73158bc78055d17d418b14e2`

RUNTIME-16 is not the active implementation base for current staging.
WIP160/Test255 remain historical only and are not authorities.

## Confirmed scheduler chain

```
$1C150
 -> $1F914
 -> $F4C2
 -> $24062
 -> $24E58
 -> $FDEE
 -> $10120
 -> $10E6A
 -> $7A00
 -> $88A2
 -> $8710
 -> $262D2
 -> $BB14/$BC00
 -> $7366
 -> $8CF8
 -> branch A or B
```

Branch A:

```
$8CF8 -> $9052 -> $899C -> $8ECC -> $2836 -> $18C4
      -> $298EC -> VBlank/ACK -> post-ACK -> $F9A -> $FA0/$21B4
```

Branch B:

```
$8CF8 -> $BCFE -> $C04A -> $C710 -> $2836
      -> $298EC -> VBlank/ACK -> post-ACK -> $F9A -> $FA0/$21B4
```

Do not assume all owners execute on every frame.

## Integrated / structurally closed staging owners

- `$88A2` — FG0/HUD producer, four wrestlers, raw FG0 output; Gameplay does not draw HUD.
- `$8710` — four compact records from `$1C14CE`, trigger bits/lifetime, `$247C/$27B8`, visual ownership external.
- `$262D2` — timer/HUD state producer, exact BCD behavior, divisor 4, 30:00 -> 29:59, raw audio/event `$000B` at minute 05; time-up still fail-closed.
- `$BB14/$BC00` — Royal replacement service, pending/counter/threshold/capacity, real `$24CC` selector, four attempts, reverse-free fallback, activation and Royal stamina path.
- `$7366` — Royal structural owner, scans ten records, actor `$1C0F1C`, table `$74DE`, RNG `$21B4`, `$2AEA/$2B58/$27B8`; ROM audit and strict gates PASS.
- `$8CF8` — gate `$1C007C`, shared lifecycle at `$1C14CE`, RNG `$21B4`, table `$8E9A`, join `$27B8`, conditional cleanup by `$1C0082`; WP25/WP26 hooks remain external.
- `$899C` — raw FG0 producer, 13 rows, ROM tables `$8A7E/$8AB2`, `CLR.L D1` each row, cumulative offsets preserved; strict/sanitizer/fanalyzer/ROM audit PASS.

## $9052 / $8ECC

`$9052` and `$8ECC` remain separate owners and must not be fused.

`$9052`:
- warning/recovery owner;
- D0/D1 partial-write behavior preserved;
- second audio command indexes `$0DEE` with the full low byte of `$1C0162`;
- demonstrated table: `3104 310A 310C 310F 3106`, repeated according to ROM indexing.

`$8ECC`:
- Royal recovery owner;
- `+$106` pauses on ineligible frames;
- threshold 128;
- `+$66 += 1`;
- `+$6A += 1`;
- no tier recomputation here;
- D0/D1 partial-write behavior preserved;
- implementation exists, but final regressions + dedicated ROM audit remain required before promotion.

## End-of-frame register contract

### $2836

Not fully transparent.

- MOVEM saves/restores D0-D5.
- D6/D7 are not protected by that MOVEM.
- deep path via `$D1FC` includes `$D216 CLR.L D7`.
- D6 may also remain caller-visible scratch.
- Do not restore D6/D7 artificially around `$2836`.

### $298EC

Empty queue:
- D0-D7 preserved.

Non-empty queue:
- D0 high16 preserved;
- D0.low16 = `4 * (last_id - 1)`;
- D1 table-derived; demonstrated queue IDs 1/2 yield `$0000018A`;
- D4 high16 preserved;
- D4.low16 = `$FFFF`;
- D2/D3/D5/D6/D7 preserved.

These caller-visible effects belong to staging semantics.

### VBlank

Normal VBlank uses MOVEM D0-D7/A0-A6 and is transparent to D0-D7.
No synthetic cleanup.

## Post-ACK / $2017A

Inter-frame boundary extends beyond VBlank/ACK.

Royal path:

```
$1156 JSR $2017A
...
$1172 JMP $F9A
```

Demonstrated execution condition for `$2017A`:

```
$1C007C != 0
AND
$1C0161.bit0 == 1
```

For the audited continuation toward `$1172 -> $F9A`:

```
$1C16C4 == 0
AND
$1C16C5 != 3
```

`$2017A` can modify D0-D6; D7 is preserved.
Do not restore a generic pre-`$2017A` snapshot.
The normal non-Royal path does not call `$2017A`; D1-D7 remain preserved there while D0.low16 may be rewritten by post-ACK.

Royal and non-Royal register shadows therefore remain distinct.

## Inter-frame RNG

Each iteration begins:

```
$F9A   CLR.W $1C006E
$FA0   JSR $21B4
```

The `$21B4` call is mandatory before branch selection.

`$21B4`:
- overwrites incoming D0 before contribution;
- consumes full LONG D1-D7;
- consumes `$1C005C`;
- consumes `$1C0080`;
- produces D0 LONG;
- writes `$1C005C`;
- restores D1 exactly;
- does not modify D2-D7.

D1-D7 are persistent inter-frame RNG-relevant state.

Forbidden:
- zeroing D1-D7;
- synthetic resets;
- C-ABI restoration;
- a single common Royal/non-Royal snapshot.

## Recent staging gates

Already passed in recent staging iterations, but must be rerun on the final consolidated tree:

- canonical ROM SHA1;
- checkpoint SHA256;
- GCC strict;
- Clang strict;
- ASan/UBSan;
- GCC `-fanalyzer`;
- WP35 regressions;
- WP36 regressions;
- SH-2 strict;
- full Saturn ELF link;
- unresolved symbols = 0;
- partial post-`$7A00` tests;
- partial `$2017A` regressions;
- no WIP160/Test255 authority leakage in active code.

## Important accumulated ROM corrections

- Referee `$20022`: `$20032 = CLR.W +$24(A0)`, clearing +$24 and +$25.
- Collision pending damage: decimal 14 must remain `0x0E`, not `0x14`.
- `$24DDC`: +1 pending damage when target `+$33.bit4 == 1`.
- `$280DC`: `MOVE.W +$32; ANDI.L #$00000004`; do not read only a byte.
- Signed arithmetic: do not reintroduce left-shift UB for negative values.
- `$9052`: full `$0DEE` table, not a two-ID simplification.
- `$2836`: D6/D7 are not restored by the MOVEM.
- `$298EC`: non-empty queue leaves caller-visible D0/D1/D4 effects.
- `$FA0`: mandatory `$21B4` call every iteration.

## Promotion blockers

Do **not** declare:
- RUNTIME-17 CANONICAL;
- RUNTIME-18;
- NORMAL FRAME STRUCTURALLY CLOSED;
- SATURN PIXEL PERFECT;
- HARDWARE PASS.

Required before promotion:

1. close `$8ECC` regressions + dedicated ROM audit;
2. revalidate the `$8CF8` join;
3. physically consolidate all post-`$7A00` owners in one tree;
4. run `wf_runtime_frame()` end-to-end;
5. consume the final D0..D7 A/B composition handoff when available;
6. rerun all gates from a clean extraction;
7. independent review;
8. produce an audited checkpoint ZIP;
9. only then consider RUNTIME-18.

## Current verdict

- POST-`$7A00` owner implementation: **ADVANCED / PARTIAL CONSOLIDATION**
- `$7366`: **ROM-AUDIT PASS**
- `$899C`: **ROM-AUDIT + STRICT/SANITIZER/FANALYZER PASS**
- `$8ECC`: **IMPLEMENTED / FINAL REGRESSION-AUDIT PENDING**
- final frame control flow: **STRUCTURALLY KNOWN**
- post-ACK / `$2017A`: **STRUCTURALLY CLOSED**
- inter-frame RNG model: **IMPLEMENTED DIRECTIONALLY / FINAL A-B MATRIX PENDING**
- RUNTIME-18: **NOT YET JUSTIFIED**

This document records current staging state only. It does not promote the branch to canonical.
