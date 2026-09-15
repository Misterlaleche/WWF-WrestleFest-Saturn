# WWF WrestleFest arcade -> Sega Saturn

Private development repository for a native Sega Saturn reimplementation of Technos WWF WrestleFest (1991).

## ABSOLUTE AUTHORITY HIERARCHY

1. **SUPREME AUTHORITY: original arcade ROM set `wwfwfest.zip`.**
   - If code, notes, a previous build, a workpack or any other document disagrees with the original ROM, **the ROM wins**.
   - Main 68000 program authority used by the audited research: SHA1 `a726c46f350723ea73158bc78055d17d418b14e2`.
2. **Audited FINAL ROM-EXACT research / decompilation packages stored in the ChatGPT project sources.**
   - The umbrella archive is `zips finales.zip`.
   - The ChatGPT project also contains specialist FINAL packages for audio, presentation/frontend, attract/bio, records/rankings, animations, CPU, victim +$65, CLEAN-R01 and Saturn technical authority.
   - These are high-authority secondary evidence and should be reused aggressively, but they never overrule the original ROM.
3. **Saturn/Yaul technical authority.**
   - Determines how proven arcade behavior is transported to SH-2 / VDP1 / VDP2 / SCSP / SMPC / CDFS.
4. **Current Saturn code and test builds.**
   - Working code never overrules evidence.

## FAIL-CLOSED IMPLEMENTATION RULE

Do not invent provisional gameplay, timings, state transitions, input behavior, RNG, collision rules, frontend logic, animation, graphics behavior, audio behavior or loops merely to make a build appear complete.

If a behavior is not demonstrated, leave it OPEN / incomplete until evidence exists.

Examples of forbidden shortcuts:
- guessed timings;
- synthetic RNG or fixed seeds;
- modernized autorepeat/input behavior;
- approximate collision/state behavior;
- fake frontend transitions;
- invented music loops;
- placeholder music presented as exact;
- CDDA used as a silent replacement for unresolved arcade synthesis;
- host/emulator success presented as real Saturn validation.

## BLOCK WORKFLOW

Development is split into isolated hardware-testable BLOCKs. A BLOCK may have its own BIN/CUE harness for diagnosis, but final integration is performed from source modules and shared Saturn infrastructure. **BIN/CUE images are never concatenated.**

A BLOCK becomes integration-eligible only after:
- traceability to ROM/audited evidence;
- host/build checks where applicable;
- real NTSC-U Saturn + MODE validation for hardware-dependent behavior.

Current frontend direction:
- BOOT: previously validated on real Saturn;
- MODE SELECT: still open; Royal visual effect is strong, TAG effect still needs exact closure;
- ROSTER SELECT: hardware scroll cadence improved; background/layer composition requires revalidation together with roster + scroll;
- rankings / attract presentation / demo: not yet integrated into the master.

## CHATGPT PROJECT-SOURCE EVIDENCE LIBRARY

The working ChatGPT project contains the original ROM plus audited FINAL ZIPs. Important names include:
- `wwfwfest.zip` — SUPREME authority;
- `zips finales.zip` — umbrella audited collection;
- `WWF-Saturn-Audio-FINAL-DEFINITIVE-ROM-EXACT-HANDOFF(1).zip`;
- `WWF-Saturn-Audio-Complete-Integration(1).zip`;
- `WWF-WrestleFest-Saturn-CPU-FINAL-ROM-EXACT-bridge-AUDITED(1).zip`;
- `WWF-WrestleFest-Saturn-Presentation-ROM-EXACT-FINAL-AUDITED(1).zip`;
- `WWF-WrestleFest-Saturn-Attract-Bio-ROM-EXACT-FINAL-AUDITED(1).zip`;
- `WWF-animaciones WrestleFest-Saturn-Animation-ROM-EXACT-FINAL-package.zip`;
- `WWF-WrestleFest-Saturn-Records-ROM-EXACT-FINAL-package(1).zip`;
- `WWF-WrestleFest-Saturn-victim65-ROM-EXACT-FINAL-package.zip`;
- `WF-CLEAN-R01-MASTER-WORKPACK(1).zip`;
- `WF-CLEAN-R01-ADDON-30-34-MASTER-WORKPACK(1).zip`;
- `SATURN-TECH-AUTHORITY-R01(1).zip`.

These evidence ZIPs do not all need to be duplicated into GitHub. GitHub should store canonical state, integration code, selected source modules, tests/manifests and provenance references. If a future worker cannot access a required ChatGPT project ZIP, they must stop and request it rather than silently replacing its conclusions with assumptions.

## AUDIO — CANONICAL CURRENT STATE

Detailed evidence and chronology: [docs/AUDIO-HARDWARE-VALIDATION.md](docs/AUDIO-HARDWARE-VALIDATION.md).

