# Changelog / Historial de cambios

## 2026-11-25 — Repository refresh / Renovación del repositorio

- Import the verified P1-03B BOOTFIX source checkpoint as the single active tree.
- Restore the 15 inherited adapted `sdk/Compiler/COMMON` files omitted from later source packages; record their exact provenance.
- Replace obsolete root instructions with English and Spanish READMEs and build guides, current status, contribution guidance, notices and immutable history links.
- Remove earlier independent snapshot trees from the active tip while preserving Git ancestry and experimental branches.
- Fix `.gitignore` so required COMMON build support stays tracked.
- Repair the controls host-test harness for the current ROM-read interface; leave game/runtime sources unchanged.

Se importa P1-03B, se recupera COMMON y se renueva la documentación en ambos idiomas. Las versiones independientes anteriores pasan al historial. Se corrige una prueba de controles y la exclusión de COMMON sin alterar las fuentes de ejecución del juego.

This is a source/documentation refresh, not a new hardware-approved game build.

## 2026-11-22 — P1-03B BOOTFIX CANDIDATE

Return Adler-32 to normal `-O2` compilation, retain other cold-code size optimizations, and add detailed asset-load failure diagnostics. The original recorded executable has an 8,208-byte static margin. Physical confirmation of the boot correction remains pending. See [report](docs/p1-03b-bootfix-20261122.md).

## 2026-11-21 — P1 updates

- P1-01 corrects the earlier TAG progression diagnosis: with complete assets and ordinary test input, the regression reaches match two. [Report](docs/p1-01-tag-progression-correction-20261121.md).
- P1-02 introduces the selected cold-code memory policy and minimum-margin gate. [Report](docs/p1-02-sh2-memory-margin-20261121.md).
- P1-03 regenerates 18 music cues with band-limited resampling and implements a five-second SCSP fade for `0x1F`. Exact loops/envelope behavior remain open. [Report](docs/p1-03-audio-bandlimit-and-fade-20261121.md).

## Earlier work

The recovered source also includes P0-01–P0-06, GFX1 and earlier integration work. Dated documents describe their original checks and limits. The old repository layout and isolated scene/audio results are indexed in [HISTORY.md](docs/HISTORY.md).
