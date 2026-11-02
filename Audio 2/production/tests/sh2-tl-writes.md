# Exact TL-prefix and YM-write host gate

`test_sh2_tl_writes.c` compares four lifted TL setup prefixes and the existing
YM writer with the untouched original scalar `src/reference_driver/z80.c`.
Each operation executes six setup instructions and twelve writer instructions,
ending at `0F5F`, before `EX AF,AF'` and `RET`.

| Entry | Register value | Level RAM | Transfer | Total T states |
|---|---|---|---|---:|
| `0C09` | Channel at `C065` + `60` | `C08A` | CALL, return `0C16` | 139 |
| `0C16` | D + 8 | `C08C` | CALL, return `0C21` | 130 |
| `0C21` | D + 8 | `C08B` | CALL, return `0C2C` | 130 |
| `0C2C` | D + 8 | `C08D` | JP, no stack push | 123 |

Arithmetic is byte arithmetic. Every admitted operation increments the low
seven bits of R by 18 while retaining its high bit. The common admission
contract requires at least 18 instructions, the full time above, clear status
bit 7, and SP in `C402..C800` inclusive. Pending NMI, enabled pending IRQ,
EI delay and halt reject admission. Masked pending IRQ remains admissible.

## Reproduce RED and GREEN

From `Audio 2/production`, supply the private, unchanged 64 KiB sound program
and a new output directory outside the source package:

```bash
python3 tools/verify_sh2_tl_writes.py \
  --rom /path/to/private/SOUND.ROM \
  --out /tmp/audio2-tl-writes-review
```

The runner uses Python's standard library and an existing C compiler (`cc`,
or `--cc`). It verifies the complete ROM hash and the pinned original scalar
and pre-correction native source hashes. It compiles the scalar with renamed
symbols and compiles the test with warnings treated as errors. Existing output
directories are refused. No original assets, traces or generated arrays are
copied into the source package.

RED uses `tests/oracle/z80_native_before_scalar9.c`. Each of the four entry
points must return exit 2 with its exact `expected 18 original instructions,
admitted 0` diagnostic. A compilation failure or unrelated failure is rejected.
GREEN uses the current `z80-native.c` and must complete **793,344 cases**:
790,920 admissions and 2,424 rejections. `verification.json` retains every
command, output, return code, source/input hash and binary/object hash; success
also requires identical input hashes before and after the run.

## What the comparisons establish

The test compares the complete CPU, 2 KiB RAM and adapter state after each
operation and at entry to both C800/C801 write callbacks. Callback addresses,
data, order, exact cycle counts, R, PC, flags and stack contents are observed.
Only the owner pointer differs between engines and is normalized. This gives
1,581,840 callback snapshots per engine in the full differential run.

Coverage includes every source-byte/level-byte pair with two state patterns,
every principal/alternate flag-byte pair, all 1,023 valid SP values including
odd addresses and `C800`, cycle wrap, exact and larger budgets/time limits,
masked IRQ and all interrupt modes. Rejections test every smaller budget,
every insufficient time allowance, all busy status values, stack boundaries,
expired limits, interrupt/delay/halt states and adjacent entry addresses.
Rejections must preserve every CPU/RAM/adapter byte and emit no write.

The fixture keeps status read-only. Counts of status-bus reads are deliberately
excluded: the native adapter may obtain the same status directly from
`v.status`. The observer records writes and does not synthesize timers or an
external YM device. The test executable also supports `--oracle-only` and
`--rejects-only`, optionally restricted to one entry PC.

This gate establishes host state and admission equivalence for these bounded
segments. It does not establish SH2 code placement or execution deadlines,
native service/timer behavior, global audio fidelity, active-OKI-bank fidelity,
listening approval or Saturn hardware acceptance. Those retain their separate
validation requirements.
