# WIP89 — grapple struggle effort accumulation over exact raw-$29 clock

## Runtime
- Keeps ROM-EXACT raw `$29` grapple-struggle animation timing and the two completed-cycle terminal.
- Adds per-cycle `grapple_effort[2]` telemetry.
- P1 P/K input is counted throughout the active exact `$29` cycle instead of being ignored after cycle start.
- P+K remains one accepted struggle action, matching category-$08 action-$29 selection semantics.
- Existing deterministic CPU synthetic edge cadence is also accumulated during the active cycle.
- On exact action-clock completion, the side with greater accumulated effort wins that cycle; tie arbitration remains provisional.

## Fidelity boundary
- ROM-EXACT: category `$08` -> raw `$29`, exact descriptor clock, completion boundary, two completed-cycle terminal.
- PROVISIONAL: CPU edge cadence and tie arbitration.
- No change to Ground Actions held release, `tick==12`, PICK UP handoff, or BIN/CUE.

## Tests
- Test241/253 PASS.
- Test365 PASS.
- Test377 PASS.
- `python3 -m py_compile tools/check_host.py` PASS.

## Source
- `/WWF-Saturn/WWF-Saturn-Test255-WIP89-source.zip`
- SHA256 `a2d185d7873636831c7c4a91f827bec87b9eba70eb89c25984b397604659b1a9`

## Next
- Close grapple ownership / reversal / HOLD transition semantics and throw selection without regressing exact raw-$29 struggle timing.
