# WIP162 CHECKPOINT — context-qualified held-Ground readiness matrix after WIP161

## Scope
Consolidate WIP149–WIP161 into a per-raw/per-phase readiness contract for future physical held release. Research/contract checkpoint only: no runtime release enabled, no 32-tick fallback removed, no `tick==12` change, no BIN/CUE.

## Required validity dimensions
A held callback context may be considered physically evaluable only when the current actor pass has exact provenance for every dimension required by that raw/phase:

```text
callback_first_entry / +$1C.bit7 semantics
32-bit Z +$0E:+$10
probe +$42
actor32 event provenance (bit2; bit0 where classifier family requires it)
current-turn contact37 classifier/mode
motion +$01/+$5C/+$5E when the action owns a demonstrated tuple
descriptor +$22/+24 phase
actor +$25 / previous +$23 where callback tests it
helper carries $11152/$11192/$11412 where required
target state/subtype/profile/link inputs used by those helpers
raw-specific internal substate where applicable (raw23 +$44)
```

A known value without a demonstrated last-event/context provenance is not enough.

## Current per-action status

### raw `$0B`, hold0
Closed:
- contact operation is contact-only CLEAR_AND_ADVANCE;
- active held branch locally copies linked high Z at `$133C6`;
- locally arms probe `$13366 = -$20`;
- WIP106 closes contact37 formula.

Still blocking:
- no proven raw0B first-entry motion writer after WIP160 ownership correction;
- actor32/classifier provenance must be exact for the current context;
- full 16.16 Z fraction remains predecessor state even when high word is locally copied.

Verdict: **NOT LIVE-READY**.

### raw `$0E`
Closed:
- target high-Z rebase and full 16.16 preservation model;
- phase0 exact first-entry motion `$0600/$0048/2`;
- phase1 contact-only clear;
- phase0 helper predicate `contact & C11152 & !C11412`;
- phase2 is STATE_TRANSITION, not ordinary release;
- descriptor/first-entry semantics represented.

Still blocking:
- phase-qualified probe/actor32/contact/helper input provenance must be live-valid;
- phase1/2 motion continuity must come from demonstrated predecessor state, not synthetic raw-only initialization;
- phase2 branch state choreography remains context-dependent.

Verdict: **NOT LIVE-READY**.

### raw `$10`, hold0
Closed:
- exact target high-Z +$50 rebase;
- immediate probe -$20;
- WIP160 proves both branch-local first-entry motion setups converge to `$0300/$0048/2`;
- WIP151/152 provide exact actor32 provenance classes/last-event tracking;
- callback clear predicate `contact & (C11152 | C11192*)` is exact.

Still blocking:
- OTHER_STATE1 actor32 remains UNKNOWN without exact last-event history;
- helper/target-state/profile inputs must be represented for the current callback pass;
- classifier mode/bit0 context must be current-turn valid.

Verdict: **CONTEXT-CLOSEST, BUT NOT GLOBALLY LIVE-READY**.

### raw `$13`, hold0
Closed:
- target high-Z +$20 rebase;
- first-entry motion `$0300/$0030/2`;
- callback operation is helper-gated SET_FINITE(0018), not immediate release;
- exact descriptor/common-updater semantics.

Still blocking:
- inherited probe42 provenance;
- actor32/classifier provenance;
- helper `$11152` live inputs.

Verdict: **NOT LIVE-READY**.

### raw `$14`, hold0
Closed:
- first-entry motion `$0500/$0048/2`;
- immediate probe +$20;
- contact-only CLEAR_AND_ADVANCE;
- exact controller requirement that old `+$25==3` cannot be reduced to elapsed frames.

Still blocking:
- inherited full Z provenance;
- current actor25/previous-actor23 validity for the gate context;
- actor32/classifier provenance.

Verdict: **NOT LIVE-READY**.

### raw `$23`, initial held gate `$15DFE`
WIP161 closes a prior ambiguity:
- gate belongs only to internal `+$44=1..4` shared subcallback;
- exact first-entry motion is `$0500/$0048/2`;
- contact operation is contact-only CLEAR_AND_ADVANCE;
- the `$0600/$0048/2` tuple belongs to distinct `+$44=8` callback and later probe/contact sequence.

Still blocking:
- inherited full Z provenance;
- actor32/classifier provenance;
- probe state reaching the `+$44=1..4` initial gate must have exact predecessor/clear provenance.

Verdict: **NOT LIVE-READY, but `+$44`/motion ambiguity removed for the initial gate**.

### raw `$35`, hold0
Closed:
- first-entry motion `$0600/$0048/2`;
- immediate probe +$20;
- helper predicate `contact & C11152 & !C11412`.

Still blocking:
- inherited full Z provenance;
- actor32/classifier provenance;
- helper/target-state/link inputs live-valid.

Verdict: **NOT LIVE-READY**.

### raw `$46/$47`, initial hold
Closed:
- first-entry motion `$0600/$0048/2`;
- exact helper predicate `contact & ((C11152 & !C11412) | (!C11152 & C11192))`;
- WIP160 removes false secondary writer `$14688` from these raws.

Still blocking:
- inherited full Z provenance;
- inherited/preparatory probe context at the initial gate;
- actor32/classifier provenance;
- helper/target-state/link inputs live-valid.

Verdict: **NOT LIVE-READY**.

## Global conclusion
No held raw/phase is authorized for physical release yet. This is now a **context-specific** safety decision rather than the old blanket unknown-state guard.

The closest candidate is raw `$10` under exact actor32 provenance classes, because Z, immediate probe and first-entry motion are structurally closed. Its remaining blocker is the live materialization of callback helper/target/classifier inputs plus exact actor32 history for OTHER_STATE1 cases; therefore it must not be globally enabled.

Raw `$23` is materially improved by WIP161: the initial held gate no longer has an unresolved internal-motion branch.

## Safety
- `arcade_prone_hold_live_release_ready()` must remain conservative.
- No 32-tick held fallback may be removed from this contract alone.
- `tick==12` remains untouched.
- No ROM bytes included.
- No BIN/CUE generated.

## Next
Prioritize the remaining **inherited full-Z/probe provenance** for raw `$14/$23/$35/$46/$47` and live helper-input materialization for raw `$10/$13/$35/$46/$47`. Promote physical release only one raw/phase and one provenance context at a time after a dedicated test proves every required validity dimension.
