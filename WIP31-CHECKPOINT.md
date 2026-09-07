# Test255 WIP31 — prone contact probe scheduling closure

Status: source/reverse-engineering checkpoint only; no BIN/CUE generated.

## ROM-EXACT closed

- Test300 pins the live fighter update order at `$F518`: clear collision response -> call `$280DC` -> apply `+$38/+$3A/+$3C` corrections to `+$06/+$0A/+$0E` -> call `$1C03E` action dispatcher/callback.
- Therefore a prone callback write to `+$42` is a contact probe for the NEXT resolver tick; it is not a synthetic duration or continuously integrated Saturn timer.
- Test301 pins the explicit prone callback probe literals and post-contact clears:
  - raw `$0E` later held phase: `+$42=$FFE8` (-24), then test `+$37 bit4`, then clear `+$42`.
  - raw `$10` phase 1: `+$42=$FFE0` (-32), then contact test, then clear.
  - raw `$14` phase 3: `+$42=$0020` (+32), then contact test, then clear.
  - raw `$23` later phase: `+$42=$0020`, then contact test, then clear.
  - raw `$35`: `+$42=$0020`, then contact test, then clear.
  - shared raw `$46/$47` callback family programs `+$42=$0020` in its preparatory phase and clears it on the contact-resolved branch.
- Combined WIP28..WIP31 chain is now: descriptor `$FF00` -> callback phase `+$25` -> callback programs/retains exact `+$42` probe -> next fighter tick `$280DC` computes candidate `+$0E + +$42` -> `$28124/$28154` producer sets `+$37 bit4` on contact -> callback consumes bit4 and clears probe/advances terminal phase.

## Still open before live Saturn bridge

- Prove the effective `$1C007E` mode during live combat/prone updates. Do not assume an indirect producer exists: boot/reset clears the global RAM range and direct immediate mode writes remain only 3/4; combat may instead run from a cleared/default mode. This must be established from the actual game-mode transition path.
- Once the combat mode is pinned, map the exact selected `$28154` producer and its threshold for fighters with/without `+$32 bit2`.
- Only after that remove `ground_attack_visual_duration()`'s legacy 32-tick callback-held fallback.

## Validation

PASS: Test294..Test301 directed ROM/host chain.
