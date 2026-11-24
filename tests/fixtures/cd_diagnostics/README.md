# Original SDK behavior oracle

`reference-behavior.json` records 18 runs of the original E02 SDK, before CD diagnostics. The runner extracts real command/start/read/refill function bodies; a W0C MMIO model and a byte-preserving DMA boundary provide the device. The receipt includes source hashes, the complete MMIO read/write hash, fields, service calls, retry spins, payload bytes consumed and final file position.

`tests/test_cd_diagnostics.py` compares the instrumented source to this oracle. It asserts counters independently, including eleven failed attempts/ten retries, recovery, timeout reasons, field wrap, retained maximum, retained last failure and snapshot purity. No retry algorithm is duplicated in the fixture. A test-only counter on FAD diagnostic assignments rejects stores on every empty poll.

From the repository root, using a host compiler with ASan/UBSan:

```
python3 tests/test_cd_diagnostics.py
python3 tests/test_cd_loading_ui.py
python3 tests/test_cd_diagnostic_mutants.py
```

The first two accept `--source-dir` and `--build-dir`. Mutants additionally accepts `--input-dir` pointing to the first runner's output. To reproduce a red counter run, pass an explicit original source tree with `--baseline --source-dir PATH`; normal tests require no sibling experiment directories.

These checks do not measure physical CD-R loading or SH2 performance. The UI test executes the actual copied loading/failure functions against a text recorder and checks that gameplay's VDP2 ownership prevents loading output. SDK diagnostic counters remain unchanged by display and snapshot operations.
