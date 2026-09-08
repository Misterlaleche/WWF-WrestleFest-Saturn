# WIP109 CHECKPOINT — neutral-entry held release blocker reduction

## Scope
Structural blocker reduction using the ROM-proven neutral Ground entry class (main-state0 or controller-generated state1). No live release enabled.

## Closed
Neutral entry proves:
- `+$42=0` before selection;
- actor `+$32.bit2=0` before selection.

Combined with exact held input profiles:
- raw `$0E` phases 0/1/2: target-Z rebase + neutral/immediate probe + actor32 clear => no remaining structural Z/probe/actor32 blocker;
- raw `$10`: target-Z rebase + immediate `-$20` probe + actor32 clear => no remaining structural blocker;
- raw `$13`: target-Z rebase + inherited-but-neutral-zero probe + actor32 clear => no remaining structural blocker;
- raw `$0B/$14/$23/$35/$46/$47`: only inherited Z remains blocking.

Added `arcade_prone_hold_release_blockers_neutral_entry()` and Test392.

## Safety
This does NOT call `arcade_prone_hold_live_release_ready()` from runtime and does not alias native Saturn states to the ROM-neutral provenance class. The existing 32-tick fallback remains.

## Regression
PASS Test330, Test334, Test335, Test392.

## Source archive
`WWF-Saturn-Test255-WIP109-source.zip`
SHA256 `4991d1b3ede2e188b4455ddf5b9bdd19c9dda717f37a9bfcbc36482760194f00`
