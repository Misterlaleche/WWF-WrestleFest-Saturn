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
- `docs/HAND-PROMPT-R15.md` (historical)
- `docs/HAND-PROMPT-R20.md` (active WP16 authority)
- `docs/HUD-CORE-R16.md` (historical base; extended by WP18 R03)
- `docs/WP18-HUD-R03-ACCEPTANCE.md`
- `docs/WP06-TAG-EXCHANGE-VISUAL-R04-ACCEPTANCE.md`
- `docs/INTEGRATION-SELF-AUDIT-A01.md`
- `docs/INTRO-HARDWARE-PASS-R17.md`
- `docs/WP12-R01-REJECTION-R3.md` (historical R01 rejection)
- `docs/WP12-R02-ACCEPTANCE.md`
- `docs/FRONTEND-TITLE-R18.md`
- `docs/GAMEPLAY-RUNTIME-16-ACCEPTANCE.md`
- `docs/WP20-R03-ACCEPTANCE.md`
- `docs/RING-ROPES-CAMERA-R02-ACCEPTANCE.md`
- `docs/WP02-ANIMATIONS-R04-ACCEPTANCE.md`
- `docs/GAMEPLAY-RUNTIME-17-OWNER-CLOSURE-AUDIT.md`

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

Current accepted checkpoint:
`BLOCK-GAMEPLAY-MASTER-01-RUNTIME-16`

Source integration commit:
`8589e9ef796c2ed6ed00fb9a9f07c8eda3c96f24`

Gameplay branch HEAD:
`1853f5564b8a00ddd5883a68169993305a40cfae`

RUNTIME-16 binds `0x00F560` and `0x00F18A`, then reaches `0x01C150` and blocks at the next mandatory owner `0x01F914` fixed referee scheduler.

Do not promote it to complete gameplay: the full normal frame is still incomplete and multiple exact downstream helpers remain fail-closed.

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
- WP16 R06 hand-prompt presentation contract (ROM-EXACT closed; Gameplay producer remains external);
- WP18 HUD presentation core + active R03 renderer-facing xBGR444 contract;
- source-level assembly of frontend + gameplay + audio + renderer.

Current master checkpoint family includes R20 WP16 R06 corrective acceptance. R17 INTRO visual block is HARDWARE-PASS; R18 binds Frontend TITLE credit/prompt presentation read-only to WP19/R11; R20 is active WP16 authority.

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

Canonical active checkpoint:
**WP12 R02 — FINAL AUDITED SOURCE CHECKPOINT**.

- branch: `entrances-prematch-r02`
- HEAD: `99d316037af7508a896f0535d44dfa18775d0c47`
- ZIP SHA256: `966ac936f2a3a71b05673675b7ef041668fd99f2cf51431ccf9290dbb146f2fa`
- exact checkpoint subtree in master: `wp12-r02/`

R01 remains rejected/superseded.

R02 state:
- TAG PARTIAL;
- ROYAL PARTIAL;
- presenter CLOSED structural/numeric;
- prematch referee CLOSED structural;
- gameplay handoff boundary CLOSED at `$F8C`, execution external;
- hardware NOT TESTED.

Do not bypass external contracts:
- canonical `$C98` setup;
- shared `$21B4` RNG;
- WP26/canonical renderer machine-coordinate transform;
- Gameplay ownership after `$F8C`.

Do not invent the still-open wrestler-name/red-band/WWF-logo semantic join or final camera/raster transform.

No WP12 BIN/CUE yet.


## WP16 hand prompt R06

Canonical external checkpoint: `WP16-HAND-PROMPT-R06.zip`
SHA256: `c0d5fe5a135cb1c89e1a79023351a6e90f774bd0e2f0bf687e413eb736b1b3c8`
Bundle HEAD: `4bee20470f2a5f7ab4a5c72412503705636082da`

R06 supersedes R19/R02 visual payloads because the ROM renderer proves the final TEXT row. Active geometry:
- canvas 80x96;
- origin (0,120);
- bbox [10,4,71,94];
- 13 rows / 78 writes;
- VDP2 NBG2 priority 4.

Integration rule:
- Gameplay/WP27 produces request and fresh-edge/mash-counter behavior;
- WP16 consumes global shared flags and produces exact DRAW/CLEAR/NONE lifecycle;
- do not label displayed button as A, B or A+B;
- no WP16 audio backend or gameplay logic;
- production renderer hookup/hardware validation still OPEN.


## WP20 CHARACTER SELECT / JOIN-IN

Canonical active checkpoint:
`WP20-SELECT-JOININ-R03-AUDITED.zip`

SHA256:
`c83e4f8ef0697c7b2e42f143bf35c458e42413b4f53656940afe5f0534235c09`

