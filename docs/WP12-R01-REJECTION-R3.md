# WP12 R01 — Master Rejection Decision from Audit R3

Date: 2026-09-12

Authority for this decision:
`WP12-R01-AUDIT-REPORT-R3(1).txt`

Audit report SHA256:
`fb051c4e527db74514cacf4944deef5bbd129ae6429890697f75a63a1ab31091`

The R3 audit restarts from `wwfwfest.zip` as absolute authority and supersedes WP12 R1/R2 audit conclusions for the decision whether R01 is integrable.

## Master decision

**WP12 ENTRANCES/PREMATCH R01 is REJECTED and MUST NOT be integrated into Integration Master.**

Do not treat R01 as:
- ROM-EXACT;
- integration candidate;
- source of final timing/state ownership semantics;
- source of final gameplay handoff semantics.

Its build stability does not rescue it: HOST and SH-2 compilation PASS, but ROM fidelity FAILS.

Recorded R01 branch/HEAD:
- branch: `entrances-prematch-r01`
- HEAD: `820ea42af3fce45591eace60cba22a94d9ad3fba`

The R3 audit also proves the delivered R01 ZIP is not reproducible from that recorded GitHub HEAD.

## Correct current status

- TAG: **BLOCKED**
- ROYAL: **BLOCKED**
- PRESENTADORA: **PARTIAL**
- ÁRBITRO PREMATCH: **MISSING**
- HANDOFF A GAMEPLAY: **BLOCKED**
- HOST COMPILE: **PASS**
- SH-2 COMPILE: **PASS**
- HARDWARE: **NOT TESTED**
- FULL SATURN ELF: **NOT APPLICABLE** for isolated R01
- BIN/CUE: **NO**

## Why R01 is rejected

The R3 audit identifies 22 blocker groups. The integration-critical failures include:

1. split +$56/+ $57 identity model is physically wrong; relevant identity is one physical +$57 byte;
2. $3032/$3031 activation predicate reversed/wrong threshold;
3. A814 writes assigned to wrong object;
4. +$57==$0A next-record activation omitted;
5. $1C0168 voice-$73 conditional controller jump omitted;
6. wrestler local visual programs $AD6C/$AD9C not executed;
7. presenter state $15 selector timing clamps instead of cycling;
8. presenter +$04 word post-processing missing;
9. WP25 numeric presenter selector->composition join not consumed;
10. ROM-proven prematch referee omitted;
11. Royal setup state written at $1086C substantially incomplete;
12. Royal actor traversal stops early when ROM continues remaining records;
13. R01 data model has 6 actors while A654 traverses 9 normal wrestler records + separate referee;
14. early-skip input evaluated before work instead of after update/render/video sync;
15. TAG-only $98BA producer omitted;
16. paid-route pre-A654 chain, including conditional credited AE20 D0=0 interstitial, omitted;
17. teardown side effects incorrectly spread over invented future frames;
18. manifest incorrectly labels $8AE6 as common teardown;
19. post-A654 preparation before proven $F8C gameplay boundary omitted;
20. renderer handoff too thin for full WP12 scene;
21. GitHub HEAD and delivered ZIP are not one reproducible source state;
22. dependency report missing; preview is only coordinate diagnostic, not scene validation.

Independent adversarial harness result against unmodified R01:
**14 failures / 15 checks**.

## Valid fragments that may inform R02, but never by blind promotion

R3 says these pieces survived within narrow scope:
- TAG/Royal separation;
- TAG primary anchor table values;
- Royal six canonical anchor values;
- Royal orientation table values as source values;
- fixed state-duration entries when those states are actually reached;
- raw voice table bytes;
- actor-window table raw values, indexed by physical +$57;
- presenter initial X/Y/Z and initial selector 2;
- presenter state15 movement target/path direction structurally supported;
- legacy fixed-corridor / rope-entry / fixed-900-tick inventions remain absent;
- no master main.c replacement, gameplay scheduler, CPU/collision or audio backend contamination;
- externalizing unresolved camera/raster producers remains correct.

These fragments are evidence only; R02 must reconstruct affected semantics directly from ROM.

## R02 mandatory gate

The next WP12 implementation is **R02**.

R02 must not be called CANDIDATE until all gates from the R3 audit are satisfied, including:

- unify identity to physical +$57;
- exact <5->$3032 / >=5->$3031;
- correct A814 object ownership;
- +$57==$0A next-record activation;
- $1C0168 skip/reachability;
- AD6C/AD9C local visual programs with threshold+1 dwell and terminal CLR.W +$1C;
- cyclic presenter state15 selector;
- A654 +$04 word post-processing;
- WP25 set 0x1F numeric presenter composition;
- prematch referee setup/presence/render contract;
- all 9 wrestler records + separate referee;
- full Royal setup or proven upstream consumption;
- continue Royal traversal after terminal callback;
- Royal +$04 rewrite from +$33.bit7;
- ROM-equivalent late skip ordering;
- TAG-only $98BA effect contract;
- paid-route pre-A654 chain, including conditional AE20 D0=0;
- ordered side-effect delivery without invented VBlank boundaries;
- exact A778 teardown and corrected $8AE6 classification;
- post-A654 table audio + $8AE6 preparation before $F8C;
- expanded renderer handoff without invented camera math/priorities;
- independent adversarial regression tests;
- dependency report;
- meaningful preview only from proven graphics/renderer data;
- exact delivered tree committed to a new R02 branch/checkpoint;
- host and SH-2 warning-clean rebuild;
- hardware remains NOT TESTED until real Saturn validation.

## Ownership safeguards

R02 must NOT touch:
- GAMEPLAY/WP28;
- CPU/AI;
- collision/physics/damage;
- TAG/Royal gameplay;
- gameplay referee logic;
- WP24 backend audio.

Raw audio producers may be preserved only as authority-backed tokens/events.

## Integration invariant

Until a new audited R02 checkpoint is delivered and accepted:

**WP12 remains BLOCKED in Integration Master.**

The proven gameplay boundary address `$00000F8C` does not mean R01 handoff is closed. Exact preparation leading to it must first be reproduced.

No BIN/CUE should be generated for WP12 at this stage.
