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

The private repository still does **not** contain the current canonical gameplay runtime implementation. CLEAN-R01 FINAL workpacks define contracts and many exact behaviors but not one complete linked gameplay source tree.

WIP160/175/176 must not be promoted as canonical bases.

## Integration rule

Audio transport and the 14 ready producer hooks are integration-eligible. `OWNER_SCOPE_ONLY` and unresolved rows remain fail-closed until proven.
