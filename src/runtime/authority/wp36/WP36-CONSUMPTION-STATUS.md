# WP36 consumption status in this checkpoint

Authority pin consumed from the supplied handoff:

- `WP36-CHARACTER-SPRITE-COMPLETENESS-ROM-EXACT-AUDITED.zip`
- expected SHA-256 `cd9731cefe74aa1c6ca13c7d7cc181d5cf9864677116746321cf29d944899a80`
- classification: `WP36 ROM-side: ROM-EXACT CLOSED`
- active Saturn implementation crosscheck: `OPEN`
- final integrated VDP1 scene capacity: owned/validated by WP26
- global actor/world/raster positioning: WP35/WP26

This referee-owner checkpoint does not claim `ALL CHARACTER GRAPHICS CLOSED` or `SATURN PIXEL-PERFECT`.
The full WP36 inventory ZIP was not present in this active workspace, so the required all-reachable-frame ROM-vs-Saturn piece-by-piece matrix was not fabricated from the handoff summary.

Pinned mandatory future regressions from the handoff:

- Ultimate Warrior pose `0x0062`, record `0x402E0`: 23 pieces, bbox `-34,-98,64,112`, palette 01.
- Ultimate Warrior pose `0x0063`, record `0x402E6`: 23 pieces, bbox `-34,-98,64,112`, palette 01.
- Big Bossman gfx set 03, pose `0x0178`, partner gfx set 01: 86 source cells.
- Demolition Smash logical gfx 0A pose `0x005B`: ROM 20 cells; recheck current runtime rather than historical WIP.
- Hawk gfx 04 pose `0x0068`: ROM 23 source cells; do not omit fully transparent source cells before legal final-screen culling.

Current runtime draw/VDP1 plan storage has `WF_DRAW_PIECES_MAX=512`, so the demonstrated 86-cell single-composition limit does not by itself overflow that source-side storage. Exact composition parity is nevertheless still OPEN until the WP36 inventory is crosschecked against every reachable emitted frame.
