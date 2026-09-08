# WIP164 CHECKPOINT — `$11192` target `+$61==$61` is exact raw-action `$61`

## Scope
ROM-EXACT interpretation of the remaining target byte tested by helper `$11192`. Research checkpoint only; no live held release, no fallback removal, no `tick==12` migration, no BIN/CUE.

## Common state5/raw-action dispatcher
Common dispatcher `$1C03E` proves that state-family byte `+$1D==$05` selects raw-action descriptor table `$12614`:

```text
CMPI.B #$05,+$1D(A0)
LEA     $12614,A1
MOVE.W  +$60(A0),D0
...
ANDI.W  #$00FF,D0
LSL.W   #2,D0
MOVEA.L (A1,D0.W),A1
```

Therefore the low byte `+$61` is the active raw-action index; high bits of word `+$60` are discarded before descriptor lookup.

Exact implication:

```text
BYTE +$61 == $61
```

means:

```text
current raw-action index == $61
```

It is not a separate semantic marker requiring an independent invented native field.

## Helper `$11192` corrected contract
Combining WIP163 with the dispatcher:

```text
C11192 =
    linked_target_main_state == 5
    AND linked_target_raw_action == $61
    AND profile10_overlap(A0,A1)
```

where profile10 overlap is the exact `$E958` 2D rectangle from WIP163.

## Raw `$61` descriptor
Descriptor table `$12614` entry `$61` points to:

```text
raw $61 descriptor = $194CA
callback           = $194D0
```

This independently confirms `$61` is a normal raw-action ID in the same table used by all state5 actions.

## Demonstrated direct producer
Raw `$37` entry points to descriptor `$17A40`, callback `$17A58`.
Inside that callback:

```text
$17A64  MOVE.W #$0005,+$20(A2)
$17A6A  MOVE.W #$0061,+$60(A2)
```

Thus raw `$37` directly places its linked A2 actor into:

```text
main state 5
raw action $61
```

which is exactly the state/raw pair required by `$11192` before profile10 overlap is evaluated.

This is a demonstrated producer, not a claim that it is the only possible producer of state5/raw61 through all table-driven transitions.

## Ground consequence
`C11192` no longer requires provenance for an opaque `+$61` byte. A future live helper implementation needs only:

- exact linked actor identity;
- exact target ROM-domain main-state validity;
- exact target current raw-action validity;
- exact X/Y/facing for WIP163 profile10 overlap.

A generic current-raw shadow is preferable to a dedicated `byte61` shadow because the ROM itself treats `+$61` as the low raw-action index.

## Validation
Local Test428 pins:
- state5 raw-action table selection at `$1C042/$1C04A/$1C08A`;
- raw `$61` descriptor `$194CA/callback $194D0`;
- raw `$37` descriptor `$17A40/callback $17A58`;
- direct linked target state5/raw61 producer `$17A64/$17A6A`;
- `$11192` state5 + low-byte `$61` tests.

## Safety
- No native-state resemblance used as proof.
- No claim that raw `$37` is the sole global producer.
- No physical held release enabled.
- No 32-tick fallback removed.
- `tick==12` unchanged.
- No ROM bytes persisted.
- No BIN/CUE generated.

Main program SHA1: `a726c46f350723ea73158bc78055d17d418b14e2`.
