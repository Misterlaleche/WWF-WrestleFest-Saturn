# WIP71 — close reachable prone repertoire execution surface

Status: Ground Actions repertoire coverage checkpoint. No BIN/CUE. No ROM files.

## Closed
- Exhaustively enumerated every raw result reachable from the exact prone categories `$05/$06/$07` for all 12 logical selections and all three physical button masks.
- The complete reachable raw set is bounded to:
  `$08,$09,$0A,$0B,$0E,$10,$13,$14,$22,$23,$35,$46,$47,$48`.
- Every non-special raw has an exact registered action descriptor.
- Any reachable descriptor with no finite duration is required to have a classified callback-held profile; no unknown zero-duration raw can leak into runtime.
- Raw `$22` remains the dedicated PICK UP path with descriptor `$15B6C`.
- Raw `$48` remains the dedicated pin/cover path.

## Meaning
The prone action table no longer has a REGISTERED-BUT-NOT-IMPLEMENTED selection hole at its dispatch surface: every exact category 5/6/7 result has a bounded native execution class. This does not mean every held callback is timing-complete; unresolved physical releases remain deliberately blocked.

## Safety
- No held release enabled.
- No 32-tick held fallback removed.
- No `tick==12` damage timing changed.
- No PICK UP victim handoff timing changed.

## Tests
- Test356 PASS: exhaustive 12-wrestler x 3-category x 3-button surface classification.
- Tests351-355 remain directed PASS.

## Next
The remaining Ground Actions gap is now concentrated in callback fidelity rather than repertoire dispatch: inherited `+$0E/+$42`, held physical release, exact damage/contact side effects, and PICK UP victim handoff.

Source ZIP: `/WWF-Saturn/WWF-Saturn-Test255-WIP71-source.zip`
SHA256: `7f14808c91c51bb28768868155a61c3b1c7a2fde4aaf15afd40fd78c7857c050`
