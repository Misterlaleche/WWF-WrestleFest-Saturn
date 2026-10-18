# WP35 + WP36 consumption status — RUNTIME-17 active implementation

## Classification

**WP35 active spatial contract: CONSUMED / SOFTWARE AUDIT PASS / HARDWARE VALIDATION REQUIRED**

**WP36 active unpaired wrestler composition path: CONSUMED / STRUCTURAL EXACT AGAINST EMBEDDED ROM-DERIVED AUTHORITY**

**WP36 partner-expanded active selection: OPEN / FAIL-CLOSED**

**ALL CHARACTER GRAPHICS CLOSED: NOT DECLARED**

**SATURN_PIXEL_PERFECT: NOT DECLARED**

**HARDWARE PASS: NOT DECLARED**

## Accepted authorities

- `wwfwfest.zip` remains supreme authority; main 68000 SHA1: `a726c46f350723ea73158bc78055d17d418b14e2`.
- WP35 R05 authority pinned by the supplied handoff: SHA256 `6cd5009bcb85c0479388f8518ff2b65d96e031b2bca7025610d5149057fb6be2`.
- WP36 authority pinned by the supplied handoff: SHA256 `cd9731cefe74aa1c6ca13c7d7cc181d5cf9864677116746321cf29d944899a80`.
- Exact handoff copied into this checkpoint: SHA256 `d8388438bf2e1a6302f2dd27fd5ba90a533ff90841b0fa48434a9911dec9b096`.

The full WP35/WP36 ZIP payloads are not embedded in this gameplay checkpoint. The accepted handoff is consumed as ownership/contract authority. Exact bulk piece comparison uses the already embedded ROM-derived WP25 expanded-piece authority and therefore does not falsely claim a fresh byte-for-byte rebuild of the absent WP36 inventory ZIP.

## Real implementation change

The previous active render path incorrectly allowed the actor `+$14/+16` values to flow as if they were final draw coordinates.

This checkpoint changes the active source path to preserve the closed separation:

`world X/Y/Z -> $247C intermediate +$14/+16 -> local metasprite composition -> WP26 buffered-sprite decode -> final draw X/Y`

`wf_visual_build_screen_positions()` now calls the existing exact R17 `$247C` contract and writes/reads the actor intermediate fields. `wf_graphics_emit_actor()` no longer adds local pieces directly to those values as final raster coordinates; it applies the WP26 9-bit decode after source-space composition.

Regression sentinel:

`raw_y=$0030 -> raster_y=192`

via:

`raster_y = ((256 - raw_y) & 0x01FF) - 16`

## WP36 composition consumption

The current resident unpaired wrestler pack was exhaustively compared against the embedded ROM-derived expanded-piece authority for all 12 wrestler sets and all 565 runtime pose slots:

- 3,632 non-empty unpaired compositions: exact piece tuple equality;
- 3,148 remaining in-range slots: exact empty equality;
- total compared runtime slots: 6,780;
- runtime piece pack SHA256: `38df145dc0f486726d1c368f3849a70c84bbe14742fbdf991a88b0dfdb0811d9`;
- runtime pose index SHA256: `cb5fc4285b0fd97caebd383e80e2ab2c1c1b71a671fa046c9308351c2a3180c3`.

Pinned regressions pass:

- Ultimate Warrior `0x0062`: 23 pieces; bbox `-34,-98,64,112`; palette 01;
- Ultimate Warrior `0x0063`: 23 pieces; same bbox/palette;
- Demolition Smash gfx 0A pose `0x005B`: 20 pieces;
- Hawk gfx 04 pose `0x0068`: 23 source cells;
- Big Bossman gfx 03 pose `0x0178`, partner 01: 86-cell authority fixture survives the runtime composition emitter and WP26 VDP1 planner with all 86 commands.

No transparent source cell is removed by the source-composition emitter.

## Deliberate fail-closed boundary

The active actor emitter still has no ROM-proven partner-set selector. Therefore a partner-expanded frame such as Big Bossman `03/0178/01` is **not guessed** from a linked actor or current target. The unpaired slot remains empty and the active actor path returns `WF_BLOCKED` until Gameplay/WP02/WP36 selection ownership is concretely joined.

The 86-cell test proves command-path capacity, not active partner selection or hardware raster output.

Exact per-character reachable-frame enumeration also remains OPEN. The generated animation table pose set is used only as an over-approximation and is explicitly labelled as such in `runtime-animation-pose-superset-matrix.csv`.

## Normal frame boundary

This work does not bypass the next Gameplay owner. `$00F4C2` remains the legitimate normal-frame blocker after the already integrated referee owner.

Existing R17 code contains the structural `$F4C2` loop, but a real binding still requires exact `$280DC` contact/geometry classification, broader `$1C03E` callback coverage, and `$2208` post-queue behavior. None is replaced by a no-op.

## Reproducible evidence

See `audit/wp35-wp36/`:

- `ROM-RUNTIME-WP26-MATRIX.csv`
- `WP35-WP36-DISCREPANCIES.csv`
- `unpaired-rom-vs-runtime.csv`
- `runtime-animation-pose-superset-matrix.csv`
- `wp35-wp36-runtime-crosscheck-summary.json`
- strict host / Clang / sanitizer / analyzer logs
- SH-2 full-link log and ELF hash
- ROM audits
- triple-review logs

