# WIP66 — CPU prone repertoire obeys wrestler-specific ROM action rows

Status: Ground Actions runtime fidelity checkpoint. No BIN/CUE. No ROM files.

## Closed
- CPU ground-pressure recovery no longer calls generic PICK UP for every wrestler.
- CPU category-$05 P+K now resolves through `arcade_action_for_buttons(character,5,3)` exactly like human input.
- raw `$22` alone enters `ST_GROUND_PICKUP`.
- Any other valid P+K raw action enters its exact `ST_GROUND_ATTACK` descriptor path; raw `$48` remains reserved for cover/pin.
- The normal CPU category-$05 button1 branch also uses the same helper, keeping all CPU ground-action entry table-driven.

## Concrete correction
- Hogan category5 P+K -> raw `$22` -> PICK UP.
- Jake category5 P+K -> raw `$09` -> ground action, never fabricated PICK UP.

## Tests
- Test137 updated to an actual raw-$22 wrestler: PASS.
- Test202 anti-loop expectation updated to wrestler-specific P+K repertoire: PASS.
- Test214 human raw-$22 fidelity: PASS.
- New Test350 CPU ground table fidelity: PASS.
- `python3 -m py_compile tools/check_host.py` PASS.

## Safety
- No held-release blockers changed.
- `tick==12` unchanged.
- PICK UP callback handoff at tick18 remains PROVISIONAL.
- No category6/7 synthesis was added; those require real target raw-state provenance.

## Persistence
- Library: `/WWF-Saturn/WWF-Saturn-Test255-WIP66-source.zip`
- SHA256: `744c28c532dd820e40f5916d839074f9cd21e22cb9e59622b980d8b513f1db3b`
- No ROMs. No BIN/CUE.

## Next
Continue Ground Actions by removing remaining native repertoire shortcuts that bypass the ROM action rows, then return to inherited raw-state/probe provenance. Do not enable category6/7 until target raw-state provenance is represented explicitly.
