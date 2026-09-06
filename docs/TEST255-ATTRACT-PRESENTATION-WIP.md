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
