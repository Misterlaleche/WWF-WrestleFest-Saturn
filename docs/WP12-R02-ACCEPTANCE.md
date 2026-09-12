# WP12 ENTRANCES/PREMATCH R02 — Integration Master acceptance

Date: 2026-09-12

Canonical checkpoint:
`WP12 ENTRANCES/PREMATCH R02 — FINAL AUDITED SOURCE CHECKPOINT`

External ZIP:
`WF-SATURN-WP12-ENTRANCES-PREMATCH-R02-FINAL-AUDITED.zip`

ZIP SHA256:
`966ac936f2a3a71b05673675b7ef041668fd99f2cf51431ccf9290dbb146f2fa`

Authoritative branch:
`entrances-prematch-r02`

Checkpoint HEAD:
`99d316037af7508a896f0535d44dfa18775d0c47`

Base:
`main @ 3287ac70c2010b3706aaf3a76003e56f7e1fa25b`

## Independent acceptance checks

- ZIP SHA256: PASS.
- Internal SHA256SUMS: 33/33 PASS.
- Git branch topology: 1 commit ahead of main, 0 behind.
- Local audited tree vs GitHub checkpoint blobs: 34/34 byte-identical Git blobs.
- Direct ROM audit log: PASS.
- Independent master HOST/adversarial + ROM/source-table/ownership rerun against `/mnt/data/wwfwfest.zip`: PASS.
- Independent master SH-2 compile + relocatable link rerun with GCC 13.2.0 toolchain: PASS / 0 unresolved.
- Source-table audit: PASS.
- HOST/adversarial: PASS.
- ASan/UBSan: PASS.
- static analysis: PASS.
- SH-2 compile + relocatable link: PASS.
- SH-2 unresolved symbols: 0.
- no master main.c replacement.
- no Gameplay/WP28, CPU/AI, collision/damage, TAG/Royal gameplay, pin/rules, CDFS/credits or WP24 backend ownership violation.

The exact `wp12-r02/` checkpoint subtree has been imported into `integration-master-r01` without modifying its blobs.

## Accepted status

WP12 overall remains **PARTIAL**, deliberately fail-closed.

- TAG: PARTIAL.
- ROYAL: PARTIAL.
- Presenter: CLOSED structural/numeric; human selector names remain OPEN.
- Prematch referee: CLOSED structural.
- Gameplay handoff: exact boundary CLOSED; execution beyond `$F8C` remains Gameplay-owned.
- Hardware: NOT TESTED.

## External contracts that remain mandatory

1. canonical `$C98` common setup executes before WP12 confirmation;
2. shared canonical `$21B4` RNG supplies requested values; no private WP12 RNG;
3. ordered WP12 operation batches must remain ordered; only proven sync operations create synchronization boundaries;
4. WP26/canonical renderer consumes machine coordinates without guessed camera compensation;
5. Gameplay owns execution after `$F8C`.

## Still OPEN

- literal wrestler-name / red-band / WWF-logo join beyond the proven TAG `$98BA` producer;
- final machine-coordinate -> raster/camera transform;
- human semantic names for presenter selectors/raw audio commands where not independently proven;
- actual Gameplay execution beyond `$F8C`.

R01 remains rejected/superseded and must not be restored as final implementation.

No BIN/CUE generated. No hardware PASS claimed.
