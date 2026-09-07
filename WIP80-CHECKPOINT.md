# WIP80 — grapple struggle completion from exact raw-$29 clock

- Replaced duplicated `grapple_action_tick > 60` completion with `arcade_animation_clock_t grapple_action_clock` driven by raw action `$29`.
- Starting a struggle action resets the exact raw-$29 descriptor clock; each active tick advances that clock; score increments only when the clock reports finished.
- `game_pair_struggle_raw_pose()` now samples the live exact action clock rather than re-sampling from a capped synthetic elapsed counter.
- The recovered two-completed-cycle terminal remains unchanged.
- CPU synthetic action-edge cadence and simultaneous-edge arbitration remain PROVISIONAL.
- Existing Test241/253 PASS; new Test365 PASS; `game.c` syntax PASS.
- Source ZIP SHA256: `2f9acb3a008b5e54d6c2d91b1258a15f861fa620db6086ad585c29a4b39f5bf3`.
- No Ground held release, no `tick==12` change, no BIN/CUE, no ROM files.
