# WP12 R02 handoff to Integration Master

1. Enter with the roster/mode/global state produced by the validated frontend.
2. Call `wp12_begin_paid_route()` and execute the returned operations in order.
3. Execute the canonical common `$C98` equivalent.
4. Populate/update the R02 context with that canonical setup state and call
   `wp12_confirm_common_setup(..., true, true)`.  Validation failure is fatal/fail-closed.
5. For each synchronized pre-match iteration, call `wp12_visible_iteration()`.
   If it returns `WP12_NEED_RNG_21B4`, obtain the next value from the canonical
   shared RNG and retry the same iteration with `rng_21b4_valid=true`.
6. Execute all returned operations in order.  `WP12_OP_VIDEO_SYNC_2836` is the
   proven synchronization boundary; do not turn other adjacent operations into
   additional frames.
7. `WP12_OP_HANDOFF_F8C` is the only point at which gameplay may take control.

No R02 source file owns or replaces master `main.c`.
