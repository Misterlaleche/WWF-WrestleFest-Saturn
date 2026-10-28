Raw20/16A16, 2026-10-24

Reproduce from runtime18 after sourcing ../toolchains/host-env.ps1:
  & $taskPython src/runtime/tests/r18/action20_capture_host.py
  & $taskPython src/runtime/tests/r18/action20_verify_rom.py --report NEW.json
Capture reads existing HOST5845 evidence; it does not run another fight.
It validates exact all9actor bytes, D0-D7, sizeof7664/offset384/7340 and the
reused binary SHA against the stage9-final frozen source manifest.

Raw20 table12614 selects16A06/16A16, kind1, durations6/4, posesAD/AE.
The entry BTST is followed by a literal NOP16A1C, so the callback executes
10FC6 even on initial entry. Carry sets state6. Otherwise profile4C=11,
WORD2A-=2; BPL tests the resulting N flag, including signed overflow cases.
A negative result sets state0, translates8 via real10BD0 with facing and
sets bit7 of BYTE26, preserving the original raw pointer tag behavior.
There is no linked-actor read or new unrepresented dependency in this owner.

Natural HOST5845 actor5/ID3 selector20 RED12614 retained in action20-red.log.
GREEN2422 whole intact1C03E cases +1 unknown-selector guard in action20-green.
Full rawRAM16KiB, FG16KiB, D0-D7, globals/RNG and original descriptor compared.
No ROM helper/instruction interception. Initial/latch/WORD-wrap/N-vs-V gates,
rope contact priorities and all9actor slots are represented. Nine continuous
five-call tests prove the countdown reaches its literal negative transition.
No fabricated HP/result/RNG/input changes; opaque RAM is zero only in the
declared typed HOST capture, never substituted into production gameplay.

Implementation is the separate private wf_r18_action20.inc, integrated by
the shared actions owner. Independent review is retained separately.
These callback tests do not assert a full native win or physical timing.
