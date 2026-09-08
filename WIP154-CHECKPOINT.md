# WIP154 CHECKPOINT — Ground +$22/+24 phase materialization

## Scope
Materialize the exact descriptor fields needed by callback-held Ground Actions after WIP153's 32-bit Z integration.

## ROM-EXACT closure
The common descriptor updater `$1C0C8..$1C126` proves:
- actor `+$22` is the active descriptor remaining counter (`$FF00` means callback-held);
- actor `+$24` is the descriptor phase index, incremented when finite `+$22` underflows.

`arcade_animation_clock_t.remaining/frame` already implements the same inclusive counter/phase semantics established by WIP145. WIP154 exposes `arcade_action_clock_phase24()` as an exact active-descriptor view:
- `actor22_remaining = clock.remaining`;
- `actor24_phase = clock.frame`.

Finished descriptors deliberately return invalid rather than fabricating terminal actor fields.

Actor `+$25` is NOT derived here; WIP104/Test387 proved it is separate callback/table phase state.

## Safety
- No live physical held release enabled.
- No 32-tick fallback removed.
- No tick12 migration.
- No contact37 or helper result synthesized.
- No BIN/CUE and no ROM bytes persisted.

## Tests
Test408, Test409, Test415, Test416, Test417 pass with user-supplied ROM available only temporarily for ROM pinning.

## Source archive
`WWF-Saturn-Test255-WIP154-source.zip`
SHA256 `da2a752fe1563a5c83e32fac733e37fb40c3bdfdf3ed7ccc6950bf4018ffab66`
