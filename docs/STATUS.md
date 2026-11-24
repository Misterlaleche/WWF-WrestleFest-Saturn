# Current status / Estado actual

**Updated 25 November 2026. Current source: P1-03B BOOTFIX CANDIDATE (22 November).**

This document is the current overview. Earlier dated implementation reports describe the revision named in each report. Their results must not be promoted to a pass of a later executable.

Este es el resumen actual. Los informes anteriores describen su propia revisión; sus resultados no acreditan automáticamente un ejecutable posterior.

## Acceptance matrix

| Area | Evidence available | Current limit |
| --- | --- | --- |
| Source recovery | P1-03B package hash verified; runtime files imported unchanged; inherited adapted COMMON restored and hash-checked | External compiler and prepared disc assets are still required |
| Boot correction | New Adler-32 compilation policy and detailed load phases; original SH-2 build and host logs recovered | P1-03B reaching the title on a physical Saturn is not confirmed |
| Frontend, TAG and Royal | Integrated source and host/reference tests are present | Full image, timing, interactions and route acceptance remain open |
| TAG progression | Original P1 regression reaches match two with ordinary test-driver inputs | Does not certify the complete campaign or physical play |
| Records, initials, credits and save | Code and regression helpers are present; older scoped save/oracle evidence retained | Complete current-build physical validation remains open |
| Music | P1-03 preparation for all 18 cues and implemented five-second SCSP fade | Exact loop/envelope/lifecycle equivalence and physical audio continuity remain open; long tracks are finite 600-second captures |
| Performance | Earlier measurements and instrumentation are retained as dated evidence | No new hardware cadence measurement; arcade-speed acceptance remains open, particularly for Royal |

## Checks run for this repository refresh

The selected asset-independent host suite passed **13/13 checks**. The [machine-readable report](../evidence/repository-refresh-20261125/host-tests.json) records commands, source identities, outputs and excluded work. Adjacent logs preserve the actual results.

Coverage includes the BOOTFIX compilation policy, sanitized input adapter and zlib bit checks, band-limited resampling, music streaming/deadline contracts, boot-progress observation, save format/adapter, video/pipeline, clock and GFX1 ordering. These are focused checks for the imported source and test maintenance; they are not a claim that every test in the repository was run.

### Controller harness maintenance

The imported `tests/test_jo_controls.py` no longer compiled because the production `input_word` callback had acquired a ROM-read dependency absent from the test harness. The repair adds the current asset interface and a test-only synthetic ROM reader. The production game callback is unchanged.

The repaired test checks 96 movement cases, buttons and port routing, big-endian ROM reads, valid range boundaries, 24-bit address aliasing, invalid arguments and failed-read status propagation. Three temporary negative controls were rejected by assertions. Runtime files under `core/`, `port/` and `sdk/` were unchanged by the repair.

### Not run in this refresh

- A new SH-2 game build or memory-margin validation of a new target executable.
- The full asset-dependent suite, complete game-route replay or new audio asset generation.
- A Saturn emulator run of this candidate.
- Physical Saturn, MODE, CD-R or loader compatibility testing.

The required game assets and compiler were not restored for those operations. No new BIN/CUE is released by this repository refresh.

## Recovered evidence is a separate category

[`evidence/p1-03b-20261122/`](../evidence/p1-03b-20261122/) contains the original build/map, asset, video, P0-01 and TAG regression logs recovered from the BOOTFIX source archive. Its program identity and 8,208-byte static margin are recorded in [PROVENANCE.md](PROVENANCE.md).

[`evidence/initial-load-r03-20261113/`](../evidence/initial-load-r03-20261113/) is earlier R03 evidence. Reports under `docs/` that describe R02, R03, C/E checkpoints or isolated Yaul/audio harnesses retain that historical scope. In particular, the 12 November TAG/Royal emulator percentages are not fresh P1-03B results and are not physical-Saturn measurements.

## Next physical check

Use the exact P1-03B disc with its CUE and the 4 MB expansion. First establish whether it passes the full sprite-pack load and reaches the initial screens/title. If it stops, photograph the complete diagnostic, including owner, data, filename and byte counters.

In the revised diagnostic, owner `0018CD10` with data `02000007` identifies an asset-format failure during the sprite-index checksum phase. This is a diagnostic interpretation, not evidence that a new failure has occurred. The [BOOTFIX report](p1-03b-bootfix-20261122.md) gives the full context.

After successful boot, test the title and Legion of Doom music, fades, transitions, sustained TAG/Royal play and records/save behavior. Record the exact console, cartridge and loader mode. See [CONTRIBUTING.md](../CONTRIBUTING.md).

## Resumen en español

Las fuentes actuales son **P1-03B candidata**, con su configuración de compilación recuperada. La renovación supera **13 comprobaciones seleccionadas de host** y corrige exclusivamente el adaptador de una prueba de controles, además de la documentación y la organización del repositorio. No modifica la lógica del juego.

Los informes de compilación SH-2, margen de 8.208 bytes, recursos y progresión TAG proceden de la entrega original del 22 de noviembre. Durante esta renovación no se ha vuelto a compilar el juego, generar un disco ni probar en emulador o Saturn física. La primera confirmación pendiente es superar la carga y alcanzar el título con P1-03B.

La aceptación completa sigue abierta: recorridos TAG/Royal, gráficos y tiempos, rendimiento sostenido, récords y guardado, audio sin interrupciones y bucles musicales exactos. Las pistas largas siguen siendo capturas finitas de 600 segundos y el fundido implementado no demuestra igualdad exacta de envolvente con el arcade.
