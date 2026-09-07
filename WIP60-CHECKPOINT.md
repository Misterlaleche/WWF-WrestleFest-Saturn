# WIP60 — raw $10 actor32 history policy

Status: source/research checkpoint only. No BIN/CUE. No ROM files.

## Closure
- Raw `$10` selection IDs `4` and `9` are now explicitly classified as **inherited/conditional** for actor `+$32 bit2`.
- The Test70 character/action row selects raw `$10` from category 7/button 1 but does not mutate `+$32 bit2`.
- The exact immediate bit2 setters are controller/state-history transitions, not character-table constants.
- Therefore neither raw-$10 wrestler row may legally force bit2 SET or CLEAR.

## Consequence
- There is no single raw-$10 producer still waiting to be discovered and wired globally.
- Correct native behavior requires the per-actor `actor32_shadow` to preserve actual controller/state-history mutations before raw `$10` begins.
- This converts the remaining blocker from “identify one producer” to “finish lifecycle coverage for the controller transitions that can precede raw `$10`”.

## Runtime safety
- `ARCADE_PRONE_RELEASE_BLOCK_ACTOR32` remains active for raw `$10`.
- No live `release=true` activation.
- No 32-tick fallback removal.
- No `tick==12` replacement.

## Tests
- Added Test344.
- Directed Tests329..344 PASS; Python syntax PASS.

## Source checkpoint
- Library: `/WWF-Saturn/WWF-Saturn-Test255-WIP60-source.zip`
- SHA256: `6eac01d833b4cf744b9deeb6c4592150b377fa742b06016c66a5897e8192ee15`

## Next
Audit native runtime controller/state transitions against the ROM mutation inventory and wire only exact SET/CLEAR events that are already semantically matched. Once `actor32_shadow` is lifecycle-complete, raw `$10` can consume the shadow directly and its blocker can be removed without inventing a character-specific constant.
