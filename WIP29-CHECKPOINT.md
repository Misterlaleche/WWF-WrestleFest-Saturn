# Test255 WIP29 — prone floor-contact producer closure

Status: source/reverse-engineering checkpoint only; no BIN/CUE generated.

## ROM-EXACT closed in this checkpoint

- Test296 recovers the shared physics producers that set actor raw `+$37 bit4` used by every reachable prone `$FF00` callback.
- The audited producer family is `$2818E/$28282/$2831C/$28516`.
- The demonstrated raw-Z floor thresholds are `$0140` for `$2818E/$2831C` and `$0100` for `$28282/$28516`.
- The producer performs the same style of raw-Z floor correction already recovered for the autonomous ringside resolver: when raw Z is below the active threshold it sets collision flag bit4 and writes the correction into `+$3C`.
- Test297 exposes the exact threshold predicate in host code. The predicate is spatial/physics-driven; no tick count participates.
- Combined with WIP28/Test295, this proves that reachable prone `$FF00` phases wait on a shared floor/contact event and must not be released by a fixed timeout.

## Still open before ground is declared Arcade Perfect

- Recover/integrate the wrestler raw-Z progression and the producer variant selected by each prone callback path.
- Feed that raw-Z floor event into the native Saturn ground-action callback phase machine.
- Reproduce raw `$0E`'s three consecutive callback-held phases individually.
- Remove `ground_attack_visual_duration()`'s legacy 32-tick fallback only after the raw-Z bridge is live and tested.
- Complete initial victim `+$65` subtype production mapping for all knockdown paths.

## Validation

PASS: Test294 exact held-profile audit.
PASS: Test295 exact callback `+$37 bit4` gates.
PASS: Test296 exact shared physics producers/thresholds.
PASS: Test297 exact raw-Z floor predicate.
