# New-chat handoff — WWF WrestleFest Saturn

Use this repository and the persistent ChatGPT Library as the source of truth when continuing development in a new conversation.

## Start here

1. Read `PROJECT-STATE.md` from this private repository.
2. Use this current validated persistent source snapshot:
   - Library path: `/WWF-Saturn/WWF-Saturn-Test100-V0115-source.zip`
   - SHA256: `e489380655d7f156b1e2ddb0fd00071e28460d1c8d33326a3042fc8b39fc4b96`
3. Continue from **Test101 / V0.116**. Do not resume from Test87 or other historical snapshots.
4. Each Test must implement or rigorously validate a real function; no empty number increments.
5. Preserve arcade fidelity. Use `program.bin` and original research evidence. Anything unproven stays PROVISIONAL or REGISTERED BUT NOT IMPLEMENTED.
6. Keep host tests, sanitizers/checkpoint recovery and `PROJECT-STATE.md` current.

## Test100 validated baseline

- Host regression: **96/96 PASS** (48 + 48 split).
- Clean extraction of the Test100 source ZIP recompiles with Yaul and passes structural ISO verification.
- A real Saturn BIN/CUE has been generated from the clean rebuild:
  - `/WWF-Saturn/WWF-WrestleFest-Test100-V0115.bin`
  - BIN SHA256 `0ac98e88a4db30bf51a9edcd13fc3598442f22095a2b400d48e6da331954f3b3`
  - `/WWF-Saturn/WWF-WrestleFest-Test100-V0115.cue`
  - CUE SHA256 `4125d7532ed37f54f2e1bbf1cdcc752ed7fa86b4eebc498ccbe55590c6029ef5`
  - clean-rebuild ISO SHA256 `4cb4f5f11cf73e2d2ca7ccb978bc9e6b36b75b8f3cffa797f5afbaa4a5b95afb`
  - 2211 sectors, single-track MODE1/2352.
- `verify_iso.py`: Saturn IP/ISO9660/USA/all 35 CD files/A.BIN checks PASS.
- Raw BIN verifier: all sectors payload-roundtrip and regenerated EDC/ECC PASS.
- Independent libcdio CUE read: PASS for all 2211 sectors and payload.
- **Hardware boot has NOT yet been tested. Emulator boot was NOT available in the build environment.**

The real SH-2 compile exposed and fixed one source-visibility defect: `main.c` needed the canonical `arcade_stamina.h` include for `ARCADE_DIFFICULTY_NORMAL`. This did not change gameplay values/logic, but changed the source bytes; therefore the earlier Test100 SHA beginning `cc9912a3` is obsolete.

## Immediate continuation

Before expanding systems, test the Test100 BIN/CUE on the target Saturn NTSC-U + Terraonion MODE. Record exact boot/render/input behavior and fix any Saturn-only issue as the next concrete checkpoint. Mednafen/MiSTer Saturn may be used as secondary validation, not as a substitute for hardware.

Once the hardware baseline is known, continue the remaining exact TAG/double-team lifecycle. Test95 live-connects Sleeper/Restrain; Tests96-99 provide a functional Restrain partner assist with explicitly provisional native timing/path/damage. Do not upgrade those provisional details to arcade-exact without further ROM/manual evidence.

## Long target

Continue toward **Test200**, intended as an alpha-class integrated build candidate. Priorities remain stamina/CPU progression, wrestler/partner AI, wrestler-specific moves, TAG/double-team/interference, ring-out/count-out/ringside objects, Royal Rumble, campaign/stages/cage, records, Continue/Buy-In/Regain Power, Game Over, attract/presentation, audio, and remaining fidelity gaps.

At Test200 produce the report divided exactly into:
- IMPLEMENTADO
- REGISTRADO PERO NO IMPLEMENTADO
- PROVISIONAL
- PENDIENTE DE BUILD SATURN
