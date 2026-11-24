# Initial-load host regressions

Run from the repository with the packaged assets available:

```sh
python3 tests/run_initial_load_tests.py --source . --assets cd --output build/initial-load-tests
```

The suite uses Python, a C/C++ compiler and ASan/UBSan. It does not build SH2,
launch an emulator, modify assets or use another work tree. To also execute the
97 combat audio traces against `ROM68K.BIN`, use a Python environment containing
`machine68k==0.5.1` and add `--rom-oracle`. The JSON report records commands,
logs, source hashes and source drift; a failed gate returns a nonzero status.

The tests cover:

- `test_raw_disc_metadata.py`: actual SDK lookup/open/read functions and the
  actual asset reader, with only sector delivery simulated. Six size queries
  read zero CD bytes when opted in and preserve the old 135,168-byte path when
  opted out. Missing/unmounted/invalid entries, JoLZ fallback, offset reads and
  the existing eleven-attempt retry contract are exercised. Two local source
  mutants must fail. `--baseline --source OLD_TREE` can reproduce the original
  zero-read failure; no old runtime is bundled.
- `test_raw_disc_packaging.py`: the real package verifier and make prerequisite,
  rejecting JoLZ, changed hashes, invalid formats and absent files. A valid
  legacy 8 KiB sprite pack is generated locally for the compatibility case.
- `test_music_headers_{api,generation,order}.py`: the real preparation/parser
  functions against the frozen pre-change reference in `fixtures/music_headers`,
  the real generation/build gate, and the real adapter initialization loop.
  Cases include short and malformed headers, unchanged output on error, zero
  reads for all 18 prepared cues, reordered/invalid IDs and stale WAV/table data.
- `test_cd_{diagnostics,loading_ui,diagnostic_mutants}.py`: actual SDK/main
  bodies with a controlled MMIO boundary, compared with the recorded original
  protocol behavior. These check counters, failure retention, quiet polling,
  UI ownership, and five rejected mutations. See the fixture's README.
- Existing audio service/PCM, combat, CD protocol, music stream/deadline,
  loading observer and frontend-fault tests. The audio stubs keep their old
  simulated results, but now validate the new API's name/header/size arguments
  against real WAV bytes. `test_audio.py --assets cd --sanitize` works without
  a ROM archive; its independent decoder and fixed PCM hash remain the oracle.
- A native `main` syntax/ABI check using the real diagnostic type. Native
  gameplay has no physical CD driver, so its hardware shim reports zero CD
  activity. This check is not a gameplay replay or a target timing test.

Individual scripts accept the source/assets/output arguments shown by
`--help` (the CD diagnostic scripts use `--source-dir` and `--build-dir`).

These are host contract tests. Zero-read metadata calls assume an already
mounted directory; an unmounted index returns failure without mounting it.
Header identity requires the mandatory full-WAV build/package gate: runtime
size checking alone cannot detect same-size content substitution. CD time
counters measure VBlank fields and can undercount waits with masked IRQs.
Nothing in this suite proves physical load duration or diagnoses the reported
six-minute R01 load.
