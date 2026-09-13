# WP12 R04 — corrected first-match visual pipeline

R04 supersedes the R03 animated/golden visual references.

R03 treated the actor Y word produced by `$247C` as final raster Y. The exact
arcade path is:

`$247C actor source -> $D1FC metasprite builder -> sprite RAM -> WP26 decode`

WP26: `raster_y=((256-raw_y)&0x1FF)-16`.

Regression: source Y `$0030` -> raster 192; update320 source Y -112 -> raster
352. Wrestlers therefore exit through the bottom, not the top.

No gameplay, CPU/AI, collision, audio backend, private RNG, `main.c` or BIN/CUE.
