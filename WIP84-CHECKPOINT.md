# WIP84 — exact state-4 descriptor clock API

## Exact animation integration
- Exposed the already-decoded ROM state-4 subtype descriptor table `$00..$2A` through `arcade_animation`.
- Added:
  - `arcade_state4_animation(subtype)`
  - `arcade_state4_descriptor_address(subtype)`
  - `arcade_state4_callback_address(subtype)`
  - `arcade_state4_clock_reset(...)`
  - `arcade_state4_clock_tick(..., release_hold)`
  - `arcade_state4_clock_pose(...)`
- These use the recovered `arcade_recovery_animations[]` index, which corresponds one-for-one to the ROM state-4 subtype dispatch table already pinned by Test354/WIP70.
- Callback-held `0xFF00` phases use the same explicit-release semantics as raw action clocks; no synthetic duration is introduced.

## Prone family detail
- Subtype `$06` descriptor `$1B268`, callback `$1B288`: first frame `FF00`, then exact finite 9-tick tail.
- Subtype `$07` descriptor `$1B278`, callback `$1B288`: first frame `FF00`, then exact finite 9-tick tail.
- Subtypes `$05/$08/$09` begin on callback-held frames as recovered from their exact descriptors.

## Tests
- Added Test370 validating descriptor/callback identity for `$05/$06/$07/$08/$09/$21/$2A`, hold behavior, and exact `$06/$07` tail length.
- Directed Tests366–370 PASS.

## Persistence
- Library source: `/WWF-Saturn/WWF-Saturn-Test255-WIP84-source.zip`
- SHA256: `465ffad223442b92e87c900b11c513b541bef171e4f2ff1424d502d2f5c366f6`

## Fidelity boundaries
- ROM-EXACT: descriptor/callback identities and descriptor clocks.
- REGISTERED BUT NOT IMPLEMENTED: live release source for state-4 held frames (`+$25/+37` callback predicates), and native victim clock ownership.
- No held Ground Action fallback removed. No `tick==12` replacement. No BIN/CUE. No ROM files.
