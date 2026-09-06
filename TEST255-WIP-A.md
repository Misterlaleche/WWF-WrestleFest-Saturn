# Test255 / V0.270 — WIP checkpoint A

This is a **real persisted WIP checkpoint**, not the canonical baseline. The canonical validated baseline remains **Test254 / V0.269**.

## Source snapshot

- Library: `/WWF-Saturn/WWF-Saturn-Test255-V0270-WIP-A-source.zip`
- SHA256: `0e8b5151a246c3f874342c4406046c25afd882c52cfb5455eed94493086ac127`
- BIN/CUE: **not generated**.

## ROM-EXACT recovered and integrated in this WIP

- Exact 14-state attract duration table at ROM `$119A`:
  `848, 736, 1040, 928, 656, 928, 1200, 944, 1616, 1152, 800, 816, 832, 864`.
- Exact four-actor state roster table at `$0F3E`.
- Exact state-1 Royal six-man cast at `$0F76`: `3,6,7,8,9,2`.
- Exact state→featured-character table at `$8E9A`: states 2–13 are Hogan, Warrior, Jake, Boss Man, Slaughter, Perfect, Earthquake, DiBiase, Hawk, Animal, Smash, Crush.
- Saturn runtime now advances the attract clock through those 14 states and changes to the correct ROM roster instead of the old fixed cast.
- ROM `$8CF8` presentation constants pinned:
  - TAG overlay: text `$29`, graphics block `$15`, palette bank 7.
  - Royal overlay: text `$2A`, graphics block `$16`, palette bank 6.
  - profile object anchor `$28,$A8`, initial phase timer 8, force-frame threshold `$0100`, removal threshold `$0200`.
  - profile text IDs are `$07..$12`; biography IDs are `$40..$4B`.
- Hawk/Animal exact palette banks 10/11 recovered from the same 16-bank ROM palette table at `$2F22`.
- New sparse ROM-derived `C10.PAK` and `C11.PAK` provide exact Hawk/Animal common attract locomotion/strike/recovery compositions without fabricated pixels.
- `ROSTER_COUNT` expanded to 12 and the Saturn roster loader can load C10/C11.

## PROVISIONAL in WIP-A

- Attract AI remains native/provisional. Extended Hawk/Animal states are deliberately fenced to common locomotion/strike actions so IDs 10/11 do not enter old 10-entry finisher/ground registries before those systems are extended.
- The original portrait pixels/profile object renderer are not yet integrated on Saturn; only their exact ROM control constants are pinned.
- The exact TAG/Royal overlay graphics are identified but not yet rendered from original tile data.

## REGISTERED BUT NOT IMPLEMENTED after WIP-A

- Exact mapping of the two Demolition/presenter speaker sequences, animation events and audio commands.
- Full TAG-records vs Royal-records presentation closure from the `$1B4E` path.
- ROM-exact profile portrait graphics/tile extraction and Saturn renderer.
- ROM-exact TAG/Royal overlay tile extraction/rendering.
- Full 12-character gameplay move/finisher/ground tables for Hawk/Animal; WIP-A only makes their attract presence safe and exact at the asset/palette level.

## Validation

New targeted tests pass for:

- ROM attract duration/roster/featured-character tables;
- native 14-state attract clock;
- Saturn runtime integration and extended-roster safety fence;
- Hawk/Animal pack/palette determinism against the supplied ROMs;
- `$8CF8` TAG/Royal/profile presentation constants.

A broad legacy host regression was also started and passed a long prefix of existing tests before the local command timeout; no failure in that run was observed before timeout. A separate old Test225 grep-style test is stale against the current Test254+ source shape and is not used as evidence for WIP-A.
