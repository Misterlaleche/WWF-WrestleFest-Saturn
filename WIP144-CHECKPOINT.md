# WIP144 CHECKPOINT — raw `$0E` phase2 is a state-transition hold exit

## Scope
ROM-EXACT classification of raw `$0E` hold phase2 (`$13FF2`) after WIP143 established that held releases cannot be represented by one universal boolean. Research/checkpoint only; no live release/timing change and no BIN/CUE.

## Exact phase2 gate
The phase2 path first requires actor `+$25>=2`, then arms the exact floor probe:

```text
$13FEC  MOVE.W #$FFE8,+$42(A0)   ; -24
$13FF2  BTST #4,+$37(A0)
```

If current-turn contact37 is clear, the callback leaves the held path unchanged for that pass.

On contact37, the callback:

```text
JSR $1110E
CLR.W +$42(A0)
CLR.B +$01(A0)
...
JSR $11152
...
BSR $11412
```

and then selects among explicit actor/linked-target state outcomes.

## No local hold-counter release
Unlike raw `$0E` phases0/1, there is no immediate `CLR.W +$22(A0)` on this phase2 contact route, and no `MOVE.W #finite,+$22(A0)` countdown comparable to raw `$13`.

The local contact outcomes instead leave the current raw-action descriptor by changing actor state.

### Demonstrated state-transition outcomes
The contact machine includes these exact classes:

1. **paired state5 choreography** around `$14028`:
   - actor main state5;
   - linked target main state5;
   - linked target raw/state payload `$51`;
   - target pending damage `$000F`;
   - associated orientation/flags/effects.

2. **actor state4 subtype `$1E`** around `$140E4/$140EA` under the exact linked-target `+$21/$61` gate.

3. **another paired state5 choreography** around `$14100..$1417E` after the `$11192` path, again with target pending damage `$000F`.

4. **actor state4 subtype `$05`** around `$14184/$1418A`, with its own local side effects.

All demonstrated contact branches leave the phase2 hold by state transition rather than ordinary descriptor phase advance.

## Classification
Raw `$0E` held phases are therefore not homogeneous:

```text
phase0: HOLD_CLEAR_AND_ADVANCE   (successful branch can CLR.W +$22)
phase1: HOLD_CLEAR_AND_ADVANCE   (successful branch can CLR.W +$22)
phase2: HOLD_STATE_TRANSITION    (contact outcome changes state; no local +$22 release)
```

This is the exact distinction required by WIP143's proposed multi-class hold API.

## Runtime consequence
The current Saturn path must **not** call `arcade_action_clock_tick(..., release_hold=true)` merely because phase2 contact37 is consumed. Advancing to the next raw `$0E` descriptor frame would model the wrong ROM operation.

A future faithful phase2 integration must instead apply the proven state-transition outcome selected by its helper predicates, after the physical inputs (`+$0E/+$42`, actor32, current-turn contact37, and helper branch results) are represented.

## ROM anchors
`$13FD0..$141D6`, especially probe `$13FEC`, contact gate `$13FF2`, probe clear `$14002`, helper chain `$14018..$14024`, state5 route `$14028..`, state4 `$1E` route `$140D0..$140F2`, state5 route `$140F8..$1417E`, state4 `$05` route `$14184..$141CC`.

## Safety
No runtime field changed. No physical release enabled. No held fallback removed. No `tick==12` migration. No ROM bytes persisted.

Main program SHA1: `a726c46f350723ea73158bc78055d17d418b14e2`.
