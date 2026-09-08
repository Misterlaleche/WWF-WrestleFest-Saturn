# WIP138 CHECKPOINT — bounded dynamic-Z state0 predecessor exceptions

## Scope
ROM-EXACT classification of the two state0-return callback families found to copy actor Z high-word dynamically from another actor before returning to neutral. Research/checkpoint only; no live release/timing change and no BIN/CUE.

## raw `$08` exception
State5/raw action table `$12614` maps index `$08` to descriptor `$1316C`, callback `$13180`.

On first entry:

```text
$13198 MOVE.W +$0E(A2),+$0E(A0)
```

so attacker high/integer Z is copied from its linked A2 actor. The callback later has a state0 fallback at `$131F4` when its linkage/state/subtype gate is not satisfied.

Therefore raw `$08` can return a primary actor to state0 with a high-word Z derived from the linked target rather than from a local constant.

By WIP134 the copy is word-width only: fractional `+$10(A0)` is **not** copied. The resulting 32-bit Z can therefore be a hybrid:

```text
high16(Z) = linked actor +$0E
low16(Z)  = attacker's previously retained +$10
```

## raw `$70` exception
State5/raw action table entry `$70` points to descriptor `$19E80`, callback `$19E90`.

Its first-entry path loads A3 from actor `+$76` and copies:

```text
X/Y/high-Z/facing from A3 -> A0
$19ECE MOVE.W +$0E(A3),+$0E(A0)
```

At terminal `+$25==$FE`, `$19EEA` writes actor main state0.

Raw `$70` itself is produced only by bounded controller/special routes, including `$F0F4`; the helper choosing relation `+$76` uses auxiliary actor records `$1C0F1C/$1C1028` (slots beyond the primary wrestler slots0..3). Another controller producer exists at `$1C7C6` before a state1/control handoff.

Again, only the high Z word is copied; fractional `+$10` remains actor-local.

## Consequence
The state0 starting-Z problem is not an unconstrained generic dynamic-write problem. The two demonstrated dynamic high-word copies immediately relevant to neutral-return auditing are identifiable callback classes:

- raw `$08` -> linked-target high-Z copy;
- raw `$70` -> special/auxiliary relation high-Z copy.

Both preserve the actor's own fractional low word. They must be modeled as explicit predecessor provenance classes rather than collapsed into a universal floor-Z assumption.

This checkpoint does not claim those are the only possible indirect position histories in all gameplay; it closes the two dynamic-copy exceptions discovered in the state0-return census used by the Ground provenance audit.

## ROM anchors
raw08 descriptor `$1316C`, callback `$13180`, Z copy `$13198`, state0 `$131F4`; raw70 descriptor `$19E80`, callback `$19E90`, relation load `$19EAC`, Z copy `$19ECE`, state0 `$19EEA`; producer/controller anchors `$F0F4`, `$F106..$F172`, `$1C7C6`.

## Safety
No runtime change. No physical release enabled. No held fallback removed. No `tick==12` migration. No ROM bytes persisted.

Main program SHA1: `a726c46f350723ea73158bc78055d17d418b14e2`.
