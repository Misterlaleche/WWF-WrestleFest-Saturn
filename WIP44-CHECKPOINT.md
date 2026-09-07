# Test255 WIP44 — exact victim +$65 callback retype profiles

## Scope
Recover and formalize already byte-pinned Test292 evidence for victim `+$65` transformation without confusing it with the still-open initial `+$65` producer.

## ROM-EXACT / evidence-preserving code
- Added `arcade_prone_target65_retype_profile(raw,&profile)` for the three callbacks already pinned by Test292:
  - raw `$0A`: callback `$13744`, transform sequence at `$13796`;
  - raw `$0B`: callback `$13A2E`, transform sequence at `$13ADE`;
  - raw `$10`: callback `$141EC`, transform sequence at `$14320`.
- Each demonstrated branch requires linked target raw state `+$21=4` and subtype `+$65=8` or `9` before applying the literal ROM transform:
  - `8 -> 6`
  - `9 -> 7`
- Added `arcade_prone_apply_target65_retype()` as a gated evidence helper. It rejects other target states, other `+$65` values, and unrelated raws.
- This is explicitly a callback-side retype/consumer-transformer, **not** proof of who initially produced victim `+$65=8/9`.

## Why this matters for Ground Actions
- The existing prone classifier already treats `+$65=8/6` as category 5 and `+$65=9/7` as category 6/7 depending on `+$70`.
- WIP44 therefore closes one downstream transformation in the `+$65` lifecycle while preserving the separate open question of initial subtype production.
- No live gameplay integration is performed here.

## Tests
- Test319 PASS: all mapped `$FF00` callbacks consume resolver contact37; only four proven gates mutate `+$42`.
- Test320 PASS: raw `$0E` maps descriptor frame identity to three distinct held phases.
- Test321 PASS: native ground entry preserves inherited `+$42`.
- Test322 PASS: raw `$47` target-contact damage profile remains exact at `+$68=10` only for demonstrated raw `$47`.
- Test323 PASS: raw `$47` contact branches A/B remain distinct.
- Test324 PASS: raw `$47` actor-local fallback remains separate from victim-contact branches.
- Test325 PASS: raw `$0A/$0B/$10` exact `+$65` retype profiles are gated to target `+$21=4` and subtype 8/9.
- Full legacy host suite was not treated as authoritative in this checkpoint because some old tests require local `program.bin`, which is intentionally absent from the source archive.

## Mandatory dependency retained
- Legacy `tick==12` must not be removed merely because exact damage values are known.
- Raw `$47` live damage scheduling still waits for correct inherited `+$0E/+$42` -> `+$37 bit4` physical contact timing.
- The 32-tick held fallback and live `$FF00 release=true` remain untouched.

## Fidelity
- **ROM-EXACT:** Test292 callback addresses and literal target `+$65` transform `8->6`, `9->7` for raw `$0A/$0B/$10` under the demonstrated target-state gate.
- **REGISTERED BUT NOT IMPLEMENTED:** live application of these callback-side `+$65` retypes.
- **PROVISIONAL / OPEN:** initial victim `+$65` producer and its full upstream lifecycle.

## Still open in Ground Actions
1. Complete pre-hold `+$0E/+$42` lifecycle.
2. Connect exact `contact37 -> release=true` only when resolver input is safe.
3. Remove 32-tick fallback.
4. Replace `tick==12` per action with exact callback producers; raw `$47` remains first prepared damage profile.
5. Continue exact callback damage/side effects for remaining ground raws.
6. Close PICK UP `$22` victim handoff.
7. Close the initial victim `+$65` producer; WIP44 closes only a downstream retype.

## Source checkpoint
- Library: `/WWF-Saturn/WWF-Saturn-Test255-WIP44-source.zip`
- SHA256: `993aff225ea1cad45c6cb2626dd796c1a02153ddb691f21bd192d99f52c8c703`

No ROM image included. No BIN/CUE generated.
