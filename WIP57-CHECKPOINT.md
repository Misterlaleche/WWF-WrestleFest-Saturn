# WIP57 — raw $10 actor32 state-2 source narrowing

Status: source/research checkpoint only. No BIN/CUE. No ROM files.

## ROM-EXACT additions
- Tests305/307/329 prove exactly four A0 `BSET #2,+$32` sites immediately coupled to a main-state 2 transition:
  - `$1C4CA`
  - `$1C52E`
  - `$1CED0`
  - `$1F354`
- Added an explicit raw-$10 actor32 candidate-source API exposing only those four sites.
- None is marked as the raw-$10 source because no caller/control-flow proof currently links any one of them to the raw-$10 prone attacker selection path.

## Safety
- raw `$10` remains blocked by `ARCADE_PRONE_RELEASE_BLOCK_ACTOR32`.
- No state-2 SET mutation is wired into live `game.c` from this checkpoint.
- No `release=true` activation.
- No 32-tick fallback removal.
- No `tick==12` replacement.

## Tests
- Directed Tests329..341 PASS.
- New Test341 verifies all four exact candidate addresses and prevents false promotion to a live source.

## Source
- Library: `/WWF-Saturn/WWF-Saturn-Test255-WIP57-source.zip`
- SHA256: `7ee134f220ea69cb64d086ca5215cfbfdb5206ca661d0ded27cbc211c34a0489`

## Next
Reconstruct caller/control-flow around `$1C4CA/$1C52E/$1CED0/$1F354` and identify whether one of these is reached on the selection-ID 4/9 prone-category-7 raw `$10` attacker path. Only an exact linkage may authorize the corresponding runtime SET.
