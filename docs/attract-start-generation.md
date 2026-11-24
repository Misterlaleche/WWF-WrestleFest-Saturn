# Paid START after the natural attract title

The stock B image stopped after accepting C then START on the title reached
through the attract sequence. The final debugger display showed phase 3 / PC
`52BE`, but that was the last observation before failure. The actual retained
frontend driver was already FATAL (7), with dependency `83A4`, generation 1,
and the interrupted match cursor still at generation 0. `main` then called
`fail(0, 0)`: the old match cursor correctly had no dependency error of its own.
VBlank continued while foreground gameplay, video and audio stopped.

`wf_r18_frontend_driver_attract_visual_8490` holds a local title cursor. Accepted
START abandons that cursor and transfers to `52BE`; its synchronous wait returns
a blocked result to unwind the old generation. The loop interpreted the blocked
result as fatal **before** checking that generation had changed. The fix checks
generation first, matching `driver_step`, `wait_vblank`, and the driver owner
call contract. A genuine error without a generation change still becomes fatal.

The exact B ELF (`47e596cbefbd4dc25434d7442364ddbbdd7da31cf159af9a449bdf4145dc24e9`)
contains that order: title call `0603401E`, result test `06034022`, fatal branch
`0603402C`, then generation comparison `06034030..36`. The 36 relevant instruction
words were checked against the ELF bytes. This failure does not require an
outdated object, DMA deadlock or SMPC timing defect. Independent review also
checked the original arcade START continuation: `8F2` jumps to `52BE` rather than
resuming the abandoned title.

## Validation

Evidence is in `evidence/attract-start-20261113/`. The immutable stock B capture
remains in the surrounding workspace at
`work/r02-smpc-continuity-order/capture-stock-tag-attract`.

- Baseline from RESET: six real demo matches, then C held for 18 physical fields,
  release, and START at field offset 55. Both selected modes reproduce
  FATAL7 / PC52BE / dependency83A4 / generation1, with no credited match.
- Fixed source: the same physical buttons enter credited TAG and Royal and run
  600 real gameplay frames in each. Driver and match generations both equal 1.
- A deliberately failed title dependency without a generation change still
  reports FATAL7 / DEAD8490. Baseline and fixed retained RAM, exported arcade
  RAM, actors and normalized runtime state are byte-identical in this case.
- All nine existing frontend adapter regressions pass, including coin/START,
  dirty publication and TAG/Royal post-match routes.
- An independent reviewer reran both successful routes and the failure control.

The harness supplies console/time boundaries and physical buttons, sampled once
per field. It never writes original RAM, RNG, actors, credits or scene state.
The failure control alone substitutes a failing title dependency. This is host
control-flow evidence; it does not validate physical Saturn timing. A new target
image must still repeat the manual title-to-game route.

To reproduce on Linux with the normal local assets and native compiler:

```sh
python3 tools/build_runtime_tests.py --build-dir build/attract-start-runtime
python3 tests/run_attract_start_main.py \
  --runtime-build build/attract-start-runtime \
  --build-dir build/attract-start
```

`--assets /absolute/cd` selects a frozen asset directory. `--driver-source` builds
one alternate driver in isolation, replacing only its native object. All other
recorded runtime source/dependency and object hashes are checked before reuse.
The driver is always freshly compiled, and the test records its source identity.
For the red control, extract the driver from commit `f5b22d4`, pass that path as
`--driver-source`, and add `--expect-bug`. The committed evidence preserves the
original red and green identities; promoted verification records the final test
source after its portable path handling was installed.
