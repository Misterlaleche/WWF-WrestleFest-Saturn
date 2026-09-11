# Audio routing

No ROM-proven sound producer exists inside the owned FBI/legal/Technos pre-title route, so INTRO emits no command and contains no direct SCSP or WP24 backend logic.

Nearest proven later producer:
- `$00095E JSR $000081BA` (TITLE)
- `$00096A MOVE.W #$3101,D0`
- `$00096E JSR $00002052` => command `0x01`

That command occurs after TITLE returns and remains outside INTRO ownership / BLOCKED_BY_RUNTIME_OWNER. Do not move it earlier or infer that it belongs to a boot plate. `0x1F` is not synthesized here.
