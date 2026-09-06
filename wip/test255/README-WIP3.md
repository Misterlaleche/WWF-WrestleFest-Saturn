# Test255 WIP3

ROM-exact attract/presentation audio producer tables are now represented by `arcade_presentation_audio.c/.h` and checked directly against `program.bin` by `test255_presentation_audio_rom.c`.

Closed evidence:
- `$AC56` local-substate command table (`0x7B`, `0x7C`, `0x7D` producer entries).
- `$AC4A` wrestler-id command table (`0x6D..0x75` mapping; Smash/Crush/Hawk/Animal share raw `0x73`).
- `$AB1E/$AB3E` OR `$3100` then call `$2052`.
- `$AB4E` special `0x73` handling sets `$1C0168=$8000`.

Still open: exact semantic mapping of the two Demolition interstitial speaker animations/voice lines. Do not infer this from shared `0x73` alone.
