# Contributing / Colaborar

The goal is faithful arcade behavior on real Sega Saturn hardware. A useful change explains the problem, identifies the original behavior or current contract, and includes the smallest relevant check. Keep the original ROM and audited comparisons as behavioral evidence; do not promote assumptions to arcade facts.

El objetivo es reproducir el comportamiento del arcade en Saturn física. Una contribución útil explica el problema, identifica la referencia o el contrato actual y aporta la comprobación pertinente. Las suposiciones no sustituyen a la ROM ni a las comparaciones auditadas.

## Before changing code

Read the [current status](docs/STATUS.md), [build guide](docs/BUILDING.md), and the relevant technical document under `docs/`. The latest target is Jo Engine with 4 MB RAM. Historical Yaul and isolated audio/scene branches are separate work, indexed in [HISTORY.md](docs/HISTORY.md).

Keep unrelated changes separate. Preserve generated runtime tables, frozen test references, compiler workarounds and memory reservations. Changes to asset generation must also update and verify their lock/metadata deliberately. Do not include ROMs, BIOS files, sound/graphics packs, private credentials, compiler executables, or unrequested disc/capture archives in commits.

## Hardware report / Informe de hardware

Please include / Incluye:

- **Identity / Identidad:** source commit, candidate name, and executable or disc SHA-256 where available.
- **Setup / Equipo:** Saturn model/region, RAM cartridge, MODE or optical drive/CD-R, boot loader and the exact loading mode selected.
- **Steps / Pasos:** mode, wrestler, match/scene, actions and where the problem first appears.
- **Observation / Resultado:** what actually happened and what you expected from the arcade. A complete diagnostic photo is useful; preserve owner, data, filename and byte counters.
- **Repeatability / Repetición:** whether a cold boot reproduces it, whether it is intermittent, and which previous exact build worked.
- **Timing / Cadencia:** distinguish subjective smoothness, captured elapsed time, emulator fields and measured console updates. Keep the measurement method with any percentage.

A report need not contain specialist measurement equipment. Clear steps and the complete screen are more useful than a guessed diagnosis.

No hace falta equipo especializado. Los pasos claros y una pantalla completa son más útiles que una causa supuesta.

## Test scope

Host tests can check logic, contracts, corruption handling and comparisons with reference implementations. They do not certify Saturn timing or hardware. Report the command and result, as well as unavailable inputs or checks not run. Use isolated emulator backup profiles for tests that deliberately create or damage test save files; never use a user's ordinary save profile.

For changes to the test harness itself, keep the production functions under test unchanged and state which dependency is substituted. The 25 November controls-harness repair is documented in [STATUS.md](docs/STATUS.md).

## Attribution

Keep file-level copyright and license notices intact. See [THIRD-PARTY.md](docs/THIRD-PARTY.md). This repository does not grant rights to original game assets or establish a new blanket license for all code.
