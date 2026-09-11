# RUNTIME AUDIO HANDOFF R01 — MASTER INTEGRATION CONTRACT

External package:
`WWF-RUNTIME-AUDIO-HANDOFF-R01-COMPLETE(1).zip`

External ZIP SHA256:
`cc1c9ed44edcde0237770521123072c3bbec6927bf37184c157e7b092858db65`

Package self-audit:
- 24/24 listed internal SHA256 entries verified;
- authority package ZIP integrity: PASS;
- producer owner-map ZIP integrity: PASS;
- producer ROM-audit ZIP integrity: PASS;
- GitHub bridge host compile with `-std=c11 -Wall -Wextra -Werror`: PASS;
- no BIN/CUE;
- no WIP160/175/176 imported;
- no unresolved producer promoted.

## Integration boundary

Exactly **14 producer rows are INTEGRATION_READY**.

### WP10
- `0x009088 -> 0x0B`, call `0x00908C`: low-stamina warning.
- dynamic call `0x0090CE`: stage-music restore dispatch.

### WP08
- dynamic call `0x01FE44`, raw `+$54 = 0x3165..0x3168` -> commands `0x65..0x68`.
- `0x01FE96 -> 0x08`, call `0x01FE9A`.
- `0x01FEE6 -> 0x08`, call `0x01FEEA`.
- `0x01FF36 -> 0x08`, call `0x01FF3A`.

### WP01
- Perfect Plex `0x0152BA -> 0x32`.
- Smash Suplex `0x0164B8 -> 0x2B`.
- Smash Suplex `0x0164C2 -> 0x32`.
- Gorilla Press `0x016DBE -> 0x32`.
- DDT `0x01755A -> 0x32`.
- Hogan Legdrop `0x017874 -> 0x2B`.
- Hogan Legdrop `0x01787E -> 0x32`.

### WP11
- stage-sensitive result selector `0x02016E -> 0x05`, call `0x020172`.

## Ownership rule

WP24 owns the Saturn audio backend. It does not own the gameplay/frontend semantic trigger.

Insertion must be:
`proven CLEAN-R01 state/callback/branch -> original arcade command byte -> wf24_command_dispatch()`.

Do not add a WP28 audio phase.
Do not add a per-frame semantic observer.
Do not infer producer identity from command-byte reuse or audible similarity.

## Critical correction

Low-stamina is:
- producer `0x009088`
- JSR `0x00908C`
- command `0x0B`

`0x0263CC -> 0x0B` is **REGISTERED_NOT_IDENTIFIED** and must be rejected as low-stamina.

## Blocked audio rows

- 4 rows: `OWNER_SCOPE_ONLY`
- 155 rows: `UNRESOLVED`

They remain fail-closed until the exact trigger is proven.

## Music

- commands `0x01..0x12`: demonstrated production range.
- source model: combined YM2151 + OKIM6295.
- Saturn transport: physical slot31, PCM16BE mono 44.1 kHz, 64 KiB ring, CA-driven refill.
- long TEST-05 transport: HARDWARE-PASS.
- final production lifetime/stop semantics: OPEN.
- exact `0x1F` fade lifecycle: OPEN.
- A53/TEST-05 validation captures are not final production loops.

## SFX / voices

Authority remains `WWF-Saturn-Audio-Complete-Integration(1).zip`.

- command range `0x20..0x81`;
- bank0: 73 samples;
- bank1: 41 logical samples;
- bank1-exclusive logical IDs 20..41 / commands `0x6A..0x7F`;
- presenter/ring-announcer voices have been audible on real Saturn.

## Master responsibility

The integration master preserves WP24, memory ownership, CDFS, and final assembly.
The gameplay owner inserts only those hooks whose CLEAN-R01 owner/callback now exists.
If the owner does not yet exist, mark `BLOCKED_BY_RUNTIME_OWNER`; do not fabricate a substitute.

## Remaining gates

- integrate the 14 ready rows into their exact CLEAN-R01 owners;
- document ROM producer/call -> command -> CLEAN-R01 owner -> runtime function -> test;
- host PASS;
- SH-2 compile PASS;
- Saturn ELF link PASS;
- 0 unresolved symbols;
- full simultaneous music + SFX + voice validation under gameplay load;
- final real-Saturn integrated audio validation.
