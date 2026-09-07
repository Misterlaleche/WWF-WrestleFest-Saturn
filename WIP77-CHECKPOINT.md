# WIP77 — Perfect Plex raw-event namespace correction

- Cross-audit proves Perfect Plex finisher registry/damage body is raw `$1A` (descriptor `$1520C`, callback `$15228`; exact damage writes `$152B4/$152EE`).
- The state4 producer `$15154 -> word $0021` belongs to raw `$19`, not raw `$1A`.
- Therefore the WIP74/75 runtime mirror that applied `$15154` at Perfect Plex raw `$1A` impact was an event-alias bug and has been removed.
- `$15154` remains ROM-EXACT as a raw `$19` producer and remains available in the producer registry, but it is not wired to Perfect Plex runtime until an exact raw `$19 -> $1A` transition/caller proof is recovered.
- Tests359/360 updated and Test362 added to enforce the namespace boundary.
- Tests357/358/361/359/360/362 PASS; `game.c` syntax and `check_host.py` compile PASS.
- Source ZIP SHA256: `aeb04bb8aa692a463f93d2966faa448d44e0f57b898963ebd3d2bdbed24f3596`.
- No held release, no `tick==12` change, no BIN/CUE, no ROM files.
