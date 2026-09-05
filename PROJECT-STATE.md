# WWF WrestleFest Saturn — private development repository

Private working state for the native Sega Saturn port/reimplementation.

## Current validated checkpoint

- **Test100 / V0.115**
- Host regression: **96/96 host binaries PASS** (executed 48/48 + 48/48 to avoid runner timeout)
- Persistent source snapshot: `/WWF-Saturn/WWF-Saturn-Test100-V0115-source.zip`
- Source snapshot SHA256: `e489380655d7f156b1e2ddb0fd00071e28460d1c8d33326a3042fc8b39fc4b96`
- Clean extraction of that snapshot recompiles successfully with pinned Yaul and passes `tools/verify_iso.py`.
- SH-2 boot executable is deterministic across the working tree and clean snapshot rebuild.

## Test100 Saturn hardware image

A real Saturn build was produced with Yaul from the clean Test100 source snapshot.

- Yaul Docker image used to export the SDK: `ijacquez/yaul@sha256:b6d272204767cd0a734139dd3a2ab298ef5912fb7b7224b78e6d9fac1a7825a6`
- Yaul toolchain: `sh2eb-elf-gcc 13.2.0`
- Program metadata: `V0.115`, release `20260905`, title `WWF WrestleFest Test100`
- Clean-rebuild ISO SHA256: `4cb4f5f11cf73e2d2ca7ccb978bc9e6b36b75b8f3cffa797f5afbaa4a5b95afb`
- Raw BIN path: `/WWF-Saturn/WWF-WrestleFest-Test100-V0115.bin`
- Raw BIN SHA256: `0ac98e88a4db30bf51a9edcd13fc3598442f22095a2b400d48e6da331954f3b3`
- CUE path: `/WWF-Saturn/WWF-WrestleFest-Test100-V0115.cue`
- CUE SHA256: `4125d7532ed37f54f2e1bbf1cdcc752ed7fa86b4eebc498ccbe55590c6029ef5`
- Disc: 2211 sectors, single-track `MODE1/2352`, NTSC-USA area present.
- `tools/verify_iso.py`: PASS — Saturn IP, ISO9660, USA area, all 35 CD files byte-identical to build inputs, `A.BIN` byte-identical to the compiled SH-2 executable.
- `tools/iso_to_bin.py`: PASS — every 2352-byte sector regenerated and compared; all ISO payload sectors round-trip byte-identically; EDC/ECC regenerated for every sector.
- Independent `libcdio` read: PASS — CUE accepted, 2211 sectors read, payload byte-identical to ISO, EDC checked bitwise for all sectors.
- **Hardware boot test: NOT YET PERFORMED.**
- **Emulator boot test: NOT YET PERFORMED in this build environment.**

### Saturn compile correction at Test100

The first real SH-2 compile exposed one host-only visibility defect: `main.c` referenced `ARCADE_DIFFICULTY_NORMAL` without directly including `arcade_stamina.h`. Test100 was corrected by adding that canonical include; no gameplay behavior or values changed. Because source bytes changed, the old Test100 snapshot hash `cc9912a3...` is obsolete and was replaced by the source snapshot/hash above.

## Major integrated state through Test100

- Arcade HUD/stamina model, CPU stamina progression, ten-stage TAG campaign, Continue flow, persistent player stamina, CPU/apron partner logic, Power Up, grapple rota, submissions, ground actions and exact 20-count count-out are integrated.
- Ringside pickup/use, CPU weapon seeking/use, ROM-backed release geometry, state-2 collision resolution work and state-4 reavailability lifecycle are integrated to the demonstrated level.
- Two-button physical action layer is recovered: input normalization, human pickup/use, full 12×21×3 contextual action matrix, running counters, behind actions, stationary actions and contextual pin.
- Mapped CPU trademark finishers execute for grapple, prone, interception and behind contexts; non-grapple opportunity frequency remains provisional.
- Raw prone categories `$05/$06/$07`, paired categories `$08/$09`, pair-state `$0B/$0C/$29` transitions, pair-role links and active/reserve handoff evidence are recovered to the documented Test75-94 level.
- **Test95:** live behind actions: P starts Sleeper as a linked submission; K starts Restrain as a linked hold when the recovered behind geometry/context is satisfied. Native state translation and Restrain timeout/release remain provisional where ROM semantics are not yet demonstrated.
- **Tests96-99:** first functional Restrain partner-assist block: reserve enters, performs one assist strike, returns to reserve; KO reserve cannot assist; Sleeper does not use this offensive assist path. Historical/ROM evidence supports Restrain-dependent partner involvement; exact Saturn path timing and current assist damage remain provisional.
- **Test100:** integrated regression checkpoint and first requested post-campaign Saturn BIN/CUE build.

## Provisional

- Native deterministic RNG feed used by the Saturn CPU weapon branch; ROM-proven bounds/weighting are exact.
- Exact initial ringside-object coordinate conversion into the Saturn world model.
- CPU priority/frequency for non-grapple trademark opportunities.
- Translation of some raw paired actor states into native Saturn states.
- Restrain native timeout/release behavior where callback semantics remain incomplete.
- Test96-99 partner-assist trajectory/timing and current 8-point assist damage; these are not claimed arcade-exact.
- Absolute Saturn placement/scale corresponding to recovered raw active/reserve coordinates; raw geometry is preserved separately rather than forcing an unproved 1:1 transform.

## Registered but not yet exact/complete

- Exact ROM-side entry/lifecycle for all three documented TAG double-team forms.
- Exact semantic/action callback mapping for all paired category `$09` actions `$15/$17`.
- Full live mapping of raw pair fields/states.
- Remaining LOD assets/behaviour, records, full Continue/Buy-In/Regain Power presentation, stages/cage, richer Rumble eliminations, attract/presentation and audio.
- Hardware/MODE validation of Test100 BIN/CUE and any Saturn-only rendering/input/runtime defects it reveals.

## Recovery / lineage note

The historical Test52 persistent ZIP was incomplete. Test55 recovered that line from Test51. Test65 exposed another manifest defect and Test66 fixed checkpoint packaging. Later obsolete branches were rejected before overwriting canonical state. Test100 was rebuilt from its complete persistent source and its checkpoint package now contains 373 source/data/report files with CRC plus `SOURCE-SHA256.txt` verification.

## Next priority

Continue from **Test101 / V0.116** only after preserving Test100 as the baseline. First priority is the requested hardware validation of the Test100 BIN/CUE on Saturn NTSC-U + Terraonion MODE (and optionally Mednafen/MiSTer Saturn). Any Saturn-only defects discovered there should be fixed before expanding unrelated systems. After that, continue concrete non-empty checkpoints toward Test200, prioritizing remaining exact TAG/double-team lifecycle, presentation/campaign gaps and audio without inventing arcade-exact behavior.
