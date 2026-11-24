# Wrestlefest SMPC hardware deadline

Historical first candidate: the issued-command checks below passed, but a
normal-key follow-up exposed controller polling starvation. This candidate
was withheld. The corrected protocol, scheduling, RTC behavior and stronger
continuity gates are documented in [SMPC continuity R02](smpc-continuity-r02.md).

R01 issued controller requests less than 300 µs after VBlank-in. The intact R01
cold-boot observer recorded 1,661 such writes, minimum 29.061 µs. Sega prohibits
all SMPC command writes during that interval. Peripheral INTBACK also needs
its status continuation before VBlank-out.

With `WF_JO_ENGINE`, the SDK now waits for a safe beam position before command
issuance. Generic SDK builds retain their existing behavior.

- Control commands use a short interrupt-masked final check of the software
  INTBACK lock, SF and a freshly latched V counter, then write SF/COMREG without
  an intervening wait. Waiting restores the caller's interrupt state. Lines
  16–192 are safely within active display. This works before VBlank handlers
  exist and while the display is off.
- Periodic INTBACK timestamps ISR entry using byte-high then byte-low FRT
  reads. It waits for at least 300 µs and checks the actual beam again. A bounded
  wait prevents waiting for another field inside the ISR. An already-delayed
  callback may issue immediately if its actual beam is still safe.
- The accepted blank windows exclude the first six scanlines and leave at
  least ten lines for status acquisition and its first CONT. A skipped request
  retains the last published controller state. Expiry, continuation toggling
  and pending-command exclusion remain intact.

The periodic path supports Wrestlefest 320×240 noninterlaced, plus 320×224 during
SDK startup, on PAL and NTSC. Foreground control also accepts inherited
320/352/640/704-pixel widths, 224/240/256-line modes, and single/double interlace;
double-interlace VCNT is normalized. This does not assert a particular Pseudo
Saturn Kai startup mode. External latch/gun mode and dedicated monitor modes
are outside this guard's contract. Unsupported configurations set the sticky
`__jo_smpc_timing_fault` RAM symbol and issue no command. This SDK change alone
does not display that diagnostic; the game's display path is separate.

No TVSTAT polling is used: its VBlank flag is forced high when DISP=0, and its
reads clear the gun latch flag. EXTEN is read to latch HV only with EXLTEN=0.
No SCU IST writes, FRT resets, compare-register changes, or new timer IRQs are
introduced. FRT/8, /32, /128 are supported. The lower deadline uses the fastest
clock and a quantization tick; the upper wait uses the slowest clock. Final
beam validation independently prevents an early write if FRT wraps.

## Verification

`python3 tests/test_sdk_smpc_timing.py --mutations` runs the complete production
SMPC source with MMIO and interrupt substitutes.11,273 ASan/UBSan cases cover
PAL/NTSC, display on/off, both Saturn CPU speeds, all supported timer divisors,
FRT wrap, delayed IRQs, inherited video modes, startup, stale-transfer release,
pad retention and invalid configurations. Four semantic mutants are rejected:
removed INTBACK guard, removed control window, ignored external latch mode,
and an unsafe late CONT cutoff. The existing non-Wrestlefest SMPC suite also
passes. Independent source, primary-manual and final LTO assembly review passed.

The isolated target ELF `c279cbc4badd66171efd8dfad50c78ec65d83a91d6af9889b271dd9ba1b4c2ce`
has `_end=0x060DB224` and `__jo_work_end=0x060E9578`, preserving the linker-enforced
music allocation and stacks. All 34 original locked disc assets were verified.

| Cold boot | SDK peripheral requests | First CONTs | Minimum command after IN | Minimum first CONT before OUT | Timing faults |
|---|---:|---:|---:|---:|---:|
| NTSC |2059|2059|390.189µs|364.961µs|0|
| PAL |1765|1765|392.393µs|3883.991µs|0|

Both captures reach records status 1, writable 1, cached 1, with the RAM timing
fault zero. No game memory is written and no state is restored. The observer
uses actual emulated VDP2 edge timestamps, not the game's timer. Its disabled
SMPC object is byte-identical to the original, and its timing logger passes
60,002 synthetic rows against an independent rational clock model.

The full raw traces retain BIOS boot commands. SDK accounting begins at the
first SSHOFF whose observed PC matches the exact ELF's command function; BIOS
bootstrap INTBACKs are reported separately. This matters because BIOS code
also executes from high RAM before the game loads. These captures use connected
emulated pads without pressed keys; they verify transfers and CONT timing,
not a played match or physical-console acceptance.

Durable evidence is in `evidence/smpc-deadline-20261113`. Recheck either trace:

```
python3 tests/smpc_timing/analyze_cont_timing.py \
  evidence/smpc-deadline-20261113/ntsc-smpc.csv.gz \
  --capture evidence/smpc-deadline-20261113/ntsc-identity.json
```

The separate BIOS low-ROM/UBC issue is documented by `ubc-comparison.json`:
R01 reads ROM 0x5F8 with BBRA 0x6C; the guarded BIOS call reads it with BBRA 0.
Mednafen does not implement BBRA readback, so physical restoration is supported
by the separate full-source MMIO tests and assembly, not claimed from this
capture. Neither timing nor UBC evidence proves a fix for physical CD loading
latency. Physical Saturn testing remains outstanding.

## Primary contracts

- [Sega SMPC §2.2](https://www.infochunk.com/saturn/segahtml_en/hard/smpc/hon/p02_20.htm): command timing and first CONT before VBlank-out.
- [Sega VDP2 §2.5](https://www.infochunk.com/saturn/segahtml_en/hard/vdp2/hon/p02_50.htm): software HV latch, counter encoding and DISP=0 behavior.
- [SH7604 hardware manual §11.3](https://antime.kapsi.fi/sega/files/sh7604.pdf): byte-high/byte-low FRT read ordering and shared temporary register.
