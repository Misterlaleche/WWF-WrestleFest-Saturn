EC34 physical-null link specialization, 2026-10-24

Run from runtime18 after . ../toolchains/host-env.ps1:
  & $taskPython src/runtime/tests/r18/human_ec34_capture_host.py
  & $taskPython src/runtime/tests/r18/human_ec34_verify_rom.py --report REPORT.json
The capture command reads existing evidence only; it does not run combat.

The natural post-input-source HOST guard6037 is F18A/DCDE/EBC4, actor1 ID7,
state8004/subtype9, mashWORD4000, LINK26=0. Capture freezes all9actors and
D0-D7 byte for byte from the actual blocked-runtime/actors/dregs dumps.
Represented scalars/records are exported to16KiB RAM, other bytes are zero.
The snapshot starts with32 native-endian D-register bytes, then rawRAM16KiB.
No state, RNG, result, input or frame value is substituted in this fixture.

Original EC34 moves LONG26 to A2. EC38 compares BYTE21(A2) to5; physical zero
therefore reads intact ROM21=FF and branches EC6A: state5, selector78, then
EDB6 clears WORD AA. No helper/RNG/audio call is involved. A null link is not
an invalid default target in that original branch. Only this known immutable
ROM byte is specialized in production. All other nonactor LINK addresses
retain EC34, including other ROM, unrepresented RAM and interior pointers.
Actor links and null links use the MC68000 physical24-bit address. Actor
state/selector reads remain BYTE comparisons; EC60 tests bit4 of the HIGH
selector byte, not the low action byte. The link is never consumed before
the original mash/state/subtype/target34 gates.

The oracle executes the entire original DCDE entry through its real calls
and return; no instruction or helper is patched/intercepted. It compares
RAM16KiB, FG16KiB, D0-D7 and unchanged RNG/globals, and confirms ROM intact.
Final5325 cases include the natural fixture,4320 actor-link state/selector
cases, all256 fresh BYTE values with physical-null/tagFF, lazy FE/state
gates, self links, target flags and all existing EBC4 completion selectors.
439 EC38 instructions execute with physical A2=0 and read real ROM21=FF.
21 unsupported LINK controls compare the original prefix at EC34. DE86's
caller D3 is saved on stack then cleared at DEB2; the guard now retains this
live D3=0 in the shadow when available. Successful full calls preserve the
caller's original D3 via the original conceptual MOVEM boundary.

RED: validation/local-20261024/human-ec34-RED.log
GREEN: validation/local-20261024/human-ec34-final.json/.log
Natural manifest: human_ec34_manifest.json (source archive and dump hashes).
Production source SHA256:
7ae4c4bf2286279b48c18e45f4ad257468408565eaf029ab1ad5f6b0ae79d066

Original stack, A-register/CCR state and physical SMPC/IRQ timing remain
outside the typed combat ABI. This HOST snapshot is not a native trajectory
capture. Only the EC34 dependency is closed; other human classifiers,
actions and nonactor memory retain their previous boundaries. No claim of
full combat, global ROM equivalence, hardware completion or a human WIN.
