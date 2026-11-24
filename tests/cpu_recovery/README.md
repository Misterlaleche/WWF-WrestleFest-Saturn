# Reconstructed CPU regression gates

These tests compare the recovered implementation with unmodified source from
restored B12 commit `805483a`. The minimal transitive reference subset is
bundled in `tests/native/cpu_reference`; `common.py` pins its manifest and
checks every file before compilation. No Git repository is needed to run
these focused gates from an exported source tree.

From the repository root:

```sh
ASAN_OPTIONS=detect_leaks=0 python3 tests/cpu_recovery/run_helpers.py
ASAN_OPTIONS=detect_leaks=0 python3 tests/cpu_recovery/run_post7.py
python3 tests/cpu_recovery/codegen.py
```

The first two commands use the host `cc` with AddressSanitizer and
UndefinedBehaviorSanitizer. LeakSanitizer is disabled because this workspace
runs under ptrace; these tests do not allocate heap memory. The codegen gate
uses the bundled SH-2 GCC8.2 compiler, its `objcopy`, and host `size`. Outputs
are written under `build/cpu-recovery`.

`run_helpers.py` compares all valid actor byte/word/long offsets (including
odd offsets), surrounding canaries, all eight shadow registers plus invalid
indexes and null pointers, the external MOVE.W function address, and clock
advance/take state. It exhausts every canonical clock remainder across
elapsed0..26 and adds invalid remainders, field wrap, overflow and random
32-bit inputs. The B12 and candidate layouts are compiled independently.

`run_post7.py` namespaces the complete original B12 translation units. It
compares entire runtime bytes, queue entries/counts, status, dependencies and
callback observations for BB14 and7366. It exhausts the16-bit Royal timer and
covers actor presence, both scan passes, animation sentinel and invalid phases,
missing hooks, rejected first/second callbacks, invalid RNG, full queues and
visibility failures. Hooks mutate unrelated runtime/queue fields to test the
complete rollback. The register snapshot at the beginning of the scan must
survive rollback after a matching actor is found.

The permanent coin gates are separate:

```sh
python3 tests/test_coin_helpers.py --build-dir build/cpu-recovery/coin-helper-gate
WF_COIN_TEST_BUILD=build/cpu-recovery/coin-run-gate \
WF_COIN_TEST_CFLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer' \
ASAN_OPTIONS=detect_leaks=0 sh tests/run_coin_run_tests.sh
python3 tests/test_coin_irq_rom.py --entry both --report build/cpu-recovery/coin-irq-rom.json
python3 tests/check_coin_codegen.py --inline-helpers \
  --build-dir build/cpu-recovery/coin-codegen --output build/cpu-recovery/coin-codegen/report.json
```

The Python coin gates require `machine68k==0.5.1`; it is only an offline test
dependency. In the recovery workspace it was loaded through
`PYTHONPATH=/workspace/scratch/53b5fb345a0c/tools_local/python`. The complete IRQ
oracle also requires the unchanged `cd/ROM68K.BIN` with its pinned SHA-1.

For independently generated complete native traces:

```sh
python3 tests/cpu_recovery/compare_native.py REFERENCE_DIRECTORY CANDIDATE_DIRECTORY \
  --output build/cpu-recovery/native-comparison.json
```

Use the same trace fixtures/assets and enable `--trace` in
`tests/run_jo_main_oracle.py` for both runs. This strict CPU-only comparison
requires every snapshot, command/pixel digest, tile stream and asset counter
to match. A renderer or asset-cache change needs its own comparison policy;
this script intentionally reports any difference.
