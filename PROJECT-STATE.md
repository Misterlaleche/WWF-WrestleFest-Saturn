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
- roster hardware-scroll cadence in the dedicated block before the background-layer regression;
- SCSP authentic PCM playback;
- master-volume control;
- SFX and bank-1 voice/presenter playback;
- WP24 slot-31 PCM16BE music streaming with 64 KiB double buffer;
- long continuous music playback in AUDIO COMPLETE TEST 05 without skips or premature cuts.

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
Status: TRANSPORT HARDWARE-PASS / RAW RUNTIME BRIDGE READY / GAMEPLAY SOURCE INSERTION OPEN.

Proven on real NTSC-U Saturn + Terraonion MODE:
- authentic SCSP PCM playback;
- master volume 0..15;
- SFX command path;
- bank-1 audio including presenter/ring-announcer voices;
- 44.1 kHz mono signed PCM16 big-endian streaming;
- physical SCSP slot 31;
- 64 KiB ring at sound RAM `0x60000..0x6FFFF`;
- CA-driven double-buffer refill;
- long continuous TEST-05 music playback without audible skips or premature cuts.

Runtime bridge now present:
- original command-byte dispatcher;
- raw stage music words from ROM table `0x000DEE..0x000E00`;
- low-stamina producer anchored to ROM PC `0x0263CC -> 0x0B`;
- ring-out raw `+$54` mapping `0x3165..0x3168 -> 0x65..0x68`;
- proven finisher/heavy call-sites only;
- no semantic `fighters[0]` observer imported.

Exactness status:
- A53 is a validation pack of 18 x 12-second renders, **not** the final production MUSIC.PAK;
- A53 validation-pack SHA256:
  `8e1c038b00f9800753d3d21eca47f2cace9a04d04c07280d7d4b9110b0fd27bf`;
- checkpoint rendering preserves continuous YM2151+OKIM6295 state;
- commands `01,02,04,06,07,0A,0B,0E` were extended to approximately 51.79-60.00 s for TEST-05 and their canonical first 12 seconds remain exact A53 PASS;
- TEST-05 BIN SHA256:
  `a7b8e4315e5f33abf54a97550fbeebfdf99c15277db6e4767f9dba39edb83951`.

Still open:
- insertion into the current canonical CLEAN-R01 gameplay source;
- production lifetime/termination for every cue;
- exact remaining game-state command producers;
- exact `0x1F` fade lifecycle;
- production source packaging/generation for continuous music;
- final integrated gameplay hardware validation.

See `docs/AUDIO-HARDWARE-VALIDATION.md` and `docs/RUNTIME-AUDIO-RECONCILIATION.md`.

## Canonical runtime source status

The private repository still does **not** contain the current canonical gameplay runtime implementation. CLEAN-R01 FINAL workpacks define contracts, scheduler order and many exact behaviors, but are not themselves one complete linked gameplay source tree.

WIP160/175/176 must not be promoted as the canonical base to fill that gap.

## Integration rule

Do not build gameplay on top of unresolved frontend approximations. Current user direction is to close the non-game frontend in blocks first, then integrate into a FRONTEND-MASTER build, then proceed toward gameplay.

Audio transport and raw producer hooks are integration-eligible. The diagnostic long renders are **not** final game assets.

## Never use as authority

Do not treat an old WIP or visually plausible diagnostic implementation as truth when it disagrees with ROM evidence. In particular, do not reintroduce known provisional frame-video title playback, full-frame DMA roster scrolling, guessed input repeat, fake music, synthetic RNG, invented music loops, the TEST-03 fixed-time audio refill scheduler, or the historical slot-0 audio observer as ROM truth.
