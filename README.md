# WWF WrestleFest for Sega Saturn

**English** · [Español](README.es.md)

A native Sega Saturn port of Technos' **WWF WrestleFest**. The aim is to preserve the arcade game's graphics, rules, input response, timing and sound on Saturn hardware. This is an active development project; arcade-perfect equivalence remains the acceptance target.

**Current source: P1-03B BOOTFIX CANDIDATE, 22 November 2026.** The repository was refreshed on **25 November 2026** from the latest source package recovered and verified for this update. The boot correction still needs confirmation on a physical Saturn. This repository update does not certify a finished game or a new hardware-tested release.

[Build instructions](docs/BUILDING.md) · [Current status and testing](docs/STATUS.md) · [Source provenance](docs/PROVENANCE.md) · [Project history](docs/HISTORY.md) · [Third-party notices](docs/THIRD-PARTY.md)

## What is here

The repository now contains one integrated source tree: the native gameplay core, Saturn platform code, the project's adapted Jo Engine, asset tools, regression tests and compact development evidence. Earlier independent scene, TAG, Royal and audio snapshots remain accessible through [Git history](docs/HISTORY.md).

| Component | Current implementation |
| --- | --- |
| Target | NTSC Sega Saturn, **4 MB RAM expansion**, Terraonion MODE as the primary hardware test setup |
| SDK | **Jo Engine 2026.3**, with project-specific changes retained in this tree |
| Video mode | **320 × 240** |
| Game logic | Native C reconstruction of the arcade's 68000 behavior, with original state and register semantics where required |
| Rendering | VDP1 sprites and command submission; VDP2 backgrounds, ring and scene layers |
| CPU work | Master SH-2 gameplay and frame preparation; slave SH-2 work through Jo's command and asset-decode queues |
| Audio | SCSP PCM playback: 18 music cues streamed from data files, plus decoded effects and voices |
| Build output | `game.bin` and `game.cue`, with intermediate ELF, raw executable and ISO files |

The current target build uses Jo Engine. Earlier Yaul work is part of the project's history and some retained reference code; it is not the SDK linked by the current `makefile`. The 4 MB expansion is required by this revision, even though some earlier work targeted an unexpanded Saturn.

## Latest work included

P1-03B includes the preceding P0-01–P0-06 gameplay/diagnostic work, GFX1, and the P1 updates. The dated reports under [`docs/`](docs/) explain their scope and evidence.

- **Boot diagnosis and correction candidate.** A physical test of the preceding build stopped after the complete `SPRITES.WFP` read, reporting owner `0018CD10`. P1-03B returns the SH-2 Adler-32 routine to the project's normal `-O2` compilation and adds detailed failure phases to the asset diagnostic. The suspected cause is documented; successful boot of this candidate remains unconfirmed.
- **Music preparation.** All 18 music cues were regenerated from the reference sound implementation with band-limited conversion from 176.4 kHz to 44.1 kHz. This addresses the aliasing introduced by the preceding simple decimation.
- **Music fade.** Command `0x1F` now applies a five-second SCSP volume fade and then stops the voice. This is an implemented approximation; exact equivalence to the arcade driver's envelope is still open.
- **Memory budget.** Selected cold code uses size optimization while active paths retain their required compilation policy. The recovered P1-03B map records **8,208 bytes** before the reserved `0x060DC000` boundary, against an 8,192-byte minimum. This is the original build's recorded margin, not a new measurement of console performance.
- **TAG progression evidence.** The recovered regression reaches the second match using ordinary test-driver button inputs. That result does not close the complete TAG or Royal campaign acceptance work.

See the [BOOTFIX report](docs/p1-03b-bootfix-20261122.md), [P1-03 audio report](docs/p1-03-audio-bandlimit-and-fade-20261121.md), and [TAG progression correction](docs/p1-01-tag-progression-correction-20261121.md).

## Why this port takes more than translating graphics

The arcade's original behavior has to survive a change from its 68000-based board to Saturn's SH-2 processors and separate graphics and sound memories. Actor ordering, collision and input rules, random-number state, scene transitions and sound commands all have to agree with the original game. The original ROM and audited traces are the behavioral reference; a convenient approximation is not evidence of equivalence.

On Saturn, the same work also has to fit the frame and memory budgets. Sprite data is packed and cached, command buffers must remain valid until VDP1 has consumed them, and music streaming has to be serviced while gameplay and asset decoding continue. Host tests, differential checks and Mednafen help expose errors. Physical Saturn testing remains necessary for boot, disc access, sustained cadence, audio continuity and final image behavior.

