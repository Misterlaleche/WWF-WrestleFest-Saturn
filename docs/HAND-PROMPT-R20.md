# Instructional Hand Prompt R20 — WP16 R06 corrective acceptance

External checkpoint: `WP16-HAND-PROMPT-R06.zip`
SHA256: `c0d5fe5a135cb1c89e1a79023351a6e90f774bd0e2f0bf687e413eb736b1b3c8`

External Git bundle:
- branch `wp16-hand-prompt-r06`
- HEAD `4bee20470f2a5f7ab4a5c72412503705636082da`
- bundle verify: PASS
- embedded Git fsck: PASS
- working tree: clean
- branch is not currently published on the remote; package/bundle is the reproducible external record.

## Why R20 supersedes R19

R19/R02 correctly closed the runtime request/lifecycle/cadence contract, but inherited the old WP16 FINAL visual payloads whose bbox ended at relative y=87.

R06 proves directly from the ROM renderer that the 13th TEXT descriptor row is also written. Row 11 applies a -232 byte correction, so row 12 lands on the same visible TEXT row 27 at a later X position. The final ROM-visible pixels therefore extend into canvas-relative y=88..94.

This is a direct ROM contradiction of the old visual hashes, so the visual freeze is legitimately replaced.

## Independent master re-audit

Integration Master independently reconstructed main 68000 from `wwfwfest.zip` and reran the R06 ROM audit:
- main SHA1 `a726c46f350723ea73158bc78055d17d418b14e2`: PASS
- 16 direct `$10C60` init callsites: PASS
- 10 direct `$10D04` step callsites: PASS
- renderer anchors `$899C..$8A7C`: PASS
- 13 descriptor rows + corrections: PASS
- exact 78-write count: PASS

Asset rebuild directly from `31e12-0.ic33`: PASS.

Corrected assets:
- pose0 SHA256 `352072fb1ab6b2be33a42b469572189b9202ed36578b14289002261d8316bf57`
- pose1 SHA256 `36ca81e2ac88ddb113000775003e9f0db0e8f087f9b0bf75660ec73d01527a02`
- canvas 80x96
- origin (0,120)
- bbox [10,4,71,94]
- final touched TEXT cell row reaches raster y=208..215; non-zero pixels reach y=214.

Package SHA256SUMS: 40/40 PASS.
Host presentation test: PASS.
Gameplay producer fixture: PASS.
Master-adapted host regression: PASS.

## Runtime contract

Runtime semantics from R19 remain valid:
- Gameplay/WP27 owns `$10D04`, fresh logical input and mash counter;
- WP16 consumes global request flags;
- previous suppress bit preserves both shared flag bytes on early return;
- DRAW/CLEAR cadence toggles every second executed pass;
- one CLEAR pass follows the last requested pass;
- completed DRAW/CLEAR copies current flags to previous and clears current flags;
- no prompt-spawn-local phase reset is proven;
- no WP16-specific audio event is proven;
- button image is not proven to mean specifically A, B or A+B.

## Renderer contract now closed numerically

- source class: TEXT
- source map: `$0C0000..$0C1FFF`
- first address: `$0C1008`
- fixed/no-scroll
- 13 rows / 78 writes
- palette 14
- Saturn equivalent: VDP2 NBG2
- priority 4
- transform: `cell=(addr-$0C0000)/4; x=(cell%64)*8; y=(cell/64)*8-8`

Master source now emits raster x/y for each exact TEXT write and preserves shared current/previous flag semantics.

## Status

WP16 presentation: **ROM-EXACT CLOSED**
R19/R02: **SUPERSEDED visually by R20/R06**
Gameplay producer insertion: **GAMEPLAY-owned / pending**
Saturn renderer binding: **contract closed; production hookup pending**
Hardware: **NOT TESTED**
BIN/CUE: **NO**
