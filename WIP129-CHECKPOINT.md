# WIP129 CHECKPOINT — state0/state1 `+$01` divergence at Ground entry

## Scope
ROM-EXACT scheduler/provenance closure for actor `+$01` at Ground selection from normal main-state0 versus controller-generated main-state1. Research/checkpoint only; no live release/timing change and no BIN/CUE.

## Closed
### Main-state0
State0 callback `$114B2` first-entry initialization explicitly executes:
- `$114BC CLR.B +$01(A0)`;
- `$114CC BCLR #2,+$32(A0)`;
- `$11500 CLR.W +$42(A0)`.

No later state0 callback path restores `+$01` before normal Ground selection.

Therefore an already-initialized main-state0 actor can select a Ground Action with exact:
- `+$01 == 0`;
- actor32 bit2 clear;
- probe42 zero.

When `$DCDE` changes that actor to the selected state5/raw action, the later same-frame `$F518` pass still sees inherited `+$01==0`, so `$280DC` is skipped before the action callback. State0 Ground entry therefore does **not** gain an automatic post-selection Z normalization pass.

### Controller-generated main-state1
Controller `$F218` enters state1 at `$F228`; the same family reasserts state1 at `$F2A2`.
A freshly written state1 cannot be Ground-selected until its actor callback has run because the write clears the main-state processed bit required by `$DCDE`.

State1 phase callback `$116C6` explicitly executes:
- `$116CE MOVE.B #$01,+$01(A0)` on first entry.

Thus by the first Ground-selectable controller-state1 input frame, `+$01==1`.
If Ground is selected there, the later same-frame `$F518` pass enters `$280DC` (subject to the separately audited bit5 bypass), unlike the state0 case.

## Consequence
The WIP124 `+$01==0` floor-normalization exception is not a generic neutral-entry property. It splits exactly by normal neutral provenance:
- state0 Ground entry: `+$01=0`, geometry skipped before first action callback;
- controller-state1 Ground entry: `+$01=1`, geometry enabled before first action callback.

Together with WIP127, primary-wrestler controller-state1 entry has no demonstrated bit5 SET blocker, leaving only the exact `Z+probe42 >= threshold` no-correction branch to audit for that provenance class.

State0 entry still requires predecessor Z provenance and cannot be promoted to floor-normalized.

## ROM anchors
`$F18A`, `$F1B6`, `$F218`, `$F228`, `$F2A2`, `$114B2`, `$114BC`, `$114CC`, `$11500`, state1 dispatcher `$11652`, state1 phase `$116C6`, `$116CE`, `$F4C2`, `$F518`, `$280DC`.

## Safety
No physical release enabled. No held fallback removed. No `tick==12` migration. No ROM bytes persisted.

Main program SHA1: `a726c46f350723ea73158bc78055d17d418b14e2`.
