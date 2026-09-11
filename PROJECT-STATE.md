# PROJECT STATE

## Current development policy

Native ROM-EXACT reimplementation of WWF WrestleFest (1991) for Sega Saturn NTSC-U. No full arcade-board emulation.

Supreme authority: original `wwfwfest.zip` ROM.

## Hardware-proven foundation

Real Saturn + Terraonion MODE has validated:
- bootable Saturn BIN/CUE pipeline;
- VDP2 visible output;
- boot/frontend sequence through FBI -> legal -> Technos/Tecmo -> TITLE in prior tests;
- credit/start handoff and mode/roster entry in earlier frontend tests;
- roster hardware-scroll cadence in the dedicated block before the background-layer regression.

## Current BLOCK status

### BLOCK-MODE-01
Status: CANDIDATE / OPEN.
- ROYAL selection effect visually close to arcade.
- TAG selection effect still differs from the arcade capture.
- Root lesson: do not animate TAG from an RGB-flattened capture; preserve indexed palette-bank/pen provenance and ROM descriptor phases.

### BLOCK-ROSTER-01
Status: CANDIDATE / OPEN.
- Cursor/navigation semantics were corrected toward ROM edge behavior.
- 208 px page scroll = 26 updates x 8 px.
- Hardware-scroll implementation fixed the prior severe slowdown.
- First hardware-scroll candidate lost the fixed background.
- Current BGFIX candidate moves roster via NBG0 hardware scroll and reconstructs fixed background on NBG2; requires real Saturn revalidation.

### TITLE / INTRO
Status: PARTIAL.
- Boot plates and static title path have hardware history.
- Full arcade title/intro animation is not yet closed.
- CREDIT: 0 and complete no-game frontend cycle still need block-level closure.
- Do not reuse TITLE.ANM streaming as final architecture.

### RANKINGS / ATTRACT
Status: NOT INTEGRATED.
Planned independent blocks:
- TAG ranking
- Royal ranking
- attract presentation/interstitials
- attract master controller
- playable demo later, after non-game frontend is closed

### AUDIO
Status: OPEN on Saturn runtime.
- FINAL ROM audio research exists.
- Exact production music requires combined YM2151 + OKIM6295 rendering.
- Current diagnostic screen can boot, but music remains intentionally locked when exact source payload is absent.
- Audio backend/SFX path still requires hardware closure.

## Integration rule

Do not build gameplay on top of unresolved frontend approximations. Current user direction is to close the non-game frontend in blocks first, then integrate into a FRONTEND-MASTER build, then proceed toward gameplay.

## Never use as authority

Do not treat an old WIP or visually plausible diagnostic implementation as truth when it disagrees with ROM evidence. In particular, do not reintroduce known provisional frame-video title playback, full-frame DMA roster scrolling, guessed input repeat, fake music, or synthetic RNG.
