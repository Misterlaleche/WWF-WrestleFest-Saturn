Raw3E / callback1831A, native Stage7b human TAG frame1230
======================================================

Authority: intact ROM SHA1a726c46f350723ea73158bc78055d17d418b14e2.
12614[3E] points to kind-zero18314/callback1831A. The callback always replaces
the initial descriptor with one of the original walk descriptors selected
by11710:115F2,1160A,11622,1163A. Existing ring_walk_descriptor is reused
unchanged. No other action, common speed helper, CPU, entry or input edits.

Literal scope
On first entry only: motion BYTE1=1, angle WORD2C=0040 or00C0 from facing
BYTE2E.bit7. Every quantum then runs1174C: character WORD2 selects byte at
116AE or116BA (FLAGS32.bit3); ROLE56.bit7 plusFLAGS33.bit5 adds six in BYTE
arithmetic; REL74.bit7 subtracts eight in WORD arithmetic.11710 selects the
walking descriptor, and writes BYTE1B=10 on the REL74.bit7 branch. Finally
BYTEFE.bit7 clears STATE WORD20 and sets LINK BYTE26.bit7. It leaves motion
and selected walking descriptor intact. No LINK/TARGET/partner pointer,
audio, RNG, collision, outcome or other pending owner is consumed.

Guard contract: supported character WORD0..11. Other table indexes remain
fatal1176A after first-entry motion/angle, D0.W=character and magnitudeWORD2A
clear. Prefixes are compared against the original ROM stopped before its
first unowned speed-byte read. This does not claim arbitrary ROM indexes.
Unconsumed invalid pointers are permitted, including on the terminal branch.

Exact capture
action3e_capture_beetle.py reads the already-stopped Beetle capture only:
validation/local-20261024/hardware-input/beetle-full-flow-stage7b-human-tag.
It checks program/BIN/CUE against pinned Stage7b files and _rt ELF symbol,
byte-swaps the2MiB WRAM representation, verifies all9actor bytes at one
unique WRAM address and all8D registers at the exact shadow offset. Fresh
Stage7b SH2 layout384/268/352/7340/0 and frozen runtime/actor headers pin the
pointer-free scalar prefix. No HOST pointer-tail offset is used. Supported
globals and actors are projected into16KiB arcade RAM; other bytes are zero.
This is emulator evidence, not a physical Saturn dump. No state, identities,
HP, seed, frame, registers, or result is forced in the capture.

Snapshot SHA256:
e7c37e1e3cc7c9bdcb27e9b1ee7d27321b1aa5074aebe83d7c47eeb0ac5709e4
Manifest records report, WRAM, linked program, BIN/CUE, ELF/map, frozen source
manifest, SH2 layout, core and input schedule hashes, plus precise addresses.

TDD and verification
action3e-red.log: natural1230 complete original1C03E returns, old model blocks
12614. action3e-green.json/log:6557 complete original1C03E calls, no owner
intercepts. Every call compares RAM16KiB,FG16KiB,D0-D7,RNG/globals and the
selected tick descriptor. Original1174C,11710 and common animation all run.
ROM pages are read-only mapped and verified unchanged.
3456 speed/modifier/init/latch/facing cases;2016 descriptor/counter/FE-width
cases;108 selector/all9slot/terminal cases;16 unused-pointer cases; natural
capture;960quanta in12continuous80-call loops with live D0-D7/RNG.
21guards:20character boundaries with RAM/FG/D prefix comparison, plus an
unknown raw35 selector. WORD IDs00FF/0100/8000/FFFF cannot alias validBYTE IDs.
All walking descriptor constants and speed bytes are asserted from the ROM.
Coverage assertions enforce each callback/helper branch and loop tick path.

Raw3E block-plus-case SHA256:
28785b60187d61e506a4b0c48e5aa1d0e096117082f02bb6f401c54fc5009960
Full source and include/generated hashes are recorded by the test. The
production owner is frozen in validation/local-20261024/action3e-owner-frozen.txt.

Run from runtime18:
  . ../toolchains/host-env.ps1
  & $taskPython src/runtime/tests/r18/action3e_capture_beetle.py
  & $taskPython src/runtime/tests/r18/action3e_verify_rom.py --report validation/local-20261024/action3e-green.json

Limits: original stack/A/CCR lie outside the typed combat ABI. This validates
the bounded action owner, not full arcade RAM, engine trajectory, physical
timing, an entire match, or global ROM equivalence. No global probe was run.
