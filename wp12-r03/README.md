# WWF WrestleFest Saturn — WP12 ENTRANCES/PREMATCH R03 AUDITED

Cumulative revision based only on the accepted R02 checkpoint:

- base branch: `entrances-prematch-r02`
- base HEAD: `99d316037af7508a896f0535d44dfa18775d0c47`
- base ZIP SHA256: `966ac936f2a3a71b05673675b7ef041668fd99f2cf51431ccf9290dbb146f2fa`

R03 closes the ROM-EXACT **first-match TAG / ROYAL special entrance presentation** located inside `$7B70`, before the R02 `$C98 -> $A654` ring-prematch path.

Authority: `wwfwfest.zip` first; WP25/WP26/WP21/WP15/WP19/WP20/WP02 only where consistent with ROM. No R01 code is restored.

## R03 closes

- first-match gate (`$1C008C==0`, `$1C0163==0`);
- TAG vs ROYAL branch from `$1C0161.bit0`;
- world mode 4 scene, scroll seeds `$0140/$0200`, explicit priority `$7B`;
- exact mode-4 world roots, palettes and layer ordering;
- TAG two-at-once pair order, ROM pair-normalization and lanes;
- ROYAL four sequential selected-record presentations;
- `$7F0E` actor setup, entry-set mapping, resource pair, movement and terminal;
- walk selector cadence and exact `$21B4.bit0` alternate-cycle dependency;
- actor signed visual offsets, including the ID1 / selectors 4..7 `+$1B=$30` exception;
- identity -> name table -> text descriptor joins;
- `$B29E` red-band + WWF-logo producer/consumer join;
- `$8FD6` reveal and `$8F50` close-mask timing;
- `$247C` world->screen formula for the scene, using exact scroll seeds;
- absence of `CREDIT:x` producer in this special scene;
- deterministic lossless ROM-derived TAG/ROYAL APNG previews plus exact trace CSVs and golden PNGs;
- selector-slot -> runtime identity table `$64CC` for human selection; CPU roster generation remains external/OPEN.

## Still not claimed CLOSED

- Royal table entries for runtime identities 4/5 are literally `0`. `$64CC` proves no human selector slot maps to 4/5; the independent CPU partner/opponent fill producer remains OPEN in WP20, so global Royal reachability of 4/5 is not invented.
- raw audio command human names remain `REGISTERED_NOT_IDENTIFIED` unless WP24 proves them.
- exact decimal PCB refresh rate is not asserted; APNG milliseconds are preview-only. Trace CSV timing is in exact ROM actor-update/VBlank quanta (402 per presentation).
- hardware is NOT TESTED.

No `main.c`, no gameplay logic, no private RNG, no audio backend and no BIN/CUE are introduced.
