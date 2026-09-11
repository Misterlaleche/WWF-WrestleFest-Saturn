# HANDOFF

Read before changing master integration:
- `README.md`
- `PROJECT-STATE.md`
- `docs/SATURN-PIPELINE.md`
- `docs/MODE-ROSTER-HARDWARE-VALIDATION.md`
- `docs/AUDIO-RUNTIME-HANDOFF-R01.md`
- `docs/SATURN-MEMORY-CDFS-INTEGRATION-R01.md`
- `docs/cdfs/CDFS-RUNTIME-LOADER-R02.md`
- `docs/MEMORY-MANIFEST-R03.md`
- `docs/WP30-OWNERSHIP-RESOLUTION-R04.md`
- `docs/SCENE-FLOW-MASTER-R05.md`
- `docs/RECORDS-R06.md`
- `docs/CREDITS-R07.md`
- `docs/ATTRACT-BIO-R08.md`
- `docs/ATTRACT-DEMO-R09.md`
- `docs/CONTINUE-GAMEOVER-R10.md`
- `docs/OPERATOR-CREDITS-R11.md`
- `docs/CONTINUE-PAYMENT-R12.md`
- `docs/CAMPAIGN-INTERSTITIALS-R13.md`

## Absolute rule

`wwfwfest.zip` ROM > audited FINAL ROM-EXACT packages > Saturn/Yaul documentation > current source.

Never invent missing behavior.

## Parallel ownership

### Gameplay — `gameplay-runtime-r01`
Owns:
- WP01 State5/actions;
- WP28 gameplay phase bindings;
- CPU/AI including WP30 O30-04;
- physics/collision/damage;
- pin/referee/ring-out/TAG/Royal;
- gameplay camera/animation;
- WP31 objects/effects/props;
- WP17 ordinary attract gameplay CPU path, `$1F7F0` override insertion and shared `$21B4` RNG/order.

Current observed checkpoint:
`BLOCK-GAMEPLAY-MASTER-01-RUNTIME-13` @ `91f4c697bec0b04678cc1e811b4ee60e606d7131`.

Do not promote it to complete gameplay: WP28 still blocks at `0x02088E`.

### MODE + ROSTER
Canonical external checkpoint:
**WF-SATURN-MODE-ROSTER-INTEGRATION-TEST03 — HARDWARE-PASS**.

Preserve:
- display blank/sync before MODE -> ROSTER CRAM/VRAM changes;
- transparent NBG0 + fixed NBG2;
- resident ROSTER foreground;
- localized 96x96 confirmation patch at VBLANK-IN;
- cursor drawn after green mask;
- exact TAG/ROYAL navigation;
- no full NBG0 confirmation rebuild;
- TEST03 harness must not replace master `main.c`.

External SHA256:
`65daa21e89f505d6ff23efc80389145e5e32b4ada6bc8fb98df9f43417d1861a`.

### Audio
Canonical external handoff SHA256:
`cc1c9ed44edcde0237770521123072c3bbec6927bf37184c157e7b092858db65`.

WP24 remains sole backend owner.
Only 14 `INTEGRATION_READY` rows may be wired directly.
No semantic observer or audio scheduler phase.

### Integration master — `integration-master-r01`
Owns:
- WP32/WP33 memory/CDFS;
- platform no-clobber rules;
- final-memory manifest/validator;
- Scene Flow Master;
- Records / Credits / Attract root;
- Continue/Game Over;
- WP19 operator/credit/payment;
- WP13 campaign interstitial root contract;
- source-level assembly of frontend + gameplay + audio + renderer.

Current master checkpoint family: R02..R13 candidates.

## Platform safeguards

- LWRAM is never an SCU DMA endpoint.
- CDFS never writes directly into VDP1/VDP2/CRAM/SCSP.
- VDP1/VDP2/CRAM remain renderer-owned.
- SCSP RAM remains WP24-owned.
- scene publication only after complete load transaction.
- no invented loading screen.
- no BIN/CUE concatenation.
- real Saturn + MODE is the hardware gate.