External bundle HEAD:
`f814acaee8f7ebb9fbac153f2299314a0f0f9702`

R03 supersedes R01/R02 and closes:
- timeout1024 -> AUTO -> terminal64;
- late joins;
- selector 8/9 identity;
- simultaneous occupancy arbitration;
- exact page ordering;
- linked TAG selector behavior;
- post-confirm 76 phase;
- final teardown/handoff `$AC0`.

Preserve exact SELECT outer-loop order documented in `docs/WP20-R03-ACCEPTANCE.md`.

TEST03 renderer remains hardware-pass and is not replaced.
New R03 logic requires integrated real-Saturn validation before HARDWARE-PASS.


## WP18 HUD R03

Active external checkpoint:
`WP18-HUD-INTEGRATION-R03-AUDITED(1).zip`
SHA256:
`8c8b46fa3b1f60f8e76fab2f76e6692177f9171e93712f9a946a0b15f148f19f`

R03 supersedes R01/R02.
Preserve xBGR444 source interpretation:
`0x00F=red, 0x0F0=green, 0xF00=blue`.
Final target is VDP2 NBG2 / CRAM mode 2 RGB888.
Do not reintroduce the R02 R/B swap.

## WP06 TAG exchange visual R04

Active external checkpoint:
`WP06-TAG-EXCHANGE-VISUAL-R04-AUDITED(1).zip`
SHA256:
`bcb5c35c1d404b88b4bafc3124dd6797b62487d6edffe4ec0073c13932b7807b`

Visual actions `$4C/$4D/$4E/$66` are ROM-exact closed.
Gameplay remains owner of legal/reserve state and the exact runtime predicates for `$188BC`.
Never restore WIP160 synthetic ST_PUNCH/18-tick slap/48-tick traversal/late swap/±16 apron correction.
Warrior `$0062/$0063` have all 23 ROM pieces; do not patch an arm.


## RING / ROPES / CAMERA R02

Active side-rope authority:
`RING-ROPES-CAMERA-R02-ROM-PERFECT-AUDITED.zip`

SHA256:
`20a54f5994de27d1e0114c62c9748e94d3b2a8138551eb4aaa8d687f7b19c44c`

Integration Master carries:
- `src/runtime/ring_rope_rom_exact.c/.h`;
- `tests/platform/test_ring_rope_rom_exact.c`;
- `tools/audit_ring_rope_rom.py`.

Do not restore any WIP/R01 synthetic deformation formula.

Exact dynamic side ropes:
`$1C1134/$1C1164/$1C1194/$1C11C4`.

Gameplay must produce the exact state writes; Ring/Ropes advances the ROM selector/timer state machine; WP26 renders the result.

No equivalent global north/south deformation object is demonstrated. Do not fabricate one.

Hardware validation remains pending.


## WP02 ANIMATIONS R04

Active authority:
`WP02-ANIMATIONS-ROM-EXACT-R04-AUDITED.zip`

SHA256:
`0a89f62e3670c7acc7d44d07e779b91d64fbf5407ff90648b52e0082e06afbcc`

Bundle:
- branch `wp02-animations-rom-exact-r04-audited`
- HEAD `5a58a537eba5cddc6fc36589f6dfd106a8e6b3d2`

R04 supersedes R03.

WP02 owns and closes:
`animation request -> exact ROM descriptor/frame/HOLD/loop -> exact metasprite composition/resident presentation`.

Critical invariants:
- do not invent finite duration for FF00;
- kind-2 loops never report finite completion;
- raw $4D is EXACT_MATCH;
- Warrior $0062/$0063 are exact 23-piece compositions mapped to resident frames 223/224;
- Earthquake $47 contains no descriptor loop.

Gameplay remains owner of request/re-entry/release conditions.
WP26 remains renderer owner.
WP25 remains palette/final RGB owner.
Hardware validation remains pending.


## Gameplay RUNTIME-17 owner closure

Accepted as **research/ownership closure only**, not as a deployable runtime.

ZIP:
`RUNTIME17-OWNER-CLOSURE-AUDIT.zip`

SHA256:
`996dd714ff3c7307ea6ed2960e36a98b0ffbecc03121c333aeb0da934cf0c447`

Proven:
- referee is fixed actor `$1C11F4` under `$1F914`;
- `$F4C2` action pass must preserve `$1C03E` callback + internal `$1C0C8` animation tick before `$247C/$27B8`;
- raw `$23` is STOMP, not Vertical Splash;
- renderer/object queue is an explicit required contract.

Do not call this a RUNTIME-17 source checkpoint.
Gameplay remains RUNTIME-16 until source integration is delivered and audited.
