# Original roster selector implementation plan

**Goal:** Port original SELECT PLAYERS owners into an isolated raw-RAM module with differential ROM evidence.

**Architecture:** Preserve the complete 8 KiB RAM window and D0-D7 register widths. Read original ROM tables through a bounded callback; expose exact external owner effects rather than inventing rendering or CPU team policy. Reuse the existing frontend input normalizer.

**Scope:** New `include/wf_r18_roster_select.h`, `src/wf_r18_roster_select.c`, and `tests/r18/roster_select_*` only. No entry, Makefiles, other owners, canonical assets or ROM modifications.

- [x] Write and observe a failing ROM-differential test for page scroll `$658C`, including direction, target clear and wrap boundaries.
- [x] Implement `$658C` with exact `$26A42` callback effect and D-register contract; compare full RAM and effects.
- [x] Write and observe failing navigation/reconciliation tests, then port `$600E` and its original table consumers with occupancy behavior.
- [x] Extend initialization and the complete live selector through `$5EA0` -> `$AC0`, including joins/assignment/credit, activation, automatic retries and confirmation palette writes; test complete trajectories against intact ROM.
- [x] Strict host and SH-2 GCC object compile, exact-ROM comparisons, API/evidence/limits. No general Windows make is installed; parent owns the project regression/build integration. Sanitizer result is reported separately if run.

Review focus: occupied cursor destinations, mixed TAG/Royal matrices, signed/byte wrap, callback register preservation, and direct ROM termination/reentry bounds. Do not invent cancel, START confirmation, timeouts, auto-selection RNG or opponent fill.
