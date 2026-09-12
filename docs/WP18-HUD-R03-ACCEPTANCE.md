# WP18 HUD Integration R03 — master acceptance

External checkpoint:
`WP18-HUD-INTEGRATION-R03-AUDITED(1).zip`

SHA256:
`8c8b46fa3b1f60f8e76fab2f76e6692177f9171e93712f9a946a0b15f148f19f`

External bundle HEAD:
`6dfd3db09f7b45fc8b2011ec6f557aa2ab6aabcd`

The external branch is not currently published on the remote. The supplied bundle is the reproducible checkpoint.

## Independent re-audit

- package SHA256SUMS: 51/51 PASS;
- bundle clone/fsck: PASS;
- HOST strict HUD test: PASS;
- HOST timer reference: PASS;
- SH-2 GCC 13.2.0 independent compile with `-Wall -Wextra -Werror -ffreestanding -fno-builtin -m2`: PASS;
- ROM palette audit against `wwfwfest.zip`: PASS;
- all 4096 xBGR444 -> RGB888 conversions: PASS;
- portrait ROM audit: PASS;
- preview regeneration: exact three PNG hashes reproduced.

## R02 defect closed

R02 interpreted arcade 12-bit words as RGB444. The source format is xBGR444 (`0x0BGR`).

Required sentinels:
- `0x00F -> #FF0000`;
- `0x0F0 -> #00FF00`;
- `0xF00 -> #0000FF`.

This exactly explains the previously observed blue / red-blue-inverted HUD portraits.

## Contract closed

- 12 portrait descriptors from `$7814`;
- `$77D8` 3x3 portrait writer;
- selector 5 suppression;
- config-1 combat TEXT palettes;
- exact portrait tile hashes from `31e12-0.ic33`;
- four HUD anchors;
- player indicators;
- six-column stamina view;
- timer view;
- referee/outside count layout;
- fixed HUD under camera/shake;
- VDP2 NBG2;
- CRAM mode 2 RGB888;
- TEXT CRAM window `0x000..0x0FF`.

Integration Master folds the exact renderer-facing pieces into `wf_hud_core.*` instead of creating a duplicate HUD state owner.

## Still outside scope / fail-closed

- human wrestler-name labels for portrait selectors;
- generic score;
- Royal elimination counter without a proven consumer;
- WP16 internals;
- final production renderer hookup where still external;
- real Saturn hardware validation.

Verdict: **WP18 HUD source/model/renderer contract PASS.**
Hardware: **NOT TESTED**.
