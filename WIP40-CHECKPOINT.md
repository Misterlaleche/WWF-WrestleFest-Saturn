# Test255 WIP40 — unified prone held-gate callback semantics

## Scope
Close the callback-side semantics common to every mapped prone `$FF00` gate without inventing the still-missing pre-hold raw-Z/probe lifecycle.

## ROM-EXACT / evidence-preserving code
- Added `arcade_prone_hold_callback_step(raw, hold_phase, contact37, probe42, release_hold)`.
- Every mapped prone held gate consumes the already-resolved actor `+$37 bit4` as its release event.
- Probe-writing gates keep the WIP39 exact side effects:
  - raw `$0E` hold phase 2: arm `+$42=-$18`, clear on consumed contact;
  - raw `$10`: arm `+$42=-$20`, clear on consumed contact;
  - raw `$14`: arm `+$42=+$20`, clear on consumed contact;
  - raw `$35`: arm `+$42=+$20`, clear on consumed contact.
- Non-probe-writing held gates do not mutate `+$42`; they only consume the resolver-produced `+$37 bit4` gate.
- Raw `$0E` remains represented as three separate held phases.

## Tests
- Test314 PASS: `$FF00` action clock advances only on explicit release.
- Test315 PASS: base contact37 resolver composes stage + actor32 + raw-Z + probe42 without a timeout.
- Test319 PASS: all eleven mapped held occurrences use contact37 as release; only the four ROM-proven immediate-probe gates mutate `+$42`.
- ROM-dependent Test316/317/318 cannot be re-run from this source archive because `program.bin` is intentionally absent.

## Not promoted / still open
- Live `ground_attack_update()` still does not pass `release=true` into the action clock.
- The legacy `ground_attack_visual_duration(...)=32` fallback therefore remains for callback-held descriptors.
- Exact pre-hold `+$0E` / inherited `+$42` lifecycle is still required for raw `$0B`, raw `$0E` phases 0/1, raw `$13`, raw `$23`, raw `$46/$47`, and must also be reconciled for the four immediate-probe gates before removing the fallback globally.
- Damage/callback side effects, old `tick==12`, PICK UP `$22` victim handoff, and initial `+$65` producer remain open.

## Fidelity labels
- **ROM-EXACT:** gate identity, contact37 consumption semantics, four immediate probe arm/clear side effects.
- **REGISTERED BUT NOT IMPLEMENTED:** live held-frame release bridge pending exact pre-hold Z/probe state.
- **PROVISIONAL:** legacy 32-tick fallback and native ground damage timing remain in runtime.

Source checkpoint:
- Library: `/WWF-Saturn/WWF-Saturn-Test255-WIP40-source.zip`
- SHA256: `d1fef4dfb29a424be425d206285d668cdf0a1478dd359a4c98f568be7a04da3a`

No ROM image included. No BIN/CUE generated.
