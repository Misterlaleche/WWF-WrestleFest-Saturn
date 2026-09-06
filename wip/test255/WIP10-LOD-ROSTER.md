# Test255 WIP10 — Legion of Doom / 12-character roster foundation

## ROM-EXACT

- Hawk and Animal are the arcade actor/composition banks 10 and 11 in the current internal Saturn/attract order.
- New resident `C10.PAK` / `C11.PAK` were generated directly from the user-supplied program ROM and reconstructed 8 MiB sprite region; no substitute art was used.
- Hawk: 34 populated resident frames, 622 unique tiles, 55,279 bytes, SHA256 `e3e6a58ee0e7f80228f720c251ee69378ffc8c839d681f73a9bde31bb578d3c5`.
- Animal: 27 populated resident frames, 549 unique tiles, 48,137 bytes, SHA256 `6b2cab47740db5ae02f53679052e365e7d557cef2050811600eec49248ffa788`.
- Populated raw-pose families cover the already-understood resident mappings for idle/walk, strike, hit/down, recovery, rise and pickup poses.
- `C07.PAK` and `C09.PAK` were normalized from 221 to the common 223-frame contract by appending empty frame directory slots only. Existing texture/record payloads remain unchanged.
- Saturn resident roster contract expanded from 10 to 12 banks; loader/runtime bounds updated so attract states containing IDs 10/11 no longer fail solely because those IDs are above 9.

## PROVISIONAL

- Hawk/Animal full move selection is not yet claimed ROM-exact. Existing generic move logic remains a temporary bridge wherever a dedicated LOD rule is not yet recovered.

## REGISTERED BUT NOT IMPLEMENTED

- Full Hawk/Animal grapple rule tables.
- LOD boss-energy/stamina behavior across modes.
- Doomsday Device exact setup, partner climb/jump, victim role, timing and failure behavior.
- Complete Fxx/Gxy/Sxy overlays involving IDs 10/11.
- Optional Saturn Special Edition selection of Hawk/Animal. Arcade mode must preserve their default boss/non-selectable status.

## Checkpoint

Full source snapshot persisted in Library as `/WWF-Saturn/WWF-Saturn-Test255-WIP10-source.zip`.
SHA256: `6d60cc4e2b2d0fdbaea5d1054d2f3c39434a1e64f3dcf57b8ed0446c713ea91e`.

Validation:
- `PASS Test256: ROM-exact Hawk/Animal resident actor banks C10/C11 cover common attract/gameplay poses`
- `PASS Test256: all 12 resident wrestler banks open with the same 223-frame Saturn roster contract`
