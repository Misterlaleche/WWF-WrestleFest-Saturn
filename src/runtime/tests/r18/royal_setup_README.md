# Royal setup `$1086C`

`wf_r18_royal_setup_1086c(wf_r18_prematch_t *)` ports the complete Royal branch
through its original RTS `$10ACA`, using the same 16 KB big-endian RAM and live
D0-D7/A0-A6 context as the prematch module. It preserves the incoming D0-D3 and
A0-A2 on success, while retaining the original live outputs in D4-D7/A3-A6.
Only original palette allocation `$2AEA` is delegated to the effect callback.
No main reset, seed replacement, generic roster, identity strip or TAG setup is
used. Failed dependencies retain their executed prefix and require stopping or
restoring a checkpoint before retry.

For WORD `$1C007C=0`, the original six existing records receive table indices
5→0 as D0 counts down; no participant RNG or identity replacement occurs. For
nonzero credited WORD, the owner preserves the active primary participants,
marks the two occupancy windows, fills three identities in each six-ID half
with exact `$24CC/$21B4` draws, and uses first-free `$10AF0/$10B26` after four
occupied choices. The RNG consumes the existing RAM seed, RAM frame counter
and live data registers. Generated CPU records start at actor4 and receive the
remaining anchors in ascending D7 order. The original `$10782` Royal/human
health rules and `$10718` referee setup are implemented locally, including the
five palette requests in their original order.

Coordinates are machine words, not screen pixels. Fractional coordinates,
reserved flags, unrelated RAM and pre-existing identity/occupancy state remain
inputs. Guards retain unsupported non-Royal entry `$1034A`, unknown IDs,
overfull halves, exhausted occupancy and out-of-window anchor requests. A live
participant topology must fit three humans per identity half and the original
six-anchor window. This module does not own the C98 caller, frontend IRQ,
presentation cursor, native palette backend or physical Saturn timing.

The intact-ROM differential passed 1,826 fixtures and 12 guard/error controls.
It compares all 16 KB RAM, 16 KB FG, 36 KB palette aperture including padding,
D0-D7/A0-A6, seed, MOVEM preservation, and the ordered context of every palette
request. `$2AEA` executes its complete unchanged ROM code in the native test
bridge; no owner, memory operation, register or RNG result is intercepted.
Both first-free fallbacks occur naturally in the matrix. Coverage also requires
four rejected RNG samples at `$24E8` and zero-sample normalization at `$24F2`.

Evidence: `validation/local-20261024/royal-setup-red.log`,
`royal-setup-final-green.json/log` and `royal-setup-controls-green.json/log`.
Research authority is the intact ROM and isolated WP12 package recorded in
`royal_setup_manifest.json`. No entry or Makefile was edited, and no global
project suite, composed SH2 build, or active VM was run by this owner task.

Run from runtime18 after `. ../toolchains/host-env.ps1`:

```powershell
& $taskPython src/runtime/tests/r18/royal_setup_verify_rom.py --report validation/local-20261024/royal-setup-final-green.json
& $taskPython src/runtime/tests/r18/royal_setup_verify_rom.py --controls-only --report validation/local-20261024/royal-setup-controls-green.json
& $taskPython src/runtime/tests/r18/royal_setup_verify_rom.py --smoke --pending
```
