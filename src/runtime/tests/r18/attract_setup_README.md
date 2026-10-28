# Uncredited attract preparation `$B1C`

`wf_r18_attract_setup_init/step/complete` ports the original `$B1C..$BD0`
controller, local `$E02..$F20` raw demo setup and physical VBlank delay
`$21E6..$2206`. It shares the existing 16 KiB arcade RAM and live D0-D7/A0-A6.
Start at B1C, E02 or 21E6. RETURN stops before their original RTS. No runtime
reset, chosen seed, synthesized input or fixed gameplay frame is introduced.

The first title timeout normally has WORD `$166=0`. That branch requests C98,
executes E02 locally, requests the original foreground flash 8FD6, clears LONG 80,
requests 1F9E, sets WORD 15FC=2 and requests 2A06. The outer AC0 caller then owns
8AE6 and the original F8C handoff. E02 clears only its original globals and
uses the exact F22/F3E/F76/F7C tables. TAG writes identity BYTES and CPU roles,
preserving high identity bytes, gives actor0 health 7000 and selector-table
health to the next three, sets reciprocal links and clears actors4/5. A low
BYTE selector 1 takes the original Royal branch, including 2988, six identities
in reverse D0 order and the existing raw 1086C owner. No roster is invented.

CALL=0 requests expose original owner/call/return PCs; run the full actual
owner and then complete with WF_OK. READ16=1 requests the physical word at
140026. Complete with the raw word: an incorrect level returns WAIT5 at the
same poll PC. The original 21E6 counts D0.W+1 high/low periods with DBRA and
restores the entire incoming D0. Pending requests are inert. Failed dependencies
are fatal preserved prefixes; WF_BLOCKED is never interpreted as waiting.

Special selectors 1/2 request the ranking scene 69C8 before its original 257-period
delay and fade. Selectors 6/A request the larger visual/input scene 8490. These
two original owners remain explicit required native dependencies. The controller
does not skip them or claim their implementation. All other dependencies use
existing native owners: AE20,264E2,C98,8FD6,1F9E,2A06,2AEA,2988 and1086C.

The intact-ROM oracle covers all 14 selectors, direct E02 variants with high
identity bytes and low-BYTE Royal selector aliases, random live registers/RAM
inputs, complete delay counters 0/1/2/3/7/20/100 and one-quantum DBRA boundaries
0/1/7FFF/8000/FFFF. It compares full RAM 16 KiB, BG 64 KiB, FG 16 KiB, palette 36 KiB
and board padding, D0-D7/A0-A6/RNG, every CALL/READ16 context/digest/order and
original call/return PC, request inertness and RTS/poll boundaries. Dependencies
execute their full intact ROM bodies; only 2052 sound output is suppressed.
The selector 0 fixture is constructed input, not a captured natural frontend
snapshot; the parent driver must verify its own live timeout composition.

Initial RED is the pending B1C guard. GREEN 115 plus 19 controls and 18 wrong-level
polls are recorded with exact source/ROM hashes in the manifest. The proof is
limited to preparation and protocol. The attract combat loop, special scene
native implementations, real IRQ mutations/cadence, CCR/A7 temporary stack,
complete driver and hardware behavior remain separate. No entry, Makefile or
active VM was changed by this owner task.

Run from runtime18 after `. ../toolchains/host-env.ps1`:

```powershell
& $taskPython src/runtime/tests/r18/attract_setup_verify_rom.py --report validation/local-20261024/attract-setup-final-green.json
& $taskPython src/runtime/tests/r18/attract_setup_verify_rom.py --controls-only --report validation/local-20261024/attract-setup-controls-green.json
& $taskPython src/runtime/tests/r18/attract_setup_verify_rom.py --pending --smoke
```
