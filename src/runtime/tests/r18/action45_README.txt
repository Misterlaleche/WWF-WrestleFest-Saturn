Raw45 / raw01, shared original owner128FA
=======================================

Natural evidence: cold frontend TAG picks0/7 CPU6/3, paired-rush HOST tape,
frame1407 slot1/id7 selector0045. action45_capture_host.py copies the exact
pre-1C03E snapshot00387, verifies all nine actor bytes and D0-D7 against the
blocked dump, and records source/report/trace hashes in action45_manifest.json.
The input profile and seed belong to that source run. No actor/register/state,
HP, winner, or result is forced by this port. This is HOST evidence, not a
physical Saturn capture. Bytes absent from the typed projection remain zero.

Intact ROM SHA1 a726c46f350723ea73158bc78055d17d418b14e2 has identical LONG
entries at12614[01] and12614[45]: descriptor128DE, callback128FA. Therefore both
selectors call one raw01_45 function; no duplicate gameplay implementation.
The callback uses latch BYTE1C.bit7, prior BYTE1F, frame BYTE25, timer WORD22,
Z WORD0E, and exact 10FC6/profile/contact/FE ordering. Tick descriptor128DE
is retained for every branch. Initial trajectory26AE alone consumes A2.

TDD RED: action45-red.log is the natural12614 guard. Separately,
action01-45-prefix-red.log proves the old raw01 implementation left D0.W=4
at the12938 call boundary; ROM has already CLR.W D0 there. The shared owner
now supplies selector BYTE*4 (4/114), materializes callback MOVE.W values,
and retains CLR.W D0 plus VZ0700/DEC0060 before an unsupported26AE call.
Successful outer1C03E still restores the original D0 through existing MOVEM.

Final GREEN: validation/local-20261024/action45-final.json/log:
  6349 complete unchanged1C03E ROM calls, no intercepts;
  all RAM1C0000..1C3FFF, FG16KiB, D0-D7, RNG/globals, tick descriptor;
  nine retained guards, eight ROM prefix checkpoints at12938;
  32 continuous callback sequences, 1120 quanta with live D0-D7;
  full low-BYTE phase versus WORD timer widths, high selector bytes,
  both facings, aliases/self links, unsigned height/offset borders,
  rope-before-profile/contact ordering, priorBYTE preset27/28, init/latch/FE.
Original raw01 suite also passes5085 calls plus five guards in
validation/local-20261024/action01-after45.json/log.
Continuous sequences apply the same declared Z/contact inputs to both models;
they do not claim a full physics simulation. ROM is read-only mapped and
verified unchanged. Consumed invalid links and signed-division-overflow paths
remain guarded26AE. Their prefix is checked before the original helper call;
no claim is made about unimplemented DIVS-overflow behavior. Unused links do
not cause a guard. Raw35 remains12614 for the negative selector fixture.

Shared owner SHA256 (callback block plus both admission cases):
ccda01f742d339e7ba7f3ebf9c22a506493b30954ea76159af60de5770864b16
Frozen owner: validation/local-20261024/action01-45-owner-frozen.txt.
Test report freezes the full production/include/generated source hashes.

Run from runtime18 with the bundled HOST environment:
  . ../toolchains/host-env.ps1
  & $taskPython src/runtime/tests/r18/action45_verify_rom.py --report validation/local-20261024/action45-final.json
  & $taskPython src/runtime/tests/r18/action01_verify_rom.py --report validation/local-20261024/action01-after45.json

The typed combat ABI does not expose CCR, A registers, original stack, or
sound-device IO. No hardware timing, global ROM exactness, full match flow,
or equality between HOST and SH2 trajectories is claimed. Entry, controller
sampling, shared helpers, HP/results, and other action owners are unchanged.
