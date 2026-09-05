# New-chat handoff — WWF WrestleFest Saturn

Use this repository and the persistent ChatGPT Library as the source of truth when continuing development in a new conversation.

## Start here

1. Read `PROJECT-STATE.md` and `docs/HARDWARE-TEST100-FEEDBACK.md`.
2. Use this current validated persistent source snapshot:
   - Library path: `/WWF-Saturn/WWF-Saturn-Test102-V0117-source.zip`
   - SHA256: `5576a4a22bb8ee957f2fa4ff80124d285f910abe0a5a6147affd55bee3ea539f`
3. Continue from **Test103 / V0.118**. Do not resume from older historical checkpoints.
4. Each Test must implement or rigorously validate a real function; no empty number increments.
5. Preserve arcade fidelity. Use `program.bin`, original research and hardware/video references. Anything unproven stays PROVISIONAL or REGISTERED BUT NOT IMPLEMENTED.
6. Keep host tests, sanitizers/checkpoint recovery and `PROJECT-STATE.md` current.

## Current validated baseline

- **Test102 / V0.117**.
- Host regression: **98/98 PASS**; new Test101/Test102 ASan/UBSan checks PASS.
- Test101 wires the ROM `$8EE0` active-actor gate into live Rumble natural stamina recovery: eliminated actors cannot recover.
- Test102 restores standard manual TAG to the original two-button model: Kick at the partner corner performs the legal-wrestler swap; Saturn C remains an optional alias; KO reserve is rejected.
- Test100 remains the latest hardware BIN/CUE and **boots/runs on Sega Saturn NTSC-U + Terraonion MODE**.

## Hardware-feedback priority

The complete user-observed backlog is in `docs/HARDWARE-TEST100-FEEDBACK.md`. Manual TAG input is now addressed in source at Test102 but requires the next eventual Saturn integration build for hardware confirmation. Highest remaining priorities are CPU difficulty/player stamina behavior, active/reserve/apron lifecycle and placement, TAG presentation, referee/pin feedback, knockdown recovery, rope deformation, TAG HUD/palettes/roster selection, Royal Rumble progression, post-match flow, records and attract/presentation scenes.

Do not arbitrarily weaken CPU or invent stamina rules: recover damage/decision/tier behavior from ROM first. Current native corner-coordinate transform and several paired-state/assist timings remain explicitly provisional.

## Long target

Continue concrete, non-empty checkpoints toward **Test200**, intended as an alpha-class integrated build candidate. Do not generate another BIN/CUE unless a Saturn-only blocker appears or the long integration checkpoint is reached. At Test200 produce the report divided exactly into:
- IMPLEMENTADO
- REGISTRADO PERO NO IMPLEMENTADO
- PROVISIONAL
- PENDIENTE DE BUILD SATURN
