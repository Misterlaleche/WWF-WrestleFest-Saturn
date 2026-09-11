# HANDOFF

Read `README.md`, `PROJECT-STATE.md`, `docs/SATURN-PIPELINE.md`, `docs/MODE-ROSTER-HARDWARE-VALIDATION.md`, `docs/AUDIO-HARDWARE-VALIDATION.md`, `docs/AUDIO-PRODUCER-CLEANR01-OWNER-MAP.md`, `docs/AUDIO-RUNTIME-HANDOFF-R01.md`, `docs/SATURN-MEMORY-CDFS-INTEGRATION-R01.md`, and `docs/cdfs/CDFS-RUNTIME-LOADER-R02.md` before doing any work.

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

### MODE + ROSTER
Canonical external checkpoint: **WF-SATURN-MODE-ROSTER-INTEGRATION-TEST03 — HARDWARE-PASS**.

Import rules:
- **do not replace master `main.c` with TEST03 `REFERENCE-SOURCE/main.c`;**
- blank/sync MODE->ROSTER before ROSTER CRAM/VRAM reconfiguration;
- transparent NBG0 foreground + fixed NBG2 background;
- keep ROSTER foreground resident;
- confirmation = green mask on selected 96x96 region, then cursor;
- begin localized patch upload at VBLANK-IN;
- no full NBG0 clear/rebuild, CRAM rewrite, background rewrite or scroll reset on confirmation;
- preserve TAG/ROYAL navigation and proven scroll cadence;
- post-confirm state transition remains owned by the next proven frontend/prematch block.

External handoff ZIP SHA256:
`65daa21e89f505d6ff23efc80389145e5e32b4ada6bc8fb98df9f43417d1861a`.

### Audio
Canonical external handoff: `WWF-RUNTIME-AUDIO-HANDOFF-R01-COMPLETE(1).zip`.

SHA256:
`cc1c9ed44edcde0237770521123072c3bbec6927bf37184c157e7b092858db65`.

Rules:
- WP24 remains sole Saturn audio backend owner;
- exactly 14 producer rows are currently `INTEGRATION_READY`;
- hooks are side effects at proven CLEAN-R01 states/callbacks/branches, never a new WP28 audio phase;
- no per-frame semantic observer;
- low-stamina = `0x009088 -> 0x0B`, JSR `0x00908C`;
- `0x0263CC -> 0x0B` remains `REGISTERED_NOT_IDENTIFIED`;
- 4 `OWNER_SCOPE_ONLY` and 155 `UNRESOLVED` rows remain blocked;
- exact `0x1F` lifecycle and final music cue lifetimes remain OPEN;
- validation captures are not production loops;
- if an exact runtime owner does not yet exist, use `BLOCKED_BY_RUNTIME_OWNER` rather than fabricate a hook.

See `docs/AUDIO-RUNTIME-HANDOFF-R01.md`.

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

Do not hand-copy in-progress gameplay. Merge/consume only named audited checkpoints.

## Platform safeguards

- LWRAM is never an SCU DMA endpoint.
- CDFS does not write directly into VDP1/VDP2/CRAM/SCSP.
- VDP1/VDP2/CRAM are renderer-owned.
- SCSP RAM is Audio24-owned.
- a scene is visible only after its load transaction is complete.
- loading screens are forbidden by convenience; only use one after measured hardware necessity and label it `SATURN PLATFORM ADAPTATION`.
- no BIN/CUE concatenation.
- real Saturn + MODE remains the hardware gate.
