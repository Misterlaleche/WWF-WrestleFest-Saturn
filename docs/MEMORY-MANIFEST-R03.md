# Memory Manifest R03

This checkpoint converts the WP32 assembler-handoff requirement into a machine-readable master manifest and fail-closed validator.

It does not invent unresolved values. The current candidate intentionally records these as blockers:
- final VDP1 command/texture partition;
- integrated ELF/link-map HWRAM end;
- scene HWRAM peaks;
- final CDFS root count;
- hardware transfer-window measurements.

Known WP26 VDP2/CRAM and WP24 SCSP partitions are represented exactly at the current integration authority.

Validation modes:
- structural mode: known maps and safety rules must pass while OPEN items remain reported;
- `--final`: every OPEN blocker becomes a hard failure.

The validator enforces:
- physical bounds;
- power-of-two alignment;
- simultaneous-lifetime overlap rejection;
- LWRAM SCU-DMA prohibition;
- graphics SCU-DMA source bounded to HWRAM;
- indirect-table alignment/end-marker requirements;
- CDFS capacity <= 384;
- final fail-closed gates.
