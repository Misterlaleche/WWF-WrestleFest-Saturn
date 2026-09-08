# WIP159 CHECKPOINT — post-hold Ground motion phase wiring

## Scope
Continue WIP158 by wiring the remaining demonstrated secondary `+$01/+$5C/+$5E` setup events that occur after a held descriptor phase has advanced. These are phase-entry callback events, not immediate contact side effects.

## ROM-EXACT phase mapping
The exact descriptor clocks establish:

```text
raw $0B: frame0 = FF00 hold -> frame1 finite; setup $13BE0/$13BE6/$13C2C
raw $23: frame0 = FF00 hold -> frame1 finite; setup $15E58/$15E5E/$15E7E
raw $46: frame3 = FF00 hold -> frame4 finite; setup $14688/$1468E/$146C2
raw $47: frame2 = FF00 hold -> frame3 finite; setup $14688/$1468E/$146C2
```

Added `arcade_prone_motion_writer_on_post_hold_frame()`.

The runtime mirrors these setups only on the first callback pass of the exact post-hold finite frame. Ordering remains:

```text
old motion integration
-> floor/contact37 resolver
-> callback phase writes secondary motion tuple
-> common descriptor clock update
```

Thus the new tuple affects physical Z only on the following actor pass.

## Safety
- No physical held release enabled.
- Contact does not directly trigger these secondary writers.
- No 32-tick fallback removed.
- No tick12 migration.
- Raw $10 remains branch-provenance blocked.
- No inherited Z/probe42 assumption added.
- No BIN/CUE and no ROM bytes persisted.

## Tests
- Test423 PASS — exact post-hold frame mapping and descriptor transition checks.
- Test424 PASS — runtime order guard: resolver before phase writer, writer before common clock update, existing held release blocker remains.
- Tests421/422 PASS regression.

## Next
Resolve raw `$10`'s two branch-local motion writer sites (`$14210` and `$1427C`) far enough to determine whether their provenance can be merged safely, then build a context-qualified held-release readiness contract combining actor32, Z, probe42 and motion validity without globally enabling release.
