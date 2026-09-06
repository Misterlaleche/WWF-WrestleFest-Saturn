# Test255 / V0.270 WIP — attract presentation/audio producer audit

## ROM-EXACT closed in this subcheckpoint

The original 68000 program contains an attract/presentation audio producer at `$AAEA`.
The producer is now represented by `arcade_presentation_audio.c/.h` and guarded by
`tools/test255_presentation_audio_rom.c`.

### Script command table

`program.bin $AC56` is a 24-byte table indexed by the local presentation substate:

`00 00 7B 00 FF 00 7C 00 FF 00 00 7D 00 FF 00 7C 00 FF 00 00 00 00 00 00`

At `$AB1E..$AB27`, nonzero/non-`FF` entries are ORed with `$3100` and sent through
`$2052`, proving raw sound-command production for `0x7B`, `0x7C` and `0x7D`.

### Character command table

`program.bin $AC4A` is indexed by wrestler id from actor `+$57`:

| ID | Wrestler | command |
|---:|---|---:|
| 0 | Hogan | `0x6D` |
| 1 | Warrior | `0x6E` |
| 2 | Jake | `0x6F` |
| 3 | Boss Man | `0x74` |
| 4 | Smash | `0x73` |
| 5 | Crush | `0x73` |
| 6 | Slaughter | `0x71` |
| 7 | Perfect | `0x70` |
| 8 | Earthquake | `0x72` |
| 9 | DiBiase | `0x75` |
| 10 | Hawk | `0x73` |
| 11 | Animal | `0x73` |

At `$AB3A..$AB47`, the selected byte is likewise ORed with `$3100` and sent through
`$2052`. `$AB4E..$AB5B` proves special handling for raw command `0x73`, setting
`$1C0168 = $8000`.

## Important non-claim

This proves producer identities and selection mechanics. It does **not** yet prove
which of `0x7B/0x7C/0x7D/0x73` is the spoken line heard in each of the two distinct
Demolition interstitials. `$A654` is part of a broader presentation/game loop, so
semantic speaker/phrase labels remain **REGISTERED BUT NOT IMPLEMENTED** until the
state-0/state-1 interstitial producer chain is independently demonstrated.

## Attract structure refinement

The 14-state attract controller remains:
- states 0/1: the two special TAG/Royal attract states (no biography character);
- states 2..13: the twelve wrestler-featured biography/demo states.

Therefore the two Demolition/records sequences must be resolved inside the special
state-0/state-1 flow rather than invented as additional attract states.

## Royal state-1 six-actor correction

The generic `$0F3E` table exposes the first four entries for every attract state, but
state 1 takes a ROM-proven special branch at `$0E2C -> $0ECC`. That path sets the
Royal mode word and starts a DBF loop with `D0=5`, creating six actor records. Their
IDs come from `$0F76`:

`03 06 07 08 09 02` = Boss Man, Slaughter, Perfect, Earthquake, DiBiase, Jake.

`arcade_attract_state_desc_t` now has `participant_count` and six slots so the exact
Royal roster can be represented without truncation. All other attract states retain
four participants.

## Two state-entry interstitial scripts recovered

The attract setup path at `$0AC0 -> $0B1C` proves that `$AE20` is invoked with
mode 1 while entering attract state 1 and mode 2 while entering attract state 2.
`$AE20` builds three presentation objects (object type `$2C`) and runs mode-specific
scripts through `$B0C8` using the pointer table at `$B19C`.

- mode 1 -> `$B1E2`: 62 phases, 707 active ticks, terminator duration `$FE`.
  The only high-bit event marker is raw frame `$8D` at phase 3. Mode 1 initializes
  the audio-table cursor to 2, so `$B0C8` sends exactly `$316A` via `$2052`.
- mode 2 -> `$B25E`: 30 phases, 431 active ticks, terminator duration `$FE`.
  The only high-bit event marker is raw frame `$83` at phase 1. Mode 2 initializes
  the audio-table cursor to 4, so `$B0C8` sends exactly `$316B` via `$2052`.

The dedicated command table at `$B14A` is `3179, 316A, 316B, 3178`.
These data are represented by `arcade_interstitial.c/.h` and guarded byte-for-byte
by `tools/test255_interstitial_rom.c`.

**ROM-EXACT:** invocation points, script pointers, every frame/duration pair, event
markers, total active timing and raw sound commands.

**REGISTERED BUT NOT IMPLEMENTED:** the exact graphical identity of the two speaking
Demolition members. Arcade captures establish that these are the two distinct
Demolition interstitials in this sequence, but the object-type `$2C` frame graphics
still need to be decoded so Smash/Crush is assigned from ROM graphics rather than
from screen-position inference.
