# PROJECT STATE

## Current development policy

Native ROM-EXACT reimplementation of WWF WrestleFest (1991) for Sega Saturn NTSC-U. Supreme authority: original `wwfwfest.zip` ROM.

## Hardware-proven foundation

Real Saturn + Terraonion MODE has validated boot/VDP foundation plus:
- authentic SCSP PCM playback;
- master volume;
- SFX and bank-1 presenter/ring-announcer voices;
- WP24 slot-31 PCM16BE streaming with 64 KiB double buffer;
- CA-driven refill;
- long continuous TEST-05 music playback without skips/premature cuts.

## Current frontend status

- **MODE + ROSTER TEST03: HARDWARE-PASS on real Saturn NTSC-U + Terraonion MODE (2026-09-11).**
- Combined route validated: `MODE -> TAG/ROYAL confirm -> ROSTER -> wrestler confirm`.
- MODE -> ROSTER yellow/orange transition flash: CLOSED by display blank/sync before ROSTER CRAM/VRAM reconfiguration.
- ROSTER transparency/background composition: HARDWARE-PASS (transparent NBG0 moving foreground + fixed NBG2 background).
- ROSTER wrestler confirmation: HARDWARE-PASS (bright-green confirmation, cursor preserved).
- TEST02 confirmation flash / temporary full NBG0 disappearance: CLOSED in TEST03 by resident foreground + localized 96x96 VBLANK-IN patch.
- Master/runtime import: OPEN. Import the proven behavior/source contract; do not replace the canonical main loop with the isolated harness.
- TITLE / INTRO: PARTIAL.
- RANKINGS / ATTRACT: NOT INTEGRATED.

## AUDIO

Status: **AUDIO ROM-EXACT SOFTWARE CLOSED (R04 MASTER-REAUDITED) / GAMEPLAY SOURCE INSERTION OPEN / REAL-SATURN FINAL VALIDATION PENDING.**

Canonical audio package:
- `WWF-SATURN-AUDIO-ROM-EXACT-SOFTWARE-CLOSED-R04-MASTER-REAUDITED.zip`
- SHA256 `c4898b654eb105a88cc24500de96c00b68d188582aa7d7f7dbb13f1b3315ae36`
- previous R04 ZIP SHA `b23f43424407c8379a45c1e6fcbb482065bf0c429856ad988ca896d2e596e5c0` is superseded due only to invalid audit-manifest packaging (absolute paths + missing `SHA256SUMS.txt.tmp` reference); implementation unchanged.

### Hardware-close gate / ownership

R04 status is **AUDIO SOFTWARE CLOSED — MASTER REAUDITED**.

AUDIO HARDWARE CLOSED is forbidden until:
- Live Z80 + Nuked-OPM performance is validated with Gameplay + VDP1 + VDP2 simultaneously;
- final integrated CPU and memory budget is verified;
- end-to-end runtime audio integration is complete;
- all unresolved producer mappings remain fail-closed;
- final validation passes on real Sega Saturn hardware.

Ownership:
- WP24 / Audio owns backend, transport, Z80/OPM VM, reproduction and audio behavior.
- Gameplay / Scene Flow / other blocks own their event producers.
- Audio never invents producer semantics or mappings.
- UNRESOLVED stays UNRESOLVED until ROM proof exists.

### Producer matrix

- 173 total ROM-verified producer rows.
- 163 immediate producers byte-verified.
- 10 dynamic/dataflow producer call sites verified.
- 154 rows remain `REGISTERED_NOT_IDENTIFIED`.
- 12 are `RECOVERED_ROM`.
- 7 finisher rows are `RECOVERED_ROM_USAGE`.

### CLEAN-R01 owner map R01

- 14 rows: `INTEGRATION_READY`.
- 4 rows: `OWNER_SCOPE_ONLY`.
- 155 rows: `UNRESOLVED`.
- 0 unidentified rows promoted to integration-ready.

Ready ownership:
- WP10: low-stamina + stage-music restore.
- WP08: ring-out/final count + terminal countout branches.
- WP01: seven recovered finisher/heavy producers; WP22/WP10 supporting.
- WP11: stage-sensitive result selector.
- WP24: backend for all audio rows, not event-semantic owner.

Critical correction:
- low-stamina producer = `0x009088 -> 0x0B`, JSR `0x00908C`;
- `0x0263CC -> 0x0B` remains `REGISTERED_NOT_IDENTIFIED`.

R04 software closure:
- live Z80 + pinned Nuked-OPM music VM for commands `0x01..0x12`;
- 18/18 first-12-second outputs byte-exact against canonical A53 hashes;
- optional live-source callback feeds the existing CA-driven WP24 slot31 ring;
- `0x1F` returns to the original driver VM rather than a PCM-domain approximation;
- no short PCM loop object is introduced.

Still open outside the audio engine itself:
- insert proven producer hooks into the canonical CLEAN-R01 gameplay source;
- keep scope-only/unresolved producer semantics fail-closed until their owning runtime work resolves them;
- final integrated real-Saturn validation of live VM + music + SFX + voices under full gameplay/VDP load.

See:
- `docs/AUDIO-HARDWARE-VALIDATION.md`
- `docs/RUNTIME-AUDIO-RECONCILIATION.md`
- `docs/AUDIO-PRODUCER-MATRIX-AUDIT.md`
- `docs/AUDIO-PRODUCER-CLEANR01-OWNER-MAP.md`
- `docs/AUDIO-ROM-EXACT-R04-CLOSURE.md`

## Canonical runtime source status

The private repository still does **not** contain the current canonical gameplay runtime implementation. CLEAN-R01 FINAL workpacks define contracts and many exact behaviors but not one complete linked gameplay source tree.

WIP160/175/176 must not be promoted as canonical bases.

## Integration rule

Audio transport and the 14 ready producer hooks are integration-eligible. `OWNER_SCOPE_ONLY` and unresolved rows remain fail-closed until proven.

MODE + ROSTER TEST03 is also integration-eligible as a **hardware-proven combined frontend checkpoint**. Preserve its rendering/input invariants when moving it under the canonical runtime: no BIN/CUE concatenation, no full NBG0 confirmation rebuild, display blank/sync before ROSTER CRAM/VRAM changes, and localized 96x96 confirmation update at VBLANK-IN.

See `docs/MODE-ROSTER-HARDWARE-VALIDATION.md`.
