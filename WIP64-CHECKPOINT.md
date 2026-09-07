# WIP64 — actor32 linked grapple role-swap runtime wiring

Status: targeted runtime lifecycle checkpoint. No BIN/CUE. No ROM files.

## ROM-EXACT event mapping
- Retained Test88 pins the reciprocal pair-role exchange around `$21424/$214CA` using actor `+$86` links.
- Inside that exchange, `$214A4` performs `BSET #2,+$32(A2)` on the linked actor at the moment that actor gains the paired role.
- Test309 independently classifies `$214A4` as the linked-role-swap actor32 producer.

## Native runtime wiring
- Added `actor32_linked_role_swap_set()` using the exact mutation profile address `$214A4`.
- Wired it only in `reverse_grapple()` and only to `fighters[new_attacker]`, the native role that takes ownership of the grapple.
- This is not a generic grapple/idle/state alias and does not fire on ordinary grab entry.

## Safety
- `$1C4CA/$1C52E/$1CED0` remain unwired.
- raw `$10` remains blocked by actor32 lifecycle; no release timing changed.
- No 32-tick fallback removed.
- `tick==12` unchanged.

## Tests
- Test340 PASS.
- Test345 PASS.
- Added Test348 role-swap runtime-wiring guard: PASS.
- `python3 -m py_compile tools/check_host.py` PASS.

## Persistence
- Library: `/WWF-Saturn/WWF-Saturn-Test255-WIP64-source.zip`
- SHA256: `84c48ce01f03e3d681ed9d23278f5593408443bbb3282f53f30169f71a7509df`
- No ROMs. No BIN/CUE.

## Fidelity
- `$214A4` instruction/effect and role-swap context: ROM-EXACT from retained tests.
- Native `reverse_grapple()` event alias: exact semantic correspondence for reciprocal pair ownership exchange.
- Broader raw-$10 provenance remains conditional/open.

## Next
Continue actor32 lifecycle only where native event aliases are exact; otherwise shift to inherited prone Z/probe closure. The next useful target is distinguishing inherited Z from callback-written probes for raw `$14/$35` and identifying any pre-hold Z producer already represented by retained ROM tests.
