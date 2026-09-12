# BLOCK-GAMEPLAY-MASTER-01 — RUNTIME-16 AUDIT

Status: **GAMEPLAY-R01 CANDIDATE / cumulative / fail-closed**.

Base: RUNTIME-11 -> RUNTIME-12 -> RUNTIME-13 -> RUNTIME-14 -> RUNTIME-15 -> RUNTIME-16. WIP160/WIP175/WIP176 are historical only.

Authority: `wwfwfest.zip` > audited FINAL ROM-EXACT packages > CLEAN-R01 > Saturn/Yaul documentation > current implementation.

## 1. Canonical ROM gate

Reconstructed main 68000 image SHA1:

`a726c46f350723ea73158bc78055d17d418b14e2`

`tools/audit_runtime16_rom.py` verifies the scheduler call sequence, exact `$00F560` aggregate, `$00F18A` scheduler head/state table, the recovered relation/timing tables, the `$00F98C/$00A0E0` gates and the next fixed-referee owner `$01F914`.

Result: **PASS**.

## 2. WP28 `$00F560` — pair/relation aggregate

The ROM aggregate is preserved as:

- `$00F574` pair scan/eligibility and relation dispatch;
- `$00F98C` scene/transition owner;
- `$00FD00` six-record timer/role maintenance;
- `$00A0E0` audio-producer state machine owner.

The pair scan retains the nine-wrestler live iterator, `$00F7E8` predicate, facing/team/role gates, exact X/Y windows and reciprocal actor-pointer writes. `$00FD00` retains its six-record scope and ROM tables.

The RNG-dependent relation branch is intentionally fail-closed at `$0021B4`: the original consumes live 68000 D-register context whose exact provenance is path-dependent across `$026936/$02983C/$00F574/$00F5E4`. RUNTIME-16 does not manufacture that context or use a synthetic seed.

`$00F98C` is executable only on its demonstrated idle/no-op gate in this block; an active scene-transition path remains blocked. `$00A0E0` is likewise permitted only on its demonstrated idle gate; active audio production remains an integration dependency under WP24.

## 3. WP28 `$00F18A` — actor raw-state prepass

RUNTIME-16 implements the demonstrated actor scan and exact state pre-dispatch behavior:

- role exclusions from actor `+$56`;
- exact human-controller gate to `$00DCDE`;
- `+$B4/+$B6` clears;
- 13-entry low-state dispatch table;
- state-0 facing/state behavior including the demonstrated facing helper semantics;
- state-1 direction table `$00F2D6` and local transitions;
- state-2 local direction/distance gates;
- state-5 selector-local transitions;
- state-9 `+$A9.bit3` transition;
- ROM RTS/no-op states preserved.

Unresolved downstream helpers remain explicit blockers rather than approximations:

- `$00DCDE` human action/controller;
- `$010E86` follow/movement helper;
- `$01129E` state-2 random/helper path.

## 4. Scheduler advance

With RUNTIME-16 real bindings, the integration trace advances through:

`$00514E`
-> `$008B3A`
-> `$02088E`
-> `$020958`
-> `$026936`
-> `$02983C`
-> `$00F560`
-> `$00F18A`
-> `$01C150`
-> **`$01F914` BLOCKED**

No stub is installed for `$01F914`.

## 5. Next owner: `$01F914`

ROM audit identifies `$01F914` as the fixed referee update at actor `$1C11F4`. Its frame path includes:

- referee low-state dispatch through table `$01F952`;
- `$01FAF0` referee continuation;
- `$00247C` render-coordinate preparation;
- `$0027B8` object queue insertion;
- Royal-specific `$00F8E4` when `$1C0161.bit1` is set.

Because the user contract forbids modifying the renderer master without an explicit integration contract, RUNTIME-16 stops here rather than replacing `$247C/$27B8` with a no-op or synthetic referee representation.

## 6. Build/test gates

- HOST: PASS, full suite, `-std=c11 -Wall -Wextra -Werror -pedantic`.
- SH-2 cross compile: PASS, 36 non-Saturn-specific translation units.
- Full Saturn ELF link: PASS, 41 objects.
- Unresolved symbols: 0.
- ELF SHA256: `bf33e0eac805fa345dd55f46a1cb2a976fad73bb85ca2a41a57f04998e176d02`.
- No disc BIN/CUE generated.

## 7. Explicitly preserved blockers / scope exclusions

- complete normal `wf_runtime_frame()`: NOT COMPLETE;
- direct-match bootstrap: BLOCKED;
- attract gameplay bootstrap: BLOCKED;
- shared RNG `$21B4`: BLOCKED until exact live register context is proved;
- WP12 ENTRANCES/PREMATCH: untouched;
- frontend/INTRO/MODE/ROSTER/credits: untouched;
- CDFS/memory master: untouched;
- renderer master: untouched;
- audio backend: untouched; WP24 remains sole owner.

RUNTIME-16 is a real audited advance, not FINAL gameplay closure.
