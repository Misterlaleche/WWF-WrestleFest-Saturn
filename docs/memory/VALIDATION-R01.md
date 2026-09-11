# WP32/WP33 integration candidate — validation

- Original main 68000 SHA1: `a726c46f350723ea73158bc78055d17d418b14e2` — PASS (locally reconstructed from the supplied ROM set).
- Host compile: PASS, `-std=c11 -Wall -Wextra -Werror`.
- Host tests: PASS.
- SH-2 object compile: PASS, big-endian Renesas SH relocatable object.
- Isolated contract object size: text 932 bytes, data 0, bss 0.
- Full Saturn ELF link: **not claimed for this isolated platform contract**; gate belongs after gameplay/frontend source merge.
- Hardware PASS: **not claimed**.
- BIN/CUE: **not generated**.

Local audited package:
`WF-INTEGRATION-MASTER-WP32-WP33-R01-CANDIDATE.zip`

Package SHA256:
`c9ccaa62c13280dec87d3ee8f64bffb518d0b46e42d8a12839494eed8c19fd25`