## Build your own copy

Start with the [English build guide](docs/BUILDING.md) or [Spanish build guide](docs/BUILDING.es.md). The documented toolchain is GNU/Linux x86-64, GNU Make, Python 3 and the pinned SH-2 GCC 8.2 supplied with Jo Engine 2026.3.

You must provide two external inputs:

1. The exact Jo Engine SDK archive accepted by [`tools/setup_compiler.py`](tools/setup_compiler.py).
2. A matching P1-03/P1-03B disc or prepared assets that satisfy [`asset-lock.json`](asset-lock.json).

The current lock describes **34 files totalling 632,644,966 bytes**. ROMs, graphics/audio payloads, BIOS files, disc images and compiler executables are not supplied here. Owning an arbitrary `wwfwfest.zip` alone does not make this checkout a complete one-command asset-generation pipeline: some original generation inputs are external. The documented build route restores the verified prepared assets from a matching disc.

The adapted [`sdk/Compiler/COMMON/`](sdk/Compiler/COMMON/) is included. It was missing from the recent source ZIPs and has been restored from the verified inherited R03 configuration. Do not overwrite it, or [`sdk/jo_engine/`](sdk/jo_engine/), with an unmodified SDK.

## Controls

For the original frontend build (`WF_FRONTEND=1`):

| Saturn input | Action |
| --- | --- |
| D-pad | Movement and menu selection |
| A | Original A input / punch |
| B | Original B input / kick |
| C | Insert a credit; press and release |
| START | Start or join when the current game state and credits permit |

The input adapter also contains multiplayer routing and tests. Their presence does not certify every multiplayer route on physical hardware. The diagnostic selector compiled without `WF_FRONTEND=1` is a different entry point; use the documented frontend flags for normal play.

## What still needs work

- Confirm that P1-03B passes the asset-loading stage and reaches the title on real Saturn.
- Finish full TAG and Royal route acceptance, including all interactions, transitions and end states. Records, initials, credits and save code are present, but their complete physical validation remains open.
- Bring sustained combat cadence and frame pacing to the arcade target, especially under Royal's heavier load. Older emulator percentages are historical measurements, not guarantees for this candidate or a real console.
- Finish image, animation, priority and timing comparisons across the full game.
- Validate the revised music on hardware, eliminate remaining audio interruptions and resolve exact musical loops and lifecycle behavior. Long music tracks are still finite **600-second captures**.

The [status document](docs/STATUS.md) separates recovered build evidence, checks run for this repository refresh, and hardware work still pending. A passing host test is not a hardware pass.

## Repository map

| Path | Purpose |
| --- | --- |
| [`core/`](core/) | Native gameplay/state logic, runtime tables and reference-compatible contracts |
| [`port/`](port/) | Saturn entry point, graphics, assets, audio, input and save adapters |
| [`sdk/jo_engine/`](sdk/jo_engine/) | Adapted Jo Engine sources |
| [`sdk/Compiler/COMMON/`](sdk/Compiler/COMMON/) | Adapted build rules, linker and Jo boot template |
| [`tools/`](tools/) | Asset restoration/generation, capture analysis and packaging tools |
| [`tests/`](tests/) | Host regressions, intentional frozen reference implementations and RAM fixtures |
| [`docs/`](docs/) | Build guides, technical records, status, provenance and history |
| [`evidence/`](evidence/) | Compact, revision-labelled reports and logs |

## Contributing and reporting hardware results

Useful contributions include small reproducible fixes, performance measurements with a clear methodology, ROM-backed behavior comparisons, and precise hardware reports. Include the source commit or disc hash, console region, RAM cartridge, MODE or CD-R/loader path, the steps taken, and the complete diagnostic screen if one appears. See [CONTRIBUTING.md](CONTRIBUTING.md).

Keep gameplay changes tied to evidence. Preserve the current build workarounds, memory reservations and third-party notices. Do not replace missing behavior with a guessed rule and label it arcade-exact.

## Credits and rights

This is an independent development project and is not affiliated with or endorsed by the original game's rights holders or Sega. Original names, characters, graphics and sound remain the property of their respective rights holders. No license to those materials is granted by this repository.

Jo Engine, zlib, the Z80 reference and Nuked-OPM retain their own notices and licensing terms. See [third-party notices](docs/THIRD-PARTY.md). The repository does not apply a new blanket license to all imported code or original game materials.
