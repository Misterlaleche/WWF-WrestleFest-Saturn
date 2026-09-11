# BLOCK-INTRO-PRESENTATION-01 — HARDWARE TEST03

Status: **CANDIDATE / physical validation pending**.

TEST03 is the first isolated harness in this block that continues past the Technos plate into a native TITLE composition instead of stopping at the TITLE handoff.

Implemented for hardware observation:
- exact boot plate assets;
- 128-VBlank plate hold;
- 8 x 3-VBlank common fade;
- native indexed starfield/background;
- 128-VBlank TITLE prephase;
- 12-card build, 42 VBlanks per card (4+4+4+4+26);
- corrected card appearance order and 4x3 positions from frame-difference correlation;
- separate indexed WWF/WrestleFest logo;
- 45 recovered logo palette states, advanced every 3 VBlanks once; no fabricated loop;
- 1480-VBlank main TITLE phase;
- CREDIT/prompt through a read-only WP19-style snapshot;
- harness uses the ROM-demonstrated zero-credit f1260 scenario (credit value 0, prompt selector 0x02 INSERT COIN) only as test input.

No local credit accounting exists. Production ownership remains WP19/master.

No audio backend changes.

Build:
- host/SH-2 compile: PASS
- Saturn ELF link: PASS
- unresolved symbols: 0
- MODE1/2352 payload roundtrip: PASS
- EDC/ECC regeneration/verification: PASS
- real Saturn + MODE: PENDING

Artifact SHA-256:
- BIN: b4d0c209b444ba4bc39c4c3114fd91a7ed84b6c04eb4e61998cd8a0145bde759
- ZIP: 10d98ff68be9d9e6ab8c508e1decd8abc29a00354da5a70601f7b54705ee6fcf
