# WP20 SELECT/JOIN-IN R03 — Integration Master acceptance

Date: 2026-09-12

Canonical external checkpoint:
`WP20-SELECT-JOININ-R03-AUDITED.zip`

ZIP SHA256:
`c83e4f8ef0697c7b2e42f143bf35c458e42413b4f53656940afe5f0534235c09`

External bundle:
- branch: `wp20-select-joinin-r03`
- HEAD: `f814acaee8f7ebb9fbac153f2299314a0f0f9702`
- bundle verify: PASS
- complete-history declaration: PASS
- note: this branch is not currently published in the GitHub remote; the package/bundle remains the reproducible external checkpoint.

## Independent Integration Master re-audit

Authority:
`wwfwfest.zip` > WP20/WP22 FINAL > Presentation FINAL > WP15/WP19/WP27 > TEST03 hardware-pass invariants.

Checks rerun:
- package SHA256SUMS: 37/37 PASS;
- HOST `-Wall -Wextra -Werror -pedantic`: PASS;
- ROM verifier against current `wwfwfest.zip`: PASS;
- SH-2 GCC 13.2.0 compile for both WP20 translation units: PASS;
- SH-2 relocatable aggregate: PASS; only compiler-runtime helper `___ashlsi3_r0` remains unresolved at `ld -r`, not a project-owner symbol;
- WP22 cross-check: selector slots 8/9 -> internal IDs `0A/0B` = Demolition Smash / Demolition Crush: PASS.

ASan/UBSan package result is accepted as supplied. A local rerun in the current environment was blocked by ASan runtime preload ordering, not by a WP20 test failure.

## R03 closes the four original WP20 OPEN items

1. **timeout 1024 -> AUTO -> terminal 64**
   - timeout pauses while page scroll bit3 is active;
   - AUTO direction from shared RNG `$21B4` is consumed in the same actor update;
   - terminal 64 creates fresh action and confirmation wins before movement.

2. **late joins P2/P3/P4**
   - generic P1-P4 join path closed;
   - dedicated `$0800` P2-only Royal/TAG routes closed;
   - TAG pending-assignment `$61A6` and linked-confirmed `$5F28` special case closed.

3. **selector IDs 8/9**
   - `$64CC` maps slots 8/9 to internal IDs `0A/0B`;
   - WP22 proves these are Demolition Smash / Demolition Crush;
   - prior Hawk/Animal claim is superseded.

4. **simultaneous occupancy arbitration**
   - transient occupancy `$1C11F4[10]` and confirmation locks `$1C1214[10]` are separate;
   - actor-order processing plus exact occupied fallback `$63F0` closes arbitration at source-contract level.

## Additional R02 defects corrected by R03

- live LEFT matrix is `$62BC`, not `$62A6`;
- page request/commit/reconcile ordering corrected;
- TAG slot8 actor-order-dependent 8/9 assignment corrected;
- normal TAG confirmation linked-selector creation implemented;
- initial selector slots/topology corrected to 0,1,5,6;
- initial 2P TAG bit6/bit7 reassertion corrected;
- post-confirm 76 moved to the separate end-of-loop scan and continues incrementing past 76 while another actor blocks completion;
- final teardown now reaches the real `$00000AC0` handoff after the WP15 `$21E6(D0=$0020)` boundary;
- HOST/SH-2 pointer-width contract fixed to explicit 32-bit `wf20_uptr`.

## Required phase order

One SELECT outer loop must preserve:

1. scroll tick OR timeout/AUTO tick;
2. active unconfirmed actors in record order;
3. late join `$60A2`;
4. pending page commit/toggle/reconcile;
5. post-confirm `+$58` scan;
6. repeat while unresolved/post-confirm/join state remains;
7. `$64CC` conversion, TAG pair ordering, teardown, `$21E6($20)`, handoff `$AC0`.

Reordering these phases is not ROM-exact.

## Ownership

WP20 does not own:
- shared RNG implementation `$21B4`;
- credit consumption `$55A` (WP19);
- Saturn pad normalization/fresh edges (WP27);
- renderer/transition or `$21E6` implementation (WP15/TEST03);
- prematch after `$AC0` (WP12);
- Gameplay/CPU/physics/collision/damage/TAG/Royal gameplay.

## Integration status

WP20 source contract: **ROM-EXACT CLOSED / NO KNOWN STRUCTURAL OPEN**.
TEST03 visual block: **HARDWARE-PASS**, unchanged.
R03 newly corrected logic: **NOT HARDWARE-PASS** until integrated and tested on real Saturn.
BIN/CUE: **NO**.

WP20 R03 supersedes R01/R02 as the active select/join-in authority.
