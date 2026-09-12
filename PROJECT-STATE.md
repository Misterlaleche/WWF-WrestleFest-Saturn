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

- BLOCK-MODE-01: CANDIDATE / OPEN; TAG effect still differs.
- BLOCK-ROSTER-01: CANDIDATE / OPEN; BGFIX requires real Saturn revalidation.
- TITLE / INTRO: PARTIAL.
- RANKINGS / ATTRACT: NOT INTEGRATED.

## AUDIO

Status: **TRANSPORT HARDWARE-PASS / PRODUCER MATRIX ROM-AUDITED / OWNER MAP R01 CLOSED / GAMEPLAY SOURCE INSERTION OPEN.**

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

Still open:
- insert the 14 ready hooks into canonical CLEAN-R01 gameplay source;
- prove semantics/trigger for the 4 scope-only rows;
- classify the 155 unresolved producers;
- exact `0x1F` fade lifecycle;
- production lifetime/termination for all music cues;
- integrated gameplay hardware validation.

See:
- `docs/AUDIO-HARDWARE-VALIDATION.md`
- `docs/RUNTIME-AUDIO-RECONCILIATION.md`
- `docs/AUDIO-PRODUCER-MATRIX-AUDIT.md`
- `docs/AUDIO-PRODUCER-CLEANR01-OWNER-MAP.md`

## Canonical runtime source status

The private repository contains the active gameplay runtime candidate on branch `gameplay-runtime-r01`. It is not yet FINAL. WIP160/175/176 must not be promoted as canonical gameplay bases.

## Integration rule

Audio transport and the 14 ready producer hooks are integration-eligible. `OWNER_SCOPE_ONLY` and unresolved rows remain fail-closed until proven.

## GAMEPLAY RUNTIME

Status: **GAMEPLAY-R01 CANDIDATE / source integration active**.

Current checkpoint: `BLOCK-GAMEPLAY-MASTER-01-RUNTIME-16`, cumulative from frozen RUNTIME-11.

Build gates at this checkpoint:
- host tests PASS (`-std=c11 -Wall -Wextra -Werror -pedantic`);
- SH-2 cross compile PASS; full Saturn ELF link PASS (41 objects);
- unresolved symbols: 0;
- BIN/CUE: not generated.

RUNTIME-16 integrates ROM-derived WP28 owners `$00F560` and `$00F18A`. Real branch-A integration now reaches `$514E -> $8B3A -> $2088E -> $20958 -> $26936 -> $2983C -> $F560 -> $F18A -> $1C150` and stops fail-closed at `$1F914`, the fixed referee scheduler. Shared RNG `$21B4` remains blocked until its exact live 68000 register context is proved.

This does **not** mean gameplay is complete. Direct-match actor/global bootstrap remains BLOCKED. TAG, Royal, full pin/referee, ring-out, remaining collision/release paths, complete WP28 ownership, final VDP2 world/ring, and hardware gameplay validation remain open/partial.

CPU-FINAL closed paths remain integrated without synthetic RNG. State5 callback execution exists for the demonstrated portions of all mapped callback families; unknown downstream dependencies remain explicit BLOCKED exits.

WIP160/WIP175/WIP176 remain historical references only and must not replace this runtime base.
