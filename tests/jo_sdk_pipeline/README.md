# Actual SDK command and display lifetime gate

Run from the source ZIP root:

```sh
python3 tests/jo_sdk_pipeline/sdk_test.py --mode normal --frames 1200 --seed 12345
```

Use `--source /path/to/source` for another isolated source tree and `--output`
for its logs, extracted SDK bodies, compiled executable and JSON manifest.
The runner needs Python 3, a Linux C compiler with UBSan, `ucontext`, `mmap`,
MMIO emulation and non-PIE linking. It does not need Git, an SH-2 compiler,
ROM assets or a previous build directory. Portability here means execution
from the delivered source tree; this fixture is not an operating-system port.

The real SDK's sort, DMA, synchronization and presentation C bodies are taken
from the selected source tree. Only SH-2 assembly helpers, cache-through/MMIO
addresses and empty busy waits are replaced. Two cooperative CPU stacks and
delayed devices check pending/displayed texture bytes, map snapshots, stable
command order, repeat copies, abort ownership and frame tags.

The `overlap`, `drain` and `edges` modes require the overlap implementation.
`lost-frame` is an intentional negative control: a nonzero exit verifies that
the fixture rejects a lost required publication. `normal` remains available
without overlap. Keep each result's mode, source manifest and exit status.

The recovered C fixture and platform header match their pre-loss hashes:

- `sdk_device_test.c`: `eb3167cb1d5fc2ef6226166886cadf8ddac7469cb9a80612a90a0e25fcf3a9df`
- `wf_jo_video_test_platform.h`: `d5903c5615178c3ce72955e6c409c4036d186d02b52e2cc8c7e21de3a93cd4df`

The runner was recovered from literal text without a retained historical hash.
Its only additional packaging change searches two parents for the source root.
