# Test255 WIP50 — prone release blocker matrix / raw $10 priority

## Scope
Turn the remaining `$FF00` uncertainty into an explicit per-held-occurrence blocker matrix, without enabling live release.

## Evidence synthesis
Using the already ROM-audited WIP35 Z provenance, WIP37/WIP39 probe provenance, and WIP48 actor32 lifecycle boundary:
- every held occurrence still carries the unresolved actor `+$32 bit2` producer/lifecycle blocker;
- inherited-Z holds additionally carry a Z blocker;
- inherited-probe holds additionally carry a `+$42` blocker.

## Key result
- raw `$10` hold 0: exact target-Z rebase (`target Z + $50`) + exact immediate `+$42=-$20`; **only actor32 remains unresolved**.
- raw `$0E` hold 2 has the same single remaining blocker, but raw `$0E` as a whole is not release-ready because holds 0/1 still inherit `+$42`.
- raw `$13`: actor32 + inherited probe remain.
- raw `$14/$35`: actor32 + inherited Z remain.
- raw `$0B/$23/$46/$47`: actor32 + inherited Z + inherited probe remain.

## Code/tests
- Added `arcade_prone_hold_release_blockers(raw,phase)` with explicit blocker flags for inherited Z, inherited probe42 and actor32 lifecycle.
- Test334 PASS: exact blocker matrix and raw `$10` priority.
- Test333 remains PASS: secondary `$23/$46/$47` probes cannot be reused for their initial held gates.

## Runtime status
- No live `$FF00` release enabled.
- No 32-tick fallback removed.
- No `tick==12` damage path changed.

## Next target
Recover the exact actor `+$32 bit2` value/provenance at raw `$10` held entry. If that closes, raw `$10` becomes the first single-held prone action with all physical release inputs known and can be evaluated for a directed live-release migration.

## Fidelity
- **ROM-EXACT inputs:** raw `$10` Z rebase and immediate probe; all provenance facts used by the matrix.
- **REGISTERED BUT NOT IMPLEMENTED:** live release bridge.
- **PROVISIONAL:** 32-tick fallback and tick==12 damage scheduling.

Source checkpoint:
- Library: `/WWF-Saturn/WWF-Saturn-Test255-WIP50-source.zip`
- SHA256: `7d0f5738506c08a4a7139ea37535a8097dc2f7052db713c9e51ea33c17791d23`

No ROM image included. No BIN/CUE generated.
