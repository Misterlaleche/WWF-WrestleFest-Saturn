# WIP158 CHECKPOINT — live validity-tagged Ground motion shadows

## Scope
Continue from WIP157 by materializing exact actor `+$01/+$5C/+$5E` motion provenance in `fighter_t` and integrating only callback setup events whose pre-gate provenance is one-to-one demonstrated. No physical held release is enabled.

## ROM-EXACT runtime model
`fighter_t` now carries:
- `actor01_shadow` — actor motion dispatcher byte `+$01`;
- `velocity5c_shadow` — signed 8.8 vertical velocity `+$5C`;
- `gravity5e_shadow` — gravity word `+$5E`;
- `prone_motion_shadow_valid` — epistemic validity bit;
- `prone_motion_last_rom_event` — exact setup commit address used to prevent repeated one-shot writes.

Ground-action entry invalidates only provenance, not stored values. This deliberately refuses to treat native state entry as proof of inherited ROM motion fields.

## Exact ordering
The live Ground pass now preserves the demonstrated actor ordering:

```text
existing +$01/+$5C/+$5E motion integration
-> floor/contact37 resolver
-> action callback / held gate
-> callback may write a new motion tuple
```

Therefore a callback setup mirrored in the current pass begins affecting Z on the next pass. `raw_z_fp_shadow` remains the full 16.16 accumulator from WIP153 and is synchronized back to the legacy high-word mirror after each exact vertical step.

## Unique pre-gate writer policy
Added:
- `arcade_prone_motion_writer_before_hold_gate()`
- `arcade_prone_motion_writer_commit_address()`

Only one-to-one pre-gate callback setups are exposed live:

```text
raw $0E phase0 -> $13D8A/$13D90/$13DDE
raw $0E phase2 -> $13FB2/$13FB8/$13FD8
raw $13 phase0 -> $14A46/$14A4C/$149FC
raw $14 phase0 -> $14BA0/$14BA6/$14BEA
raw $23 phase0 -> $15DE2/$15DE8/$15DF4
raw $35 phase0 -> $177A4/$177AA/$177DA
raw $46/$47 phase0 -> $1446E/$14474/$144B8
```

Deliberate exclusions:
- raw `$0B`: its known setup is post-gate, not a pre-gate one-to-one event;
- raw `$10`: two branch-local writer sites remain provenance-ambiguous even though their literal values currently match;
- raw `$23` secondary and raw `$46/$47` secondary setups remain post-gate/unwired.

## Safety
- No held physical release enabled.
- `arcade_prone_hold_live_release_ready()` policy unchanged.
- No 32-tick fallback removed.
- No `tick==12` damage migration.
- No raw `$10` actor32 relaxation.
- No inherited Z/probe42 assumption added.
- No BIN/CUE generated.
- ROM used only temporarily for ROM-pinning tests and removed before packaging.

## Tests
Added:
- Test421 — unique pre-gate motion-writer provenance and deliberate raw0B/raw10 exclusions.
- Test422 — runtime ordering/validity guard: old motion integrates before resolver; unique callback setup mirrors once before callback gate; native entry invalidates provenance.

Directed regressions PASS:
Tests335, 349, 408, 409, 415, 416, 417, 418, 419, 420, 421, 422.

## Next
Close post-gate motion setup events for raw `$0B/$23/$46/$47`, then combine exact motion provenance with typed held-operation results and context-qualified actor32/Z/probe42 readiness. Physical release must still be enabled one raw/phase at a time only after every input is proven.
