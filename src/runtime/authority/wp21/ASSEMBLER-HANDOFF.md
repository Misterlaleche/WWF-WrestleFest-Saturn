# ASSEMBLER HANDOFF — Workpack 21

## Authority

- Main 68000 SHA1: `a726c46f350723ea73158bc78055d17d418b14e2`.
- Direct dependencies: CPU FINAL; Animation FINAL; Referee/Announcer FINAL; Records FINAL for FG0 geometry; Presentation/Attract FINAL only for scene labels/timing they own.
- Historical WIP160 is capture/reference only, not global camera authority.

## Implementable now

1. Preserve two scroll pairs equivalent to arcade `$17E6/$17EE` and `$17EA/$17F2`.
2. Implement camera `$26936` using actor camera-class low byte `+$4B` and exact nine-slot iterator semantics.
3. Class 0: include every eligible active standard actor; signed-average X/Y deltas; clamp each axis to `[-4,+4]`.
4. Class 1/2/3/4 exactly as the report; do not attach unproven gameplay names.
5. Apply boundary table `$298B4` indexed by scene word equivalent to `$1C007E`.
6. Preserve world→screen relation:
   - `sx = +06 - scrollX + signed8(+19)`, with signed X correction negated when `+$2E.bit7=1`;
   - `sy = +0A + +0E - scrollY + signed8(+1B)`.
7. Preserve shake timers/table; a newly written count 14 consumes table indices `14..1`. Shake occurs after base gameplay clamp and before display scroll upload.
8. Preserve actor render class equivalent to `+$12`: six lists; emit `5→4→3→2→1→0`; sort only 5/4/2/0 by actor `+$0A` descending; 3/1 retain insertion order.
9. Do not flatten all ring ropes into one fixed background. Reproduce the capture-proven relation: side ropes before standing fighters but after climber; front horizontal ropes after referee.
10. Keep HUD/FG0 in screen space independent of world camera and camera shake.
11. Referee and TAG presenter move through world→screen/render-list composition when active, but are not automatically included in the normal nine-slot centroid.
12. Logical viewport: 320×240 at 1:1. Do not add invented crop/overscan compensation.

## Must remain fail-closed

- No universal human semantic mapping for render-list values `0..5` without stronger evidence.
- Do not guess the custom-video bit meaning of priority register `$00140010`; reproduce final painter behavior instead.
- Do not label every `$1804/$1806` writer as ring/outside/entrance unless its owning FINAL source proves the scene.
- Do not reintroduce WIP limits `[-130,62]/[-95,131]`, two-fighter midpoint tracking or `±3` camera speed.

## Exact tables / constants

- camera max per call: `±4` independently per axis;
- class-1 vertical constant: `$0140`, or `$0100` when `$1C0161.bit1=1`;
- normal X anchor: `scrollA_x + $00A0`;
- normal Y anchor: `scrollA_y - $00F0`;
- clamp table: `tables/camera_clamps.csv`;
- shake table: `tables/shake_table.csv`;
- render lists: `tables/render_lists.csv`;
- scroll-register mapping: `tables/scroll_registers.csv`.

## Required actor/global fields

- actor `+$06`: world X WORD;
- actor `+$0A`: world Y WORD;
- actor `+$0E`: vertical/base WORD used by class 1 and renderer;
- actor `+$19/+1B`: signed low-byte render corrections;
- actor `+$2E.bit7`: facing/orientation sign control;
- actor `+$12`: render-list selector WORD;
- actor `+$4B`: low byte of camera-class word `+$4A`;
- actor `+$33.bit0/+bit2`: camera eligibility/special branch predicates;
- scene index `$1C007E` WORD;
- scene target/seed `$1C1804/$1C1806` WORDs;
- scroll pairs `$1C17E6/$1C17EE/$1C17EA/$1C17F2` WORDs;
- shake counts `$1C1800/$1C1802` WORDs.

## Ordering/timing requirements

- On normal gameplay route: camera `$26936` → clamp `$2983C` → actor callbacks → collision/damage → sprite emission.
- VBlank/display-side shake is later than base camera clamp and before scroll-register upload.
- Do not re-clamp the shaken display offset in the same display update.
- Actor render list emission order is exact and must not be replaced by a generic all-sprite global sort.

## No-clobber requirements

- Preserve 16-bit WORD behavior for camera/scroll/targets/clamps.
- Preserve signed 8-bit interpretation of `+$19/+1B` after low-byte extraction.
- Preserve facing-controlled sign reversal only for X correction.
- Do not merge the two scroll pairs merely because normal camera updates them in parallel.
- Do not make referee/presenter fixed-screen just because they are special actors.

## Required integration tests

- verifier passes on canonical ROM;
- 1/2/4+ eligible actor tests match signed averaging and never exceed `±4` per update;
- class 2 freezes the camera call; class 3 contributes nothing; class 4 converges toward exact scene target by max 4/axis;
- every boundary row matches `camera_clamps.csv`, including fixed-Y rows 2 and 6;
- world→screen test covers facing reversal, non-zero `+$0E`, negative low-byte offsets;
- edge-of-world shake remains visible beyond the base boundary for that display update;
- render-list order/sorting matches `5→0` and selective descending Y sort;
- referee/presenter move with world camera but do not alter nine-slot centroid;
- HUD remains fixed while world scroll/shake moves;
- rope occlusion regression covers climber / standing fighter / referee / front-rope relation.

## Runtime-dependent items

- exact actors participating in a specific frame centroid;
- exact camera X/Y on a particular frame;
- dynamic reachability of some camera-class writers/scene target values.

## Not provable from current evidence

- universal semantic label for every actor render-list index;
- full bitfield semantics of arcade custom-video priority register `$00140010`;
- human scene label for every direct `$1804/$1806` writer inside this isolated workpack.
