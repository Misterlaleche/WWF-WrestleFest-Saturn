# New-chat handoff — WWF WrestleFest Saturn

Use this private repository and the persistent ChatGPT Library as source of truth.

## Current start point

1. Read `PROJECT-STATE.md` first.
2. Current validated canonical source snapshot:
   - `/WWF-Saturn/WWF-Saturn-Test150-V0165-source.zip`
   - SHA256 `34e3b712696b2bf7959bf33cc00d7c18987f3e4145bb7cd55406e8803025f4e0`
3. Continue from **Test151 / V0.166**. Do not resume from Test144 or older snapshots.
4. Host regression at Test150: **144/144 PASS**.
5. Targeted ASan/UBSan for Tests148-150: **PASS**.
6. Test147/V0.162 is the latest SH-2/Yaul compile+link PASS; Tests148-150 changed only host-side raw helper/tests/docs/Makefile label and were not target-built in that runtime.
7. No new BIN/CUE was produced for Tests145-150.

## Hardware state

Test144/V0.159 remains the current pending hardware-validation image. Test100/V0.115 remains the latest image physically confirmed on NTSC-U Saturn + Terraonion MODE until the user tests Test144.

## Critical new finisher evidence

- Test145: Slaughter `$17ECE` and DiBiase `$179C0` each write exact `#$0005` to victim `+$68` before `JSR $111C8`; this is a per-pulse operand, not proven total damage/cadence.
- Test146: the old Earthquake `$23/$15CE0` Vertical Splash candidate is contradictory because `$23->$15CE0` is independently proven STOMP. Earthquake is gated off unless HIGH_CONFIDENCE.
- Test147: Earthquake (selection 8), prone category `$07`, fresh button1 -> exact raw action `$47`, descriptor `$14440`, callback `$14458`, first recovered pose 132. Human-readable identity remains CANDIDATE.
- Test148: `$14458` exact raw prelude: actor `+$60 bit7` or `+$1C bit7` early-branches; otherwise clear actor `+$01`, write `+$5C=$0600`, `+$5E=$0048`, choose D0=5 only if target `+$21==$05 && +$61==$61`, else D0=2, then call `$26AE`.
- Test149: `$26AE` proves D0 is a 4-byte table selector (`LEA $275C,A1; LSL.w #2,D0; ...`), **not a damage value**.
- Test150: selector 2 raw pair is `{$0038,$FFF8}`; selector 5 raw pair is `{$0040,$FFFF}`. Keep them as raw effect/vector evidence until semantics are proven.

## Fidelity rules

Every Test must implement or rigorously validate a real function. Preserve evidence labels: **ROM-EXACT**, **PROVISIONAL**, **REGISTERED BUT NOT IMPLEMENTED**. Never promote a move, damage value, AI behavior, geometry, timing or presentation detail to arcade-exact from visual plausibility or memory.

## Immediate continuation

1. Continue Earthquake raw action `$47`/callback `$14458` until composition and actual effect consequence are closed; do not treat selector 2/5 as damage.
2. Recover exact TAG/Royal victory, defeat and winner-clap/celebration descriptors/states/coordinates/timing.
3. Continue stamina raw path `+$66 -> +$BD`, `+$109`, `+$20/+$60/+$61` until gameplay semantics are demonstrated before wiring stamina-tier CPU behavior.
4. Complete/prove all Super Throws / Signature Finisher Moves, respecting per-record evidence flags.
5. Revalidate useful evidence from the historical non-canonical Test140 research archive only under new future Test numbers.
6. Continue exact TAG apron/entry/exit, Royal assets/eliminations, records, attract/promo, presentation and audio work as evidence permits.

## Long target

Continue concrete checkpoints toward Test200. At genuine Test200 report exactly:
- IMPLEMENTADO
- REGISTRADO PERO NO IMPLEMENTADO
- PROVISIONAL
- PENDIENTE DE BUILD SATURN