The audited FINAL audio authority establishes:
- arcade audio research/model: CLOSED;
- production music is combined **YM2151 + OKIM6295**;
- music commands `0x01..0x12` are the demonstrated production range;
- `0x13..0x1E` are registered but no main-CPU producer is demonstrated;
- `0x1F` is a special original fade lifecycle, not an ordinary music cue;
- complete SFX/voice command range is `0x20..0x81`;
- audited Saturn stream format is 44.1 kHz, mono, signed PCM16, **big-endian**;
- a finite repeated PCM segment is not ROM-EXACT continuous arcade synthesis.

### Real Saturn + MODE hardware status — 2026-09-11

**HARDWARE-PASS:**
- SCSP / poneSound path;
- audible authentic PCM playback;
- master volume 0..15;
- SFX command playback;
- bank-1 material including ring-announcer / presenter voices;
- WP24 music transport on physical SCSP slot 31;
- 64 KiB music ring at sound RAM `0x60000..0x6FFFF`;
- double-buffer refill using the SCSP current-address monitor;
- long continuous PCM16BE playback without skips or premature cuts in AUDIO COMPLETE TEST 05.

**ROM-EXACT validation:**
- the canonical A53 validation pack is **18 x 12-second validation renders**, not final production tracks;
- A53 `MUSIC.PAK` SHA256:
  `8e1c038b00f9800753d3d21eca47f2cace9a04d04c07280d7d4b9110b0fd27bf`;
- the checkpoint renderer preserves YM2151 + OKI state across render segments;
- TEST-05 long renders for commands `01,02,04,06,07,0A,0B,0E` revalidate their first 12 seconds exactly against canonical A53 hashes;
- real Saturn playback of those long renders was reported clean, without the prior cuts or scheduler-induced skipping.

**Still OPEN:**
- final production lifetime/termination semantics for every music cue;
- game-state dispatch of music/SFX/voices;
- exact `0x1F` fade lifecycle in the integrated runtime;
- production packaging/generation strategy for continuous music;
- final integrated gameplay audio validation.

Do **not** relabel the A53 12-second validation pack or TEST-05 long diagnostic captures as final production loop assets.

Reusable WP24 transport source now lives under:
- [src/audio/wp24/](src/audio/wp24/)

The TEST-05 hardware audit manifest is stored under:
- [tests/audio/complete-test-05/](tests/audio/complete-test-05/)

No ROM, `AUDPCM.PAK`, `LONGMUS.PAK`, generated PCM or protected audio payload is committed to this repository.

## GAMEPLAY RUNTIME — RUNTIME-17 STAGING

Current staging branch: `gameplay-runtime17-staging`.

Classification: **RUNTIME-17 STAGING ADVANCED / PARTIAL CONSOLIDATION**.

This branch is explicitly:
- **NOT CANONICAL**;
- **NOT RUNTIME-18**;
- **NOT HARDWARE PASS**.

Active implementation base:

`BLOCK-GAMEPLAY-MASTER-01-RUNTIME-17-7A00-OWNER-INTEGRATED-AUDIT-PASS`

SHA256:

`ebcd26f52a5fa5c4aa5d0f0b71dea8b667c9e5cc5a9e02b8dcd0d037739ab555`

The current scheduler/frame chain is structurally known through the post-`$7A00` owners, branch A/B final-frame paths, `$298EC`, VBlank/ACK, post-ACK `$2017A`, `$F9A` and mandatory inter-frame RNG call `$FA0/$21B4`.

Recent staging gates include GCC strict, Clang strict, ASan/UBSan, GCC `-fanalyzer`, WP35/WP36 regressions, SH-2 strict, full Saturn ELF link and unresolved symbols = 0. These must all be rerun on the final consolidated tree before promotion.

Remaining promotion blockers are narrow and explicit:
1. final `$8ECC` regressions + ROM audit;
2. final `$8CF8` join revalidation;
3. physical consolidation of all post-`$7A00` owners in one tree;
4. `wf_runtime_frame()` end-to-end;
5. final D0..D7 branch A/B composition;
6. clean rerun of all gates and independent review.

Detailed current state:
[docs/RUNTIME17-STAGING-CONTINUATION-2.md](docs/RUNTIME17-STAGING-CONTINUATION-2.md).

Supreme authority remains `wwfwfest.zip`; WIP160/Test255 remain historical only.

## MANDATORY CROSS-CHAT HANDOFF

Before changing the port, read:
- [PROJECT-STATE.md](PROJECT-STATE.md)
- [HANDOFF.md](HANDOFF.md)
- [docs/SATURN-PIPELINE.md](docs/SATURN-PIPELINE.md)
- [docs/AUDIO-HARDWARE-VALIDATION.md](docs/AUDIO-HARDWARE-VALIDATION.md)
- [docs/GAMEPLAY-RUNTIME-R01.md](docs/GAMEPLAY-RUNTIME-R01.md)

These files define the current block status, graphics/input/audio/gameplay pipeline rules, hardware validation gate, and integration contracts so parallel chats do not reintroduce superseded diagnostic implementations.
