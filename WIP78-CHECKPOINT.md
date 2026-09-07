# WIP78 — correct Perfect Plex producer and raw-$10 provenance semantics

- Audited all ten finisher raw actions against the 51-producer registry.
- Perfect Plex is raw `$1A` / descriptor `$1520C` / callback `$15228` and has a unique direct state4 victim producer at `$15348 -> word $0006`.
- `$15154 -> word $0021` is a raw `$19` producer. Earlier WIP54/WIP74 wording that called `$15154` a Perfect Plex producer was semantically incorrect.
- Runtime victim shadow now mirrors Perfect Plex through `$15348`, never `$15154`.
- Raw-$10 upstream provenance helper remains structurally valid but is relabeled as the raw `$19` subtype-$21 route; its producer_raw_action is corrected from `$0E` to `$19` and the legacy Perfect Plex helper name is removed.
- Finisher producer cardinality: Hogan 1, Warrior 2, Jake 1, Boss Man 1, raw43 0, raw44 1, Slaughter 0, Perfect Plex 1, vertical splash/raw23 2, DiBiase 0.
- Tests338/357/358/359/360/361/362/363 PASS; `game.c` syntax and `check_host.py` compile PASS.
- Source ZIP SHA256: `02da3d150abf99a4b840ac5ab6538ecbd1cb1ee206251f926893b2fb21a71635`.
- No held release, no `tick==12` change, no BIN/CUE, no ROM files.
