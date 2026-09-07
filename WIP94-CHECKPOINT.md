# WIP94 — raw `$15/$17` direct victim-producer audit

## ROM-EXACT closure
- raw `$15`: descriptor `$14CE0`, callback `$14CF4`, finite visual profile already closed in WIP93.
- raw `$17`: descriptor `$14E2C`, callback `$14E40`, finite visual profile already closed in WIP93.
- Complete action-side state-4 producer registry reports **zero** direct victim `+$64/+65` producers for raw `$15`.
- The same registry reports **zero** direct victim `+$64/+65` producers for raw `$17`.

## Consequence
- The current native semantic bridges (`$15 -> grapple rota`, `$17 -> irish_whip`) cannot be promoted to ROM-EXACT from the retained state-4 producer evidence.
- Visual poses/timing alone are insufficient to name either callback.
- No runtime semantic change in this checkpoint.

## Tests
- Tests379–382 PASS.
- `python3 -m py_compile tools/check_host.py` PASS.

## Source
- `/WWF-Saturn/WWF-Saturn-Test255-WIP94-source.zip`
- SHA256 `7c7a3d1520c8316615588ceef993b52eca70811b09aef8204edb813b64600790`

## Next
- Modernize stale grapple regression fixtures so they traverse raw `$29` struggle -> `$0C/FF` ownership before testing the still-PROVISIONAL move rota/counter layer.
