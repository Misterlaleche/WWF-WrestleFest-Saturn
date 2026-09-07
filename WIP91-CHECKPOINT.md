# WIP91 — exact category-$09 button dispatch on won grapple

## Runtime
- A normal HOLD produced by the exact raw `$29` terminal is recognized by raw topology attacker `$0C`, linked target `$FF`.
- In that context, fresh action buttons now use the recovered category `$09` table:
  - P -> raw `$15`
  - K -> raw `$17`
  - P+K -> raw `$15`
- This fixes the previous native mismatch where P+K released the grapple even though the arcade category maps it to the same raw action as P.
- `grapple_raw_action` records the selected raw identity for regression/diagnostics.

## Fidelity boundary
- ROM-EXACT: category `$09` button-to-raw-action identity (`$15/$17/$15`) once the live pair is in the recovered `$0C/FF` ownership topology.
- PROVISIONAL: semantic bridge `$15 -> current grapple move rota` and `$17 -> current native Irish-whip path`; the human-readable callback identities of raw `$15/$17` are not yet closed.
- Native HOLD reversals are intentionally excluded from the ROM-EXACT category-$09 claim because their raw ownership transition has not been recovered.

## Tests
- Test377 PASS.
- Test378 PASS.
- Test379 PASS.
- `python3 -m py_compile tools/check_host.py` PASS.

## Source
- `/WWF-Saturn/WWF-Saturn-Test255-WIP91-source.zip`
- SHA256 `6b7ab47a922c646bc1ca5a333d35929c41fd7018c79f11468e188e5dba3778b2`

## Next
- Audit raw `$15/$17` descriptors/callbacks and HOLD reversal ownership. Do not infer semantic move names without ROM evidence.
