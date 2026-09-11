# HANDOFF

Read `README.md`, `PROJECT-STATE.md`, `docs/SATURN-PIPELINE.md`, `docs/AUDIO-HARDWARE-VALIDATION.md`, `docs/AUDIO-PRODUCER-CLEANR01-OWNER-MAP.md`, `docs/SATURN-MEMORY-CDFS-INTEGRATION-R01.md`, and `docs/cdfs/CDFS-RUNTIME-LOADER-R02.md` before doing any work.

## Absolute project rule

`wwfwfest.zip` ROM > audited ROM-EXACT FINAL packages > Saturn/Yaul documentation > current Saturn code.

Never invent missing behavior.

## Parallel ownership

### Gameplay
Branch: `gameplay-runtime-r01`.

Owner scope:
- WP01 State5/action callbacks;
- WP28 gameplay bindings;
- CPU/AI;
- physics/collision/damage;
- pin/referee/ring-out/TAG/Royal;
- gameplay camera/animation integration.

Continue from the audited RUNTIME-11 lineage. Do not use WIP160/175/176 as the gameplay base.

### Integration master
Branch: `integration-master-r01`.

Owner scope:
- WP32 memory ownership/budgets;
- WP33 CDFS/disc/loading contract;
- shared platform no-clobber rules;
- CDFS runtime loader and scene publication gate;
- scene asset lifetime matrix;
- later source-level assembly of frontend + gameplay + audio + renderer.

Current R02 candidate:
- host tests PASS;
- SH-2 core compile PASS;
- SH-2 Yaul adapter compile PASS;
- VDP2/CRAM capacity mapped;
- HWRAM/LWRAM load classes enforced;
- overlapping load destinations rejected;
- scenes remain unpublished after partial/failed I/O;
- VDP1 final partition BLOCKED pending runtime maxima;
- no BIN/CUE;
- no new hardware PASS claimed.

Do not hand-copy an in-progress gameplay checkpoint into this branch. Merge/consume only a named audited gameplay commit.

## Audio safeguards

- WP24 remains sole Saturn audio backend owner.
- low-stamina producer is `0x009088`, JSR `0x00908C`, command `0x0B`.
- `0x0263CC -> 0x0B` is `REGISTERED_NOT_IDENTIFIED`.
- only `INTEGRATION_READY` producer rows may be wired directly.
- no TEST-03 fixed-time refill;
- no slot-0 semantic observer;
- no finite validation capture relabelled as a production loop.

## Platform safeguards

- LWRAM is never an SCU DMA endpoint.
- CDFS does not write directly into VDP1/VDP2/CRAM/SCSP.
- VDP1/VDP2/CRAM are renderer-owned.
- SCSP RAM is Audio24-owned.
- a scene is visible only after its load transaction is complete.
- loading screens are forbidden by convenience; only use one after measured hardware necessity and label it `SATURN PLATFORM ADAPTATION`.
- no BIN/CUE concatenation.
- real Saturn + MODE remains the hardware gate.
