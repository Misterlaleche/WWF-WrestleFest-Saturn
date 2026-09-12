# WWF WrestleFest Saturn — WP12 ENTRANCES/PREMATCH R02 CHECKPOINT — PARTIAL

R02 is a source-only, fail-closed checkpoint reconstruction of the paid pre-match route.

Git branch: `entrances-prematch-r02`.
It supersedes R01 as implementation work.  R01/R3 are historical audit inputs only.

Authority order used:

1. `wwfwfest.zip` original arcade ROM.
2. WP12 FINAL, only where it agrees with ROM.
3. Referee/Announcer FINAL, Presentation FINAL, WP25 Graphics, WP26 Renderer.
4. Saturn/Yaul constraints.

R02 deliberately does **not** contain `main.c`, gameplay scheduler, collision, CPU/AI,
audio backend, synthetic camera math, guessed priorities, or BIN/CUE.

Important integration model:

- `$C98` common actor/video setup is emitted as an exact external call boundary.
- The caller executes its canonical equivalent and then calls
  `wp12_confirm_common_setup(..., true, true)`.  R02 validates the WP12-facing fields instead of
  fabricating unknown common-setup state.
- `$21B4` shared RNG is not reimplemented locally.  AAEA iterations that need RNG
  return `WP12_NEED_RNG_21B4` until the caller supplies the exact shared RNG nibble.
- One `wp12_visible_iteration()` returns an **ordered batch** of effects belonging to
  one ROM control-flow slice.  It never invents one VBlank between successive audio
  or teardown calls.

See `docs/TECHNICAL-REPORT.md`, `docs/DEPENDENCIES.md`, and `audit/SELF-AUDIT-R02.md`.
