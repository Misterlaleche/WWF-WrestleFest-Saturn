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
- rankings / attract presentation / demo: not yet integrated into the master;
- gameplay: intentionally not advanced until the requested frontend blocks are closed.

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

The audited FINAL audio authority establishes:
- arcade audio research/model: CLOSED;
- production music result requires **combined YM2151 + OKIM6295**;
- audited Saturn PCM transport source format: 44.1 kHz, mono, signed PCM16, **big-endian**;
- a finite repeated PCM segment is not ROM-EXACT continuous arcade synthesis;
- command `0x1F` is a special original fade lifecycle and must not be replaced by the old provisional scalar final-PCM gain curve.

Workpack 24 status is source/module complete but hardware-unvalidated. Its audited Saturn contract includes SCSP slot 31 and a 64 KiB double buffer at sound RAM `0x60000..0x6FFFF`.

### AUDIO TEST 01 — real Saturn observation

The diagnostic screen successfully boots on real Saturn, but the captured screen reports:

```
AUDIO BACKEND: ERROR
LAST SFX: 0x3B
CACHE MISSES: 0
STATUS: MUSIC PLAY LOCKED: EXACT SOURCE ABSENT
```

Therefore audible Saturn playback is **not closed**.

The music lock is intentional and correct: the FINAL audio handoff deliberately distributes no ROM/VGM/PCM/PAK payload. Exact music source assets must be generated locally from the user's ROM-derived sound program using the pinned exact renderer pipeline. The test must fail closed rather than playing fake synthesis, a test tone, approximate CDDA or a guessed loop.

### Next audio task

1. Generate at least one exact combined YM2151+OKIM6295 source asset from the ROM-derived audio program using the pinned renderer/provenance rules.
2. Feed it through the audited SCSP slot-31 transport path.
3. Diagnose the current SFX backend error independently.
4. Validate PLAY / STOP / NEXT / PREV and audible output on real Saturn.
5. Only a real-hardware PASS can close the Saturn runtime audio milestone.


## MANDATORY CROSS-CHAT HANDOFF

Before changing the port, read:
- [PROJECT-STATE.md](PROJECT-STATE.md)
- [HANDOFF.md](HANDOFF.md)
- [docs/SATURN-PIPELINE.md](docs/SATURN-PIPELINE.md)

These files define the current block status, graphics/input/audio pipeline rules, hardware validation gate, and integration contracts so parallel chats do not reintroduce superseded diagnostic implementations.
