# WIP63 — actor32 context metadata consistency

Status: source/research safety checkpoint only. No BIN/CUE. No ROM files.

## Closed
- The generic actor32 mutation inventory now matches the context classification already established by WIP58/WIP62.
- `$1C4CA/$1C52E/$1CED0` are explicitly classified as `CPU_DISPATCH_CORE`, not as the raw-state-2 range handler.
- `$1F354` is explicitly classified as `AI_STATE0_APPROACH`.
- The legacy generic `STATE2_TRANSITION` enum remains only for compatibility and is marked unsuitable for newly classified sites.

## Runtime consequence
- No additional runtime SET is wired.
- `$1F354` remains the only classified live SET event.
- raw `$10` remains blocked by `ARCADE_PRONE_RELEASE_BLOCK_ACTOR32`.
- No held release, 32-tick fallback, or `tick==12` behavior changed.

## Tests
- Updated Test329 expected context table.
- Added Test347 context-consistency guard.
- Directed Tests329/340/342/343/344/345/346/347 PASS.
- `python3 -m py_compile tools/check_host.py` PASS.

## Persistence
- Library: `/WWF-Saturn/WWF-Saturn-Test255-WIP63-source.zip`
- SHA256: `e15a502197d7c82001d2557afd199c390346d69542cd6bf46efc4634aaf3e343`
- No ROMs. No BIN/CUE.

## Fidelity
- Address/effect inventory: ROM-EXACT.
- `$1F354` event identity: ROM-EXACT from retained Test130/Test131 evidence.
- `$1C4CA/$1C52E/$1CED0` exact sub-branch identities: OPEN / REGISTERED BUT NOT IMPLEMENTED.

## Next
Do not spend runtime changes on the unresolved `$1Cxxx` sites without ROM/caller proof. Continue Ground Actions on the next independently recoverable inherited input, prioritizing probe/Z provenance before removing any held-action fallback.
