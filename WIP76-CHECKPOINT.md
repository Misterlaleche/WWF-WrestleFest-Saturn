# WIP76 — map all direct state4 producers to exact raw actions

- Cross-linked the 51 direct state4 producer sites with the retained raw-action callback registry.
- `arcade_state4_initial_producer_t` now records exact `raw_action`.
- Added per-raw producer count/lookup helpers.
- This cleanly distinguishes unique producer raws from branch-dependent raws:
  - raw `$35`: one producer `$17858 -> $1A`.
  - raw `$44`: one producer `$186E6 -> $05`.
  - raw `$23`: two producers `$15F70 -> $07` and `$1601A -> $09`.
  - raw `$10`: no direct initial producer; its known role is downstream target retype.
- Test361 PASS; Test358 PASS; `game.c` syntax PASS.
- Source ZIP SHA256: `ae2d2b80508b02facc4b0a0470d773911bda455f004e521e47a0722e25dde4b9`.
- No held release, no `tick==12` change, no BIN/CUE, no ROM files.
