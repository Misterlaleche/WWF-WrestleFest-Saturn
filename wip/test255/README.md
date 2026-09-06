# Test255 / V0.270 WIP — attract state engine

This is WIP evidence/code, not a canonical checkpoint.

## ROM-EXACT recovered data now executable
- original 14-state duration table at program ROM offset `0x119A`: `848, 736, 1040, 928, 656, 928, 1200, 944, 1616, 1152, 800, 816, 832, 864`;
- states 2–13 protagonist order: Hogan, Warrior, Jake, Boss Man, Slaughter, Perfect, Earthquake, DiBiase, Hawk, Animal, Smash, Crush;
- overlay producer around `0x8CF8` selects TAG text ID `0x29`, graphics block `0x15`, palette bank 7 and Royal text ID `0x2A`, graphics block `0x16`, palette bank 6.

Host test passes for the executable 14-state scheduler.

## REGISTERED BUT NOT IMPLEMENTED / not yet promoted
- exact mapping of the two Demolition speaker segments to per-speaker animation/audio events;
- exact TAG-records vs Royal-records placement/data;
- Hawk/Animal C10/C11 roster/attract packs;
- final runtime integration replacing the old fixed 1800-tick demo and fixed cast.

No BIN/CUE generated.
