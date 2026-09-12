# WP02 ANIMATIONS ROM-EXACT R04 — Integration Master acceptance

External checkpoint:
`WP02-ANIMATIONS-ROM-EXACT-R04-AUDITED.zip`

ZIP SHA256:
`0a89f62e3670c7acc7d44d07e779b91d64fbf5407ff90648b52e0082e06afbcc`

External reproducible bundle:
- branch: `wp02-animations-rom-exact-r04-audited`
- HEAD: `5a58a537eba5cddc6fc36589f6dfd106a8e6b3d2`
- bundle verify: PASS / complete history
- clone/fsck: PASS
- clean working tree: PASS

## Why R04 supersedes R03

R03 was technically strong in ROM/data but failed as a reproducible final package. R04 corrects all four master findings:

1. restores the inherited FINAL fixtures required by package tests;
2. corrects stale raw `$4D` classification;
3. makes the roster mapping SH-2 `-Werror` safe;
4. adds a self-contained branch/HEAD Git bundle.

R04 also finds and fixes an additional real R03 regression:
- R03 had dropped Warrior pose 98/99 resident mappings and reverted to 223-frame packs;
- R04 restores `ROSTER_FRAMES=225`;
- Warrior `$0062/$0063 -> frames 223/224`;
- both contain 23 exact ROM pieces.

## Independent master re-audit

Clean delivered ZIP:
- SHA256SUMS exact coverage: **104/104 PASS**;
- no missing/extra manifest paths;
- clean `pytest`: **38/38 PASS**;
- HOST strict C11: **PASS**;
- SH-2 GCC 13.2.0 strict: **PASS** for `arcade_animation.c` + all 3 owned C tests.

Bundle-cloned functional checkpoint:
- `pytest`: **38/38 PASS**;
- HOST strict: **PASS**;
- SH-2 strict: **PASS**.

Canonical `wwfwfest.zip`:
- main 68000 SHA1 `a726c46f350723ea73158bc78055d17d418b14e2`;
- main SHA256 `265ac89deeb7e708ac6fda724356bfd6af1c9012928a4e551e6e2faa424c2306`;
- direct ROM animation audit: **PASS**;
- interpreter `N+1 / FF00 / kind2` audit: **PASS**;
- fresh main/action/state4 table scans: byte-identical to package;
- recovered GFX2 SHA256 `a3b5e1fedd6c41cd398ba0cb437cddb2f50b3ae0a0dc178a10b70a7ed5d010ba`.

Fresh full metasprite rebuild:
- 399/399 reachable poses;
- 12 internal banks;
- 10,204 valid compositions;
- 425,714 piece rows;
- 51,248 unique tile codes;
- packaged metasprite manifest: byte-identical PASS;
- packaged piece manifest: byte-identical PASS;
- packaged summary: byte-identical PASS.

Resident presentation verifier:
- Warrior `$0062/$0063`: PASS;
- Hawk/Animal resident pose set: PASS;
- all 12 packs report 225 frames.

## WP02-owned closed contract

Interface:

`animation request -> ROM descriptor -> exact frame/counter/HOLD/loop -> exact metasprite composition/resident presentation`

Current audited universe:
- 216 descriptor/subdescriptor records;
- 629 frame records;
- 399 unique non-sentinel base poses;
- 78 `$FF00` hold entries across 73 descriptors;
- 18 kind-2 dispatch loop entries;
- 399/399 metasprite pose coverage.

All 216 rows in the PASS/FAIL/OPEN matrix report:
- sequence: EXACT_MATCH;
- timing: EXACT_MATCH;
- metasprite_data: EXACT_MATCH;
- holds/loops exact where applicable.

`arcade_animation_finite_ticks()` correctly returns zero for cyclic kind-2 descriptors.

## Raw $4D / TAG visual consistency

Raw `$4D` is now correctly classified:
- descriptor `$1892C`;
- poses `0062 0063 0000 0066 0067`;
- effective cadence `13 13 HOLD 13 13`;
- classification `EXACT_MATCH`.

This agrees with WP06 TAG Visual R04.

No synthetic Warrior arm/tile patch is authorized.

## Earthquake raw $47

Animation-side exact descriptor:
- `$14440`, callback `$14458`;
- `$0084` x9;
- `$0085` x13;
- `$01C1` HOLD;
- `$01C2` x17.

There is **no descriptor loop**.

Observed repeated jumps/re-entry must come from external callback/request behavior. Camera/ring shake remains Gameplay/WP21/RING ownership.

## External blockers — not WP02 defects

The complete port is not yet visually ROM-perfect because external owners still contain/openly own:
- Gameplay callback/re-entry for FF00 release and descriptor swaps;
- TAG/Rumble request/state choreography;
- provisional ring-entry/outside-fall producer timers in historical/current Gameplay source where still present;
- provisional apron rope-shake request;
- provisional postmatch TAG gesture bridge;
- WP26 stability fallback if compiled;
- Demolition entrance resource-family bridge;
- WP25 final RGB palette identity;
- camera/ring shake.

These do not reopen WP02-owned descriptor/frame/metasprite data.

## Provenance note

The delivered ZIP and bundle functional tree are source/test/asset-identical. One tracked audit log differs only in the nondeterministic pytest elapsed-time string (`2.70s` in final ZIP vs `2.22s` in bundled commit). This is not a functional/source difference and does not affect reproducibility of the checkpoint.

## Verdict

**WP02 ANIMATIONS: CLOSED within WP02-owned scope.**

This does not mean complete end-to-end Gameplay/Renderer/Hardware animation behavior is closed.

Hardware: **NOT TESTED**.
BIN/CUE: **NO**.
R03: **REJECTED AS FINAL PACKAGE / SUPERSEDED BY R04**.
