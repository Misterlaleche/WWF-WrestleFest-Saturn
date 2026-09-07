# WIP79 — raw-keyed unique finisher producer runtime

- Replaced character/address-specific finisher victim subtype wiring with a raw-action keyed gate.
- `state4_apply_unique_raw_initial_producer()` mirrors a victim producer only when the finisher's own raw action has exactly one direct A2 state4 producer.
- This automatically enables the exact unique families: raw `$35`, `$34`, `$31`, `$44`, `$1A`.
- It automatically rejects ambiguous raw `$2F` (two producers) and raw `$23` (two producers), and rejects finisher raws with no direct producer.
- Perfect Plex therefore resolves through raw `$1A` unique producer `$15348->$06`; raw `$19` `$15154->$21` cannot alias into it.
- Tests338/358/359/360/361/362/363/364 PASS; `game.c` syntax and `check_host.py` compile PASS.
- Source ZIP SHA256: `2c1ff82d80b0b36acc2c5c0db19539d60edad79cc66e47889b8c5587c48f2b0a`.
- No held release, no `tick==12` change, no BIN/CUE, no ROM files.
