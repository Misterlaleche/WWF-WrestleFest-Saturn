# Test255 WIP28 — prone callback-held phase closure

Status: source/reverse-engineering checkpoint only; no BIN/CUE generated.

## ROM-EXACT closed in this checkpoint

- Test294 classifies every reachable prone action whose animation descriptor contains `$FF00`:
  - raw `$0B` -> desc `$13A1E`, cb `$13A2E`, first hold frame 0, one hold, terminal phase `$FE`, terminal actor state 7.
  - raw `$0E` -> desc `$13D26`, cb `$13D46`, first hold frame 3, three consecutive holds; terminal branch remains multi-phase and is not collapsed to a timer.
  - raw `$10` -> desc `$141D8`, cb `$141EC`, first hold frame 1, one hold, terminal phase `$FE`, terminal actor state 7.
  - raw `$13` -> desc `$149E2`, cb `$149F2`, first hold frame 1, one hold, terminal phase `$FE`, terminal actor state 7.
  - raw `$14` -> desc `$14B48`, cb `$14B64`, first hold frame 3, one hold, terminal phase `$FE`, terminal actor state 7.
  - raw `$23` -> desc `$15CE0`, cb `$15D08`, first hold frame 0, one hold, terminal phase `$FE`, terminal actor state 5.
  - raw `$35` -> desc `$175A8`, cb `$175C4`, first hold frame 3, one hold, terminal phase `$FE`, terminal actor state 5.
  - raw `$46` -> desc `$14424`, cb `$14458`, first hold frame 3, one hold, terminal phase `$FE`, terminal actor state 7.
  - raw `$47` -> desc `$14440`, cb `$14458`, first hold frame 2, one hold, terminal phase `$FE`, terminal actor state 7.
- Test295 byte-pins `BTST #4,+$37(A0)` inside the callbacks for all nine reachable `$FF00` prone actions.
- Therefore `$FF00` is not permitted to be represented by a fixed native timeout. The release bridge must be driven by the native equivalent of the arcade contact/collision producer feeding `+$37 bit4` plus the callback's own phase predicates.

## Still open before ground is declared Arcade Perfect

- Recover/map the exact producer of actor `+$37 bit4` into the Saturn collision model.
- Reproduce the callback phase transitions for the multi-hold raw `$0E` without collapsing its three `$FF00` phases.
- Replace `ground_attack_visual_duration()`'s legacy fallback only after the above producer exists; do not freeze or auto-release gameplay as an intermediate shortcut.
- Initial victim `+$65` subtype production on every knockdown path remains to be mapped completely.

## Validation

PASS: Test294 exact prone held-profile audit.
PASS: Test295 exact `+$37 bit4` contact-gate audit.
Existing WIP27 Test292/Test293 evidence remains unchanged.
