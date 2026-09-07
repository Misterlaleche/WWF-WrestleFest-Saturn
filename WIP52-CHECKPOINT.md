# Test255 WIP52 — raw $10 actor32 provenance narrowed to pre-callback state

## ROM-EXACT / evidence-preserving closure
- The exhaustive actor `+$32 bit2` mutation inventory from Tests305/329 contains no mutation inside raw `$10` callback span `$141EC..$14320`.
- Therefore the bit2 value observed by raw `$10` at its held `$FF00` gate is inherited from pre-callback actor state; the callback itself neither produces nor clears it.
- The actual incoming bit2 value is still not inferred because the exact upstream raw-$10 entry producer path has not yet been pinned.
- Raw `$10` remains blocked only by `actor32 bit2`; its exact Z rebase and immediate `+$42=-$20` probe remain closed from prior checkpoints.

## Code/tests
- Added `arcade_prone_actor32_callback_policy()` with an exact profile only for raw `$10`.
- Test336 PASS: raw `$10` callback actor32 provenance is inherited and live release remains guarded.
- Directed regression Tests329–336 PASS.
- `tools/check_host.py` registers Test336 before the runner.

## Runtime status
- No gameplay timing changed.
- `contact37 -> release=true` remains disabled.
- 32-tick held fallback remains PROVISIONAL.
- Legacy `tick==12` remains PROVISIONAL and must not be removed before physical contact timing is safe.

## Next
- Trace the upstream producer/entry path that selects raw `$10` and determine the exact `+$32 bit2` value at callback entry.
- Only if that value is ROM-EXACT may raw `$10` become the first candidate for isolated live `$FF00` release.

## Source checkpoint
- Library: `/WWF-Saturn/WWF-Saturn-Test255-WIP52-source.zip`
- SHA256: `5e60c5f1c7fb04e9954e0c1f1ceba42e374f01d7f84f328004af0ac2e0cba9f9`

No ROM image included. No BIN/CUE generated.
