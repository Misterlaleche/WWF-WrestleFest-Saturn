# WIP88 — PICK UP tick18 boundary audit; unsafe migration rejected

## Audit
- Raw `$22` descriptor `$15B6C` is fully finite (98 ticks) and already owns attacker pose/termination.
- Its first frame has duration `$10`, which under the recovered inclusive clock means:
  - updates 1..16 decrement frame0 remaining to zero;
  - update 17 enters frame1 with remaining `$10`;
  - update 18 is already the second tick of frame1 (`remaining=$0F`).
- Therefore the current provisional victim handoff at native `tick==18` is **not** equivalent to an exact descriptor frame boundary.

## Decision
- Rejected the attempted replacement of `tick==18` with `prone_clock.frame==1` because it would move the handoff one tick early and falsely claim exactness.
- The current tick18 handoff remains isolated and PROVISIONAL until raw `$22` callback `$15B8C` field-write timing is recovered.

## Tests
- Test352 continues to require the isolated provisional tick18 handoff.
- Added Test376 locking the exact descriptor timing and proving tick18 is not a frame boundary.
- Tests352/376 PASS.

## Persistence
- Library source: `/WWF-Saturn/WWF-Saturn-Test255-WIP88-source.zip`
- SHA256: `4541892843a18c906d1885411ffc5794efba863960694fdbd0b6e7390f67f0c8`

## Fidelity boundaries
- ROM-EXACT: raw22 descriptor timing.
- PROVISIONAL: victim handoff tick18.
- No runtime behavior changed after the audit; the unsafe migration was reverted before checkpoint.
- No BIN/CUE. No ROM files.
