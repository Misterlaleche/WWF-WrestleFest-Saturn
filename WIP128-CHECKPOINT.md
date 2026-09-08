# WIP128 CHECKPOINT — state7 recovery preserves inherited Z into state0

## Scope
ROM-EXACT audit of the standard main-state7 recovery callback and its transition to state0, specifically for the WIP124 first-selectable Ground Z-normalization question. No live release/timing change and no BIN/CUE.

## State7 dispatcher
Generic state dispatcher `$1C03E` uses table `$11478` for ordinary main states. Main state7 maps to descriptor `$11E36`, callback `$11E42`.

## Exact callback behavior
On first state7 entry `$11E42`:
- clears actor `+$01` at `$11E4A`;
- clears several state flags/counters;
- does **not** write actor `+$0E`.

On terminal `+$25==$FE`:
- the normal recovery branch at `$11E90` writes actor main state0;
- no intervening write restores `+$01` to a nonzero value;
- no direct `+$0E` write occurs on the state7→state0 branch.

An alternate branch may return to state4 depending on `+$FE/+$1F/+$64` conditions; that is not collapsed into the normal state0 recovery path.

## Ground consequence
For the common state7→state0 recovery provenance:
- the first state0 actor pass before Ground becomes selectable has inherited `+$01==0`;
- `$F518` therefore skips `$280DC` on that pass;
- state0 callback itself also does not rewrite `+$0E` (WIP113).

Therefore state0 does **not** provide a universal floor-Z normalization after state7 recovery. Actor Z reaching the next selectable Ground action is inherited from the predecessor/recovery chain and must remain explicitly traced.

This is a negative but important closure: WIP124's `+$01==0` exception is demonstrably reachable in normal recovery and cannot be optimized away.

## ROM anchors
state table `$11478`, state7 descriptor `$11E36`, callback `$11E42`, `CLR.B +$01` `$11E4A`, terminal gate `$11E6A`, normal state0 write `$11E90`.

## Safety
No physical release enabled. No held fallback removed. No `tick==12` migration. No ROM bytes persisted.

Main program SHA1: `a726c46f350723ea73158bc78055d17d418b14e2`.
