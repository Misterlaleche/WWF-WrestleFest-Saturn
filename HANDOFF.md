# HANDOFF

Read:
- `README.md`
- `PROJECT-STATE.md`
- `docs/SATURN-PIPELINE.md`
- `docs/MODE-ROSTER-HARDWARE-VALIDATION.md`
- `docs/AUDIO-HARDWARE-VALIDATION.md`
- `docs/AUDIO-PRODUCER-CLEANR01-OWNER-MAP.md`
- `docs/AUDIO-RUNTIME-HANDOFF-R01.md`
- `docs/SATURN-MEMORY-CDFS-INTEGRATION-R01.md`
- `docs/cdfs/CDFS-RUNTIME-LOADER-R02.md`
- `docs/MEMORY-MANIFEST-R03.md`
- `docs/WP30-OWNERSHIP-RESOLUTION-R04.md`

## Absolute project rule

`wwfwfest.zip` ROM > audited ROM-EXACT FINAL packages > Saturn/Yaul documentation > current Saturn code.

Never invent missing behavior.

## Parallel ownership

### Gameplay
Branch: `gameplay-runtime-r01`.

Owner scope:
- WP01 State5/action callbacks;
- WP28 gameplay bindings;
- CPU/AI including WP30 O30-04 CPU-controller remainder;
- physics/collision/damage;
- pin/referee/ring-out/TAG/Royal;
- gameplay camera/animation integration;
- WP31 objects/effects/props engine (WP30 O30-05).

Continue from audited RUNTIME-11 lineage. Do not use WIP160/175/176 as gameplay base.

### MODE + ROSTER
Canonical external checkpoint: **WF-SATURN-MODE-ROSTER-INTEGRATION-TEST03 — HARDWARE-PASS**.

Import rules:
- do not replace master `main.c` with TEST03 harness;
- blank/sync MODE->ROSTER before CRAM/VRAM reconfiguration;
- transparent NBG0 foreground + fixed NBG2 background;
- keep ROSTER foreground resident;
- confirmation = green mask on selected 96x96 region, then cursor;
- begin localized patch at VBLANK-IN;
- no full NBG0 clear/rebuild, CRAM rewrite, background rewrite or scroll reset on confirmation;
- preserve TAG/ROYAL navigation and proven scroll cadence.

External handoff SHA256:
`65daa21e89f505d6ff23efc80389145e5e32b4ada6bc8fb98df9f43417d1861a`.

### Audio
Canonical external handoff:
`WWF-RUNTIME-AUDIO-HANDOFF-R01-COMPLETE(1).zip`
SHA256:
`cc1c9ed44edcde0237770521123072c3bbec6927bf37184c157e7b092858db65`.

WP24 is sole Saturn audio backend owner. Exactly 14 producer rows are currently `INTEGRATION_READY`. No semantic observer/new audio scheduler phase.

### Integration master
Branch: `integration-master-r01`.

Owner scope:
- WP32 memory ownership/budgets;
- WP33 CDFS/disc/loading;
- shared platform no-clobber rules;
- CDFS runtime loader and scene publication gate;
- final-memory manifest/validator;
- Records/ranking/initials integration (O30-01);
- Ending Credits integration (O30-02);
- Attract/Bio root orchestration (O30-03);
- single Scene Flow Master root (O30-06);
- source-level assembly of frontend + gameplay + audio + renderer.

R03/R04 gates:
- memory manifest structural validation PASS;
- final memory fit BLOCKED until VDP1/link-map/scene peaks/CDFS root count/hardware timing close;
- all six WP30 ownership gaps assigned;
- ownership assignment does not mean implementation complete.

Do not hand-copy in-progress gameplay. Consume named audited checkpoints only.

## Platform safeguards

- LWRAM is never an SCU DMA endpoint.
- CDFS does not write directly into VDP1/VDP2/CRAM/SCSP.
- VDP1/VDP2/CRAM are renderer-owned.
- SCSP RAM is Audio24-owned.
- scene publication occurs only after its load transaction completes.
- loading screens only after measured necessity and labelled `SATURN PLATFORM ADAPTATION`.
- no BIN/CUE concatenation.
- real Saturn + MODE remains the hardware gate.
