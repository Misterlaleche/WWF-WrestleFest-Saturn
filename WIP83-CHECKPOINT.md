# WIP83 — unique ground victim state4 producer mirroring + directed host-test repair

## Runtime
- The existing native ground impact event (`tick==12`) remains explicitly **PROVISIONAL** and is not promoted to ROM-EXACT timing.
- At that already-existing impact boundary, the victim now mirrors a state-4 `+$64/+65` producer only when the raw action has exactly one ROM-demonstrated A2 producer.
- Reachable Ground Actions affected by current exact registry:
  - raw `$13` -> ROM `$14A90` -> WORD `$0007` -> victim low subtype `$07`.
  - raw `$14` -> ROM `$14C52` -> WORD `$0007` -> victim low subtype `$07`.
  - raw `$35` -> ROM `$17858` -> WORD `$001A` -> victim low subtype `$1A`.
- Ambiguous raw `$23` remains unwired because it has two exact producers (`$15F70->$07`, `$1601A->$09`).
- Raws with no direct state-4 producer remain untouched.
- This allows later prone selection to consume exact category 6/7 for `$07` when the WIP82 validity guard remains true, without inventing a producer value.

## Tests
- Added Test369: verifies raw `$13/$14/$35` unique producer mirroring and that ambiguous raw `$23` is not guessed.
- Repaired `tools/check_host.py` so WIP58+ registrations are placed before execution, selected test names can be run without triggering unrelated legacy failures, and one-file Python tests execute with the Python interpreter instead of being sent to gcc.
- Directed Tests358–369 PASS.
- `python3 -m py_compile tools/check_host.py` PASS.

## Persistence
- Library source: `/WWF-Saturn/WWF-Saturn-Test255-WIP83-source.zip`
- SHA256: `cace7b541825a8a806e9e469cb4aca6d18f3ba648b0f96b6ec72f40b4745f754`

## Fidelity boundaries
- ROM-EXACT: producer addresses, write widths, target role A2, and produced `+$64/+65` values.
- PROVISIONAL: the native `tick==12` event used as the current integration boundary.
- REGISTERED BUT NOT IMPLEMENTED: branch-dependent raw `$23` producer selection, live state-4 callback timing, held physical release, and contact-driven damage scheduling.
- No held fallback removed. No `tick==12` replacement. No BIN/CUE. No ROM files.
