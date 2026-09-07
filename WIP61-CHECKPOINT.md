# WIP61 — actor32 AI approach runtime event wiring

Status: source/runtime-shadow checkpoint only. No BIN/CUE. No ROM files.

## ROM-EXACT runtime wiring
- The already-classified `$1F354: BSET #2,+$32(A0)` event is now wired to the native CPU AI state-0 approach predicate.
- The SET is applied only when `arcade_ai_state0_enter_approach(...)` succeeds.
- It is not treated as a generic RUN alias or a generic main-state-2 alias.

## Guardrails preserved
- `$1C4CA`, `$1C52E`, and `$1CED0` remain unwired because their exact native event sources are not yet proven.
- raw `$10` live `$FF00` release remains disabled.
- The 32-tick callback-held fallback remains unchanged.
- `tick==12` damage timing remains unchanged.

## Tests
Directed PASS:
- Test329
- Test340
- Test342
- Test343
- Test344
- Test345 (new)
- `python3 -m py_compile tools/check_host.py`

A full `check_host.py` sweep was not used as the acceptance gate because unrelated legacy Test164 requires an external file not present in this materialization.

## Fidelity
- `$1F354` mutation instruction and AI state-0 approach context: ROM-EXACT from prior WIP58 evidence.
- Native event wiring to the already-existing exact AI approach predicate: implemented.
- Remaining CPU-control actor32 producers: REGISTERED BUT NOT IMPLEMENTED.
- raw `$10` live release: REGISTERED BUT NOT IMPLEMENTED.

## Source archive
- Library: `/WWF-Saturn/WWF-Saturn-Test255-WIP61-source.zip`
- SHA256: `6697e03da2ef921ede036e9686d319e370f9d24857c67d0aec5597dc80de959f`

## Next
Classify and event-wire `$1C4CA/$1C52E/$1CED0` only where exact native transitions can be proven. Then re-evaluate whether `actor32_shadow` is trustworthy end-to-end for raw `$10`.
