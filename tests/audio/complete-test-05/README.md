# AUDIO COMPLETE TEST 05

Purpose: hardware-diagnose long continuous music transport using the exact checkpoint renderer.

Music commands included:
`01,02,04,06,07,0A,0B,0E`

These were the commands reported as cutting early in TEST-04.

Each TEST-05 track is a continuous, non-looped YM2151+OKI diagnostic render. The first 12 seconds of all eight were revalidated against canonical A53 SHA256 values and PASS.

Lengths are approximately 51.79 to 60.00 seconds.

**These are diagnostic captures, not final production loop assets.**

Real Saturn + Terraonion MODE result reported by the user:
- playback clean;
- no skips;
- no premature cuts.

SFX/voice path was unchanged from TEST-04.

Controls used by the diagnostic build:
- L/R: category
- Left/Right: previous/next item
- A: play
- B: stop
- Up/Down: master volume

The ROM-derived PCM payload and BIN/CUE are intentionally not committed here. See `AUDIT-TEST05.md` and `LONGMUS-INDEX.json` for reproducibility metadata.
