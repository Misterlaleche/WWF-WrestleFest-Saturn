# Test255 WIP19 — Hawk / Animal individual action-registry checkpoint

Status: **WORK IN PROGRESS**. Test254 / V0.269 remains canonical; Test255 / V0.270 remains WIP.

## Source checkpoint

- Library: `/WWF-Saturn/WWF-Saturn-Test255-WIP19-source.zip`
- SHA256: `1ec21a23227d54d486328a40cfae68cce82399ef670e4163043582c553b616e7`
- Derived directly from the WIP18 live-Doomsday source line.
- No BIN/CUE generated.

## What is closed

- **ROM-EXACT:** all 126 Hawk/Animal contextual button decisions are pinned by Test274 (2 wrestlers × 21 raw categories × 3 fresh physical button combinations).
- **ROM-EXACT:** hidden logical/internal mapping remains Hawk 10 -> internal 4, Animal 11 -> internal 5.
- **ROM-EXACT:** Hawk stationary `$00` -> `$12BDA/$12C22`; Animal stationary `$00` -> `$12F08/$12F40`.
- **ROM-EXACT:** the already recovered descriptor/callback/finite-clock registry for the distinctive Hawk/Animal raw actions is pinned.
- **ROM-EXACT:** Animal K/raw `$89` remains restricted to the recovered linked contexts; there is still no synthetic `Animal holding -> Hawk auto-jump` shortcut.

The full table and evidence boundary are documented in `docs/TEST255-WIP19-LOD-INDIVIDUAL-REGISTRY.md` inside the Library source checkpoint.

## Validation

PASS: Test257, 259, 267, 270, 271, 272, 273, 274.

This confirms WIP19 did not regress the WIP18 Doomsday chain while freezing the hidden LOD individual action rows.

## PROVISIONAL

Native semantic mapping/damage for Hawk raw `$02/$2C/$2B/$28` and Animal raw `$2D/$2E/$30/$2F` remains provisional until callback/dataflow proof is repeated against the original program ROM. Guide-observed move names are not promoted to ROM-EXACT evidence.

## REGISTERED BUT NOT IMPLEMENTED

The above exclusive/context-distinctive raw actions have exact registered descriptor/callback addresses but are not yet promoted to complete individual native Saturn moves.

## Required next step

The source checkpoint intentionally contains no arcade ROM. `wwfwfest.zip` was searched for in the current conversation and Library and is not available. The user must re-upload the original `wwfwfest.zip` before the remaining Hawk/Animal byte-level callback, damage, link and move-identity closure can continue safely.

After that: Hawk individual closure -> Animal individual closure -> interactions against the other ten wrestlers -> full 12-wrestler roster closure.
