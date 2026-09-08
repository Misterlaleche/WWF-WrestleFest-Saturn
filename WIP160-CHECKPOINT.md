# WIP160 CHECKPOINT — corrected held-Ground motion provenance and callback first-entry semantics

## Authority correction
This checkpoint **supersedes WIP157, WIP158 and WIP159 motion writer/action-attribution statements** wherever they conflict with this file.

A ROM-exact callback ownership audit proved that three writer sites had previously been attributed to the wrong raw action by physical adjacency:

```text
$13BE0 belongs to raw $0C callback $13BC8, not raw $0B
$13FB2 belongs to raw $0F callback $13F90, not raw $0E
$14688 belongs to raw $11 callback $14670, not raw $46/$47
```

Those sites are removed from reachable held-Ground provenance and from live wiring.

## Correct reachable held-Ground motion inventory
Exactly 9 demonstrated writer sites remain:

```text
raw $0E: $13D8A/$13D90/$13DDE -> +$5C=$0600, +$5E=$0048, +$01=2
raw $10: $14210/$14216/$142A0 -> +$5C=$0300, +$5E=$0048, +$01=2
         $1427C/$14282/$142A0 -> same final tuple
raw $13: $14A46/$14A4C/$149FC -> +$5C=$0300, +$5E=$0030, +$01=2
raw $14: $14BA0/$14BA6/$14BEA -> +$5C=$0500, +$5E=$0048, +$01=2
raw $23: $15DE2/$15DE8/$15DF4 -> +$5C=$0500, +$5E=$0048, +$01=2
         $15E58/$15E5E/$15E7E -> +$5C=$0600, +$5E=$0048, +$01=2
raw $35: $177A4/$177AA/$177DA -> +$5C=$0600, +$5E=$0048, +$01=2
raw $46/$47: $1446E/$14474/$144B8 -> +$5C=$0600, +$5E=$0048, +$01=2
```

Raw `$0B` has no proven motion first-entry writer in this inventory.

## Callback first-entry semantics
ROM common updater `$1C0D4..$1C0E4` proves actor `+$1C.bit7` is the callback initialization latch:

```text
BTST #7,+$1C(A0)
if clear:
    CLR.W +$24
    load phase0
BSET #7,+$1C(A0)
```

Callbacks execute before the common updater. Therefore a held action's first callback pass sees bit7 clear, performs callback initialization, and must not consume/tick the normal held gate. The native runtime now models this with `prone_callback_initialized_shadow`.

On first held callback pass:
1. existing proven motion integrates;
2. floor/contact resolver runs;
3. exact callback first-entry motion tuple is applied when proven;
4. descriptor gate/tick is skipped;
5. next pass begins normal phase0 gate processing.

## Raw $10 branch merge
Raw `$10` first-entry has two mutually exclusive branch-local velocity/gravity writers:

```text
$14210: +$5C=$0300, +$5E=$0048
$1427C: +$5C=$0300, +$5E=$0048
```

Both converge at:

```text
$142A0: +$01=2
```

Therefore the final first-entry motion tuple is branch-independent even though the precise branch predicate remains unresolved. This is safe to expose as one ROM-EXACT first-entry profile.

## Raw $23 remains blocked
Raw `$23` dispatches internally through `+$44`; its two writer sites belong to different internal subcallbacks. They are deliberately **not** collapsed into a single first-entry profile until the `+$44` lifecycle is closed.

## Safety retained
- No held physical release enabled.
- No 32-tick fallback removed.
- No `tick==12` migration.
- No inherited Z/probe42 assumption added.
- Raw `$10` actor32 release blocker remains intact.
- No ROM bytes included in source package.
- No BIN/CUE generated.

## Tests
PASS:
- Test335
- Test349
- Tests415–425 relevant to Z32/action phase/motion provenance

New/corrected assertions:
- Test420: corrected 9-site reachable held inventory.
- Test421: exact first-entry profiles; raw10 merged; raw0B/raw23 remain unproven.
- Test422: runtime callback first-entry ordering.
- Test423: descriptor/callback ownership correction.
- Test424: ROM pin for common `+$1C.bit7` first-entry latch.
- Test425: ROM pin for raw10 equal converging branches.

## Source package
`WWF-Saturn-Test255-WIP160-source.zip`

SHA256:
`16c47d6b175b68b1597717266cd0e0943ab9d4df7825d6e4acb85b1bc663c770`

## Next
Close raw `$23` internal `+$44` substate lifecycle so the correct pre-held motion writer can be selected without guessing. Then build a context-qualified held-release readiness contract over actor32, Z, probe42, contact37, motion, action phase and actor25 validity without globally enabling release.
