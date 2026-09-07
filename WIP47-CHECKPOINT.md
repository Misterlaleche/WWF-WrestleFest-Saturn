# Test255 WIP47 — actor32 bit2 mutation inventory + held-input provenance matrix

## Scope
Reduce the remaining physical-contact uncertainty without enabling live `$FF00` release or changing provisional damage timing.

## ROM-EXACT / evidence-preserving code
- Added a complete profile inventory for the nine immediate `+$32 bit2` mutation sites already pinned by Tests305/307/309:
  - clears: `$114CC`, `$11E04`;
  - A0 sets: `$0F3BC`, `$1C4CA`, `$1C52E`, `$1CED0`, `$1D85A`, `$1F354`;
  - A2 set: `$214A4`.
- Semantics are classified only where older ROM audits proved them:
  - `$114CC`: state-0/base init clear;
  - `$1C4CA/$1C52E/$1CED0/$1F354`: state-2-coupled producers;
  - `$1D85A`: raw-$75 producer;
  - `$214A4`: linked-A2 role-swap producer;
  - `$11E04` and `$0F3BC` remain exact mutations with context deliberately unclassified.
- No held prone callback is promoted as a direct `+$32 bit2` mutator; prior audit proves none contains an immediate bit2 mutation.

## Held-input provenance matrix
Added exact per-held-occurrence profiles for all 11 reachable `$FF00` gates:
- raw `$0B` phase 0: inherited Z, inherited probe;
- raw `$0E` phases 0/1: target-Z rebase, inherited probe;
- raw `$0E` phase 2: target-Z rebase, immediate probe `-$18`;
- raw `$10`: target-Z + `$50`, immediate probe `-$20`;
- raw `$13`: target-Z + `$20`, inherited probe;
- raw `$14`: inherited Z, immediate probe `+$20`;
- raw `$23`: inherited Z, inherited probe;
- raw `$35`: inherited Z, immediate probe `+$20`;
- raw `$46/$47`: inherited Z, inherited probe.

This does not claim the inherited inputs are already closed. It makes the remaining uncertainty explicit per hold instead of treating all holds as one generic blocker.

## Tests
- Test319 PASS
- Test320 PASS
- Test321 PASS
- Tests322–328 PASS
- Test329 PASS: exact global `+$32 bit2` mutation inventory represented without invented semantics.
- Test330 PASS: all 11 held occurrences expose exact Z/probe provenance.
- `tools/check_host.py` syntax PASS.

## Runtime status — unchanged
- Live `contact37 -> release=true`: **REGISTERED BUT NOT IMPLEMENTED**.
- `ground_attack_visual_duration(...)=32`: **PROVISIONAL**.
- legacy `tick==12`: **PROVISIONAL** and must not be removed before physical contact timing is safe.
- raw `$47` exact damage/callback profiles remain prepared but not scheduled live.

## Remaining blocker
The unresolved work is now narrowed to the actual inherited pre-hold state:
1. inherited `+$42` source/lifetime for non-immediate-probe gates;
2. inherited raw `+$0E` transitions for raws without target-Z rebase;
3. exact context of the still-unclassified `+$32 bit2` mutations only if they can feed a reachable ground path.

## Fidelity
- **ROM-EXACT:** mutation addresses/effects/targets, classified contexts where proven, held gate identities, target-Z rebases and immediate probe literals.
- **REGISTERED BUT NOT IMPLEMENTED:** live physical release bridge.
- **PROVISIONAL:** 32-tick fallback and `tick==12` damage scheduling.

Source checkpoint:
- Library: `/WWF-Saturn/WWF-Saturn-Test255-WIP47-source.zip`
- SHA256: `9c2fe6d8b3a8ae78072376455e45aff13501883428d080b6e863d2c946d080a8`

No ROM image included. No BIN/CUE generated.
