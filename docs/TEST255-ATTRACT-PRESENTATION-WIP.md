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

This proves producer identities and selection mechanics. It does **not** prove semantic
phrase labels for `0x7B/0x7C/0x7D/0x73` beyond their producer path. Those semantics remain
separate from the dedicated state-entry Demolition/presenter interstitial described below.

## Attract structure refinement

The 14-state attract controller remains:
- states 0/1: the two special TAG/Royal attract states (no biography character);
- states 2..13: the twelve wrestler-featured biography/demo states.

Therefore the two Demolition/records sequences are resolved inside transitions around the
special state-0/state-1 flow rather than invented as additional attract states.

## Royal state-1 six-actor correction

The generic `$0F3E` table exposes the first four entries for every attract state, but
state 1 takes a ROM-proven special branch at `$0E2C -> $0ECC`. That path sets the
Royal mode word and starts a DBF loop with `D0=5`, creating six actor records. Their
IDs come from `$0F76`:

`03 06 07 08 09 02` = Boss Man, Slaughter, Perfect, Earthquake, DiBiase, Jake.

`arcade_attract_state_desc_t` now has `participant_count` and six slots so the exact
Royal roster can be represented without truncation. All other attract states retain
four participants.

## Demolition/presenter interstitial object set recovered

The attract setup path at `$0AC0 -> $0B1C` calls `$AE20` while entering attract states 1 and 2. Further reverse engineering corrects the earlier WIP5 interpretation: `$AE20` creates **three simultaneous presentation objects**, all using graphics set `$2C`, and `$B19C` is a three-object script pointer table rather than a mode-to-script table.

ROM-exact object layout and scripts:

- object 0 / left: x=`$48`, resource/palette family `$33`, script `$B1A8`, 29 phases / 353 active ticks, frames `$16..$1E`;
- object 1 / centre: x=`$98`, resource/palette family `$32`, script `$B1E2`, 62 phases / 707 active ticks, frames `$0A..$15` plus `$1F`;
- object 2 / right: x=`$F8`, resource/palette family `$31`, script `$B25E`, 30 phases / 431 active ticks, frames `$00..$09`.

The `$32` family decodes visually from the original graphics ROM as the presenter; `$31` and `$33` are the two Demolition members. Mode 1 sets the audio-table cursor to 2 and sets object2 `+$1D`, proving the **right** Demolition object is the active/speaking member. Mode 2 sets the cursor to 4 and sets object0 `+$1D`, proving the **left** Demolition object is active/speaking. The dedicated sound table at `$B14A` remains `3179,316A,316B,3178`.

`tools/build_interstitial2c_assets.py` now decodes the complete set `$2C` from the verified original `gfx2` ROM image and generates `INTER2C.PAK` plus three RGB555 palettes. Current exact asset census: **32 frames, 2,288 composition records, 973 unique tiles**. `tools/test255_interstitial_assets.c` validates the generated pack and palette conversion against the original program/gfx data.

**ROM-EXACT:** three-object creation, set `$2C`, object positions, script pointers and phase bytes, resource/palette families, speaker-side selection for mode 1/mode 2, sound-table cursor selection, all 32 decoded composition frames and palette sources.

**REGISTERED BUT NOT IMPLEMENTED:** final Saturn runtime replacement of the old provisional interstitial and a direct internal-ROM proof assigning the names Smash vs Crush to the left/right graphical families. The left/right Demolition identities themselves are graphically demonstrated.
