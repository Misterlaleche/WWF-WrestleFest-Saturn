# Mandatory card presentation `$7B70`

`wf_r18_card_scene_init/step/complete` ports the complete original `$7B70`
controller, local `$7F0E` actor program and `$7E86` control graphic. It uses
the shared raw 16 KiB RAM and live D0-D7/A0-A6 context. Start at `$7B70`, or
use standalone `$7F0E/$7E86` entries for their original helper boundaries.
TAG returns before RTS `$7E14`; Royal returns before `$7E84`. Helper returns
are `$8134/$7F0C`. This owner precedes `$C98` participant generation and
therefore preserves the identities already present in RAM.

Requests contain `kind, owner, call_pc, return_pc, address, value`. CALL0
requires the complete actual dependency owner, then `complete(...,WF_OK)`.
It is never a request to skip a helper. WRITE8=2 commits the original board
byte `$140011=$7B` before completion. SYNC=3 waits for the original IRQ ACK
`$1C006F.bit1`; completion with that bit still clear returns WAIT5 at the same
poll PC. The driver must supply its actual interrupt handling and must not
manufacture an ACK or clock. Pending requests are inert. Failed dependencies
are fatal and retain executed prefixes, requiring a stop or checkpoint restore.

The complete TAG/Royal branch order, initial character swaps, progress-BYTE
4/9 boss records, per-pair/per-human loops, BSET latch, motion/animation and
control graphic timing follow the ROM. The owner preserves coordinate
fractions. Resource `$4B` is corrected to `$4A` only on a WORD match, while
text/palette indices use the low BYTE of the identity. Local `$21B4` consumes
the live RAM seed/frame counter and data registers. Original CLR.W `$80`
operations preserve its low word; the separate flash owners perform CLR.L.

Mandatory external owners remain explicit: `$2052/$1FC0/$26E66/$2A06`,
`$285DA/$2AEA/$247C/$27B8/$2836`, `$8FD6/$1F9E/$2503C/$B29E`, and
`$2208/$8F50`. Flash `$8F50/$8FD6` is a foreground operation with real ACK
and VBlank polling, not a generic palette fade. World4/text/descriptor/palette
resource lifetimes belong to the compositor. Unknown actor/table indices and
unsupported cursor PCs remain guards.

The oracle runs intact ROM bodies for all external dependencies and suppresses
only `$2052` sound output. It compares complete RAM16 KiB, BG64 KiB, FG16 KiB,
palette36 KiB and board regions including padding, D0-D7/A0-A6/RNG, every
CALL/WRITE8/SYNC entry context and digest, original call/return PCs, pending
request inertness and final original RTS boundaries. Deterministic ACK and
physical VBlank levels are input fixtures; no ROM helper return, memory
operation, register or RNG result is replaced. Initial RED is the pending
`$7B70` guard. Final GREEN covers 1,100 cases, 26 guard/protocol controls and
ten delayed ACKs, including coordinate fractions, high WORD progress, mode
FE/FF, all twelve identities and high WORD identity bits. Required ROM branch
coverage is asserted. The independent review repeated this matrix without
blocking findings. Source/ROM hashes and evidence are in the manifest.

The proof is scoped to these owners and their protocol. Native dependency
composition, real IRQ mutations/input scheduling, CCR/A7 temporary stack,
physical Saturn behavior and global ROM equality remain outside this test.
No entry, Makefile, bootstrap or active VM was modified by this task.

Run from runtime18 after `. ../toolchains/host-env.ps1`:

```powershell
& $taskPython src/runtime/tests/r18/card_scene_verify_rom.py --report validation/local-20261024/card-scene-final-green.json
& $taskPython src/runtime/tests/r18/card_scene_verify_rom.py --pending --smoke
```
