# Additional live-input routes

Physical-pad searches revealed paths that the checkpoint's two fixed tapes did
not visit. Fixtures below preserve real pre-call RAM/register state. Tests
execute the original68000 instructions without replacing helpers, compare the
complete16KiB RAM image and represented D registers, and retain negative cases.
They are local branch-fidelity evidence, not a claim that every possible input
or every remaining guarded branch has been implemented.

| Route | Cause and correction | Focused evidence |
|---|---|---|
| TAG CPU1DB30 | Tagged null grapple link reads ROM on the24-bit bus; reuse a const ROM view. |31 ROM cases; live replay crosses1816 and reaches normal loss3218. |
| TAG collision24A90 | Relation can point to either fixed ringside object, not only wrestlers. |Six ROM cases including complete scan, four invalid/rollback controls;97 existing sound traces also pass. |
| Royal CPU24CC | Missing state9 weighted40/60 branch and original D0=id prefix. |163 ROM cases covering both decisions and state5 handoff. |
| Royal raw49/135B0 | Released victim's null link writes ROM$9C; the bus ignores that store. |Five ROM cases, unsupported-pointer control; continuous route crosses9803. |
| Royal referee200EC | Released link's BSET targets ROM and has no writable destination. |Seven ROM cases with complete RAM, D registers, FG0 and sounds; live loss9060. |
| Royal prepass10BE8 | Null target reads reset-vector X0654/stateFF instead of an actor. |11 ROM cases plus329 existing facing cases; real path continues beyond7212. |
| Royal humanEA42/E2DC | Null target still participates in original action classification; state5 also has ordinary fallthroughs. |States0/1:168 ROM cases+five guards; state5:801 ROM cases+12 guards. |
| Royal raw4A/180EC | Null pin link follows the ROM's escape18184 after its bit tests. |16 ROM cases+three guards; raw49 regression remains green. |

The combined human/pin corrections allow physical trial020 to complete its
normal Royal defeat at tick12172, owner1AFC, with no decode failures, backing
reads or runtime fault. This is a defeat-path check, not a victory claim.

Known unsupported human branches remain explicit, including target0 with
specific relation/FLAGS34 conditions, raw15/16 classifiers and raw7B before
its RNG latch. Tests of these branches check the existing guard and its exact
prefix effects; they do not pretend the continuation is implemented.

An independent review found no new concrete defect in the non-CPU changes and
checked93 extra action-remap/flag/geometry cases against intact ROM, also with
full RAM/D-register equality. Evidence is in `build/review-fixes-20261113`.

The complete ordinary application routes were rerun together after these
changes and records integration, with the deferred asset worker enabled.
Royal ends at14409ticks and TAG at10397. All command/pixel rows and periodic
state snapshots match frozen C26; worker jobs are positive and balanced.
See `build/integrated-core-oracle` and `build/integrated-core-vs-c26.json`.

The autonomous policies and campaign harness remain test tools: they return
ordinary controller bits and observe game state. They never assign outcomes,
actor health, timers, RNG, stages or ring selection. Full credited victory and
Saturn cadence are separate ongoing gates.

## Addendum 2026-11-17 — P0-03

The historical raw7B-before-RNG-latch limitation above is superseded only for
$1129E mode0 reads whose physical target bus address is zero. Canonical ROM[$56]
is $FF; the native path now selects the original table and advances RNG instead
of blocking at $112C8. No actor or outcome is substituted. Raw15/16 classifier
and other relationship/FLAGS34 limitations remain open. See
`docs/p0-03-rng-20261117.md` and `tests/test_rng_p0_03_integration.py` for scope,
synthetic inputs, original-instruction comparisons and explicit oracle limits.
The optional machine68k legacy test was updated but not run in this environment.


## Addendum 2026-11-17 — P0-04

The raw15/raw16 guard described above is superseded within the scope of
`docs/p0-04-human-20261117.md`. $E1D4 uses the +$26 link, not necessarily the
primary +$7A target. Physical-zero +$7A no longer prevents that classifier;
physical-zero +$26 uses immutable ROM tier/filters. Tagged linked actor addresses
are resolved on the 24-bit bus without rewriting the original pointer.
The dynamic table, BCHG old bit, timer clearing and post-remap are retained.
Explicit invalid-input failures also remain failures with a NULL output trace.
Other idle target-zero relation/FLAGS34 branches remain OPEN. No claim of full
Royal/TAG completion, independent CPU verification or physical Saturn acceptance.


## Addendum 2026-11-17 — P0-05

The normal idle0/1 physical-zero target FLAGS34/REL_FLAGS limitations above
are superseded only within `docs/p0-05-relations-20261117.md`. E002 category11
retains priority over E2DC and E4BA; only the dynamic category toggles/writes
the link and permits its D3 remap. E4BA carry-clear retains held directions.
Nonzero invalid references still fail. The state1 prepass now preserves the
F24A..F252 word-sized D1 index prefix and its high word.

Final limited-ROM comparisons: 4447 direct cases per compiler, 4191 phase
cases with the instrumented 117-unit core; 11 human and 4 prepass compiled
mutations detected. These include explicit native-rejection controls, not
just legitimate arcade inputs. Other states, MASH/EBF6 and alternate E496
paths remain outside this closure. Global suite NOT PASS: 11 zero-exit groups,
8 missing-asset failures after the exact program ROM alone was restored.
No independent CPU, FIX2 integration, SH2 link or Saturn acceptance claim.
