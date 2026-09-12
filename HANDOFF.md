# HANDOFF

Read before changing master integration:
- `README.md`
- `PROJECT-STATE.md`
- `docs/SATURN-PIPELINE.md`
- `docs/MODE-ROSTER-HARDWARE-VALIDATION.md`
- `docs/AUDIO-RUNTIME-HANDOFF-R02.md`
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
- `docs/FRONTEND-TRANSITION-R14.md`
- `docs/HAND-PROMPT-R15.md` (historical; superseded by WP16 R02 / master R19)
- `docs/HUD-CORE-R16.md`
- `docs/INTEGRATION-SELF-AUDIT-A01.md`
- `docs/INTRO-HARDWARE-PASS-R17.md`
- `docs/WP12-R01-REJECTION-R3.md`
- `docs/FRONTEND-TITLE-R18.md`

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
- WP17 ordinary attract gameplay CPU path, `$1F7F0` override and shared `$21B4` RNG/order.

Current observed checkpoint:
`BLOCK-GAMEPLAY-MASTER-01-RUNTIME-15` — source checkpoint `72de727a77603c0a81f552fa059c0d2072cd249b`; gameplay branch documentation HEAD `7f40fde30cb7af32cf95cd4d9469884a342da8b8`.

Do not promote it to complete gameplay: RUNTIME-15 closes `0x020958` and binds WP21 camera phases `0x026936/0x02983C`, but WP28 now blocks at `0x00F560`.

### INTRO / PRESENTATION
Canonical external checkpoint:
**WF-SATURN-INTRO-ROMEXACT-FINAL-HWTEST — HARDWARE-PASS** on real Saturn NTSC-U + MODE.

Package SHA256:
`0fb488514ae233c429058834b8ced73c757b0224d819d6ba4dc3015042f81c98`.

Critical integration invariants:
- TITLE.ANM `TITLE_COUNT=451` means 451 **stored asset frames**, not arcade-native total duration;
- do not derive native scene duration from 451;
- INTRO contains no credit/coin/START/MODE logic;
- `CREDIT:x`, `INSERT COIN` and START prompt presentation belong to FRONTEND;
- logical credit/prompt state belongs to WP19/R11;
- do not import the inspection-only final-frame infinite hold;
- do not replace master `main.c` with the hardware-test harness.

### MODE + ROSTER
Canonical external checkpoint:
**WF-SATURN-MODE-ROSTER-INTEGRATION-TEST03 — HARDWARE-PASS**.

Preserve:
- display blank/sync before MODE -> ROSTER CRAM/VRAM changes;
- transparent NBG0 + fixed NBG2;
- resident ROSTER foreground;
- localized 96x96 confirmation patch at VBLANK-IN;
- cursor after green mask;
- exact TAG/ROYAL navigation;
- no full NBG0 confirmation rebuild;
- TEST03 harness must not replace master `main.c`.

External SHA256:
`65daa21e89f505d6ff23efc80389145e5e32b4ada6bc8fb98df9f43417d1861a`.

### Audio
Canonical external handoff:
**WWF-RUNTIME-AUDIO-HANDOFF-R02-REAUDITED.zip**

SHA256:
`49335637b8ba6d26b361c88e3886f620e9f52c076df3d4bfb44accf3701ccb5c`.

R02 supersedes R01. Do not use the old R01 package as current authority.

WP24 remains sole backend owner.
Only 14 `INTEGRATION_READY` rows may be wired directly.
Four rows remain `OWNER_SCOPE_ONLY`; 155 remain `UNRESOLVED`.
No semantic observer or audio scheduler phase.
`0x1F` remains OPEN/fail-closed.
The 12 WP24 source/test blobs in R02 already byte-match Integration Master.

See `docs/AUDIO-RUNTIME-HANDOFF-R02.md`.

### Integration master — `integration-master-r01`
Owns:
- WP32/WP33 memory/CDFS;
- platform no-clobber rules;
- final-memory manifest/validator;
- Scene Flow Master;
- Records/Credits/Attract root;
- Continue/Game Over;
- WP19 operator/credit/payment;
- WP13 campaign interstitial root contract;
- WP15 transition primitives;
- WP16 R02 hand-prompt presentation contract (ROM-EXACT closed; Gameplay producer remains external);
- WP18 HUD presentation core;
- source-level assembly of frontend + gameplay + audio + renderer.

Current master checkpoint family includes R19 WP16 R02 acceptance. R17 INTRO visual block is HARDWARE-PASS; R18 binds Frontend TITLE credit/prompt presentation read-only to WP19/R11; R19 closes WP16 presentation logic.

## Platform safeguards

- LWRAM is never an SCU DMA endpoint.
- CDFS never writes directly into VDP1/VDP2/CRAM/SCSP.
- VDP1/VDP2/CRAM remain renderer-owned.
- SCSP RAM remains WP24-owned.
- scene publication only after complete load transaction.
- no invented loading screen.
- no BIN/CUE concatenation.
- real Saturn + MODE is the hardware gate.


## A01 audit corrections

Before any integrated hardware build, preserve these corrections:
- R09 post-demo first 24 VBlanks invoke ROM $008F50 stripe-mask close, not $0264E2 palette fade.
- R13 must gate progression==9 to $1B4E before interstitials; after BYTE increments config bit7 skips progression values 3/8; progression==5 requires the exact $1C0598/$1C05B0 preparation contract before $AC0.

A01 verdict: PASS WITH TWO CORRECTED FINDINGS. No hardware PASS implied.


## WP12 ENTRANCES/PREMATCH

R3 authority decision:
**R01 is REJECTED and must not be integrated.**

Do not consume `entrances-prematch-r01` as a ROM-EXACT candidate even though its host/SH-2 builds pass.

Current WP12 state:
- TAG BLOCKED;
- ROYAL BLOCKED;
- presenter PARTIAL;
- prematch referee MISSING;
- gameplay handoff BLOCKED;
- hardware NOT TESTED.

R3 supersedes earlier R1/R2 audit conclusions. Next accepted work must be **WP12 R02** and must satisfy all gates in `docs/WP12-R01-REJECTION-R3.md`.

The proven `$F8C` runtime boundary does not authorize a generic handoff; the exact post-A654/pre-$F8C preparation remains mandatory.

No WP12 BIN/CUE yet.


## WP16 hand prompt R02

Canonical external checkpoint: `WP16-HAND-PROMPT-R02.zip`
SHA256: `ffa8a1104023db142ba0f379c9f79a77c393ad5e79f8769b5f7ecde0a0877642`

Integration rule:
- Gameplay/WP27 produces the request and fresh-edge counter behavior;
- WP16 consumes the global request and outputs the exact presentation lifecycle;
- do not label the displayed button as A, B or A+B;
- no WP16 audio backend or gameplay logic;
- renderer binding/hardware validation still OPEN.
