# Overlap admission and display closure gates

These are host correctness tests for the opt-in overlap source. They are not
Saturn timing measurements. The actual SDK C scheduler/display test is in
`tests/jo_sdk_pipeline`; the two additional runners here test port lifecycle
and the application's actual main/frontend/clock functions.

## Port lifecycle and drain

```sh
python3 tests/overlap/port_test.py --source . --output build/overlap-port
```

This compiles production video/backend/memory source with ASan and UBSan. It
retains the ordinary pipeline suite, then models two CPU command buffers and
a delayed manual display. The added vector crosses tag `UINT32_MAX` to0,
aborts a command borrow, changes live palette/maps, rejects and retries the
service callback, probes reentry and requires a complete visible field. It
also verifies zero elapsed fields for a drain without an active epoch.

The base mock's ordering comparisons are made wrap-safe in the generated
fixture, and only the new drain vector uses borrowed commands and two buffers.
Every substitution is asserted and recorded; production source is not edited.
The fixture's adapted `close_frame()` still uses the real snapshot bank rather
than deriving a bank from the shared physical texture address.

## Actual main: clock, START and wait

Build native objects from the same source tree first. The paired host worker
option is supported and exercises private jobs, with no target SDK linked:

```sh
python3 tools/build_runtime_tests.py --build-dir build/overlap-native --worker
python3 tests/overlap/clock_test.py --source . --object-source . \
  --objects build/overlap-native --output build/overlap-wait --wait-close
python3 tests/overlap/clock_test.py --source . --object-source . \
  --objects build/overlap-native --output build/overlap-start-seam --attract --seam
python3 tests/overlap/clock_test.py --source . --object-source . \
  --objects build/overlap-native --output build/overlap-start-noepoch --attract --no-epoch
python3 tests/overlap/clock_test.py --source . --object-source . \
  --objects build/overlap-native --output build/overlap-start-zero --attract --start-zero --seam
```

The runner compiles actual main/frontend/clock and reuses the supplied core
objects. `--object-source` must name the unchanged source used for those
objects. It rejects a different core/backend/assets implementation, except the
ring integration which it recompiles. All supplied objects and source/header
inputs are hashed before and after execution. Linker wrapping replaces only
the three overlap video APIs in the hardware model, allowing objects from
C06 or from the current overlap tree without duplicate symbols.

Each run reaches native gameplay through the real frontend. It first admits
three frames in a single physical field and checks two ACKs, no new coin/IRQ3
events, and four unchanged callbacks after the debt is exhausted.

The wait vector executes48 cases: closure0/3 fields, six fractional phases,
field stride1..4 and wrap. Closure happens before the wait origin. The original
wait consumes65 complete arcade periods, preserves overshoot and pending debt,
and performs no coin IRQ, game tick or ACK. A later coin service consumes each
physical group once; repeated service in the same field does not resample.

The START vectors enter original attract, insert a real cabinet coin pulse,
then press START. They check that the drain precedes `$8AE`, that no IRQ2 runs
inside that IRQ3 path, and that only the drain's elapsed fields affect phase
and discarded debt. Existing scene transport remains excluded. A field
arriving just after the single anchor belongs to the next owner. Pending0 is
saturated correctly when calculating discarded debt.

The host fixture's `WF_JO_FOCAL_CLOCK_SHIM` guard selects this test's controlled
pause. The ordinary fullnative fixture instead asserts on an unexpected fatal
drain pause, rather than entering an unbounded hardware wait.

### Deliberate negative controls

The following must fail at the indicated runtime assertions after a successful
build. A compile failure or unrelated crash is not a valid control result.

```sh
# Must fail the closure/epoch assertion in original_wait_65.
python3 tests/overlap/clock_test.py --source . --object-source . \
  --objects build/overlap-native --output build/negative-wait \
  --wait-close --omit-wait-close
# Must fail the clock anchor/phase assertion in accepted_start.
python3 tests/overlap/clock_test.py --source . --object-source . \
  --objects build/overlap-native --output build/negative-anchor \
  --attract --seam --old-reanchor
# Must fail that assertion even with an inactive epoch.
python3 tests/overlap/clock_test.py --source . --object-source . \
  --objects build/overlap-native --output build/negative-transport \
  --attract --no-epoch --charge-transport
```

These controls alter only generated source under the requested output folder.
The JSON retains the actual child failure and signal; it does not relabel an
arbitrary nonzero exit as PASS.

## Actual SDK ordering and DMA

```sh
python3 tests/jo_sdk_pipeline/sdk_test.py --source . \
  --output build/sdk-normal --mode normal --frames 1200 --seed 1
python3 tests/jo_sdk_pipeline/sdk_test.py --source . \
  --output build/sdk-edges --mode edges --frames 1200 --seed 12345
```

See that fixture's README for its guarded coroutine stacks, delayed DMA,
presentation/tag assertions and deliberately lost-frame control. No game ROM
or BIOS is needed for the SDK/port tests; the actual-main gate uses the same
resident asset pack as the ordinary native oracle.
