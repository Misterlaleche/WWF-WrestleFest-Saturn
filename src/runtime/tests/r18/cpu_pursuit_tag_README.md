# Pursuit TAG request connection

The production change connects the existing exact1C5FA helper to the1D5A2 pursuit entry. The ROM calls this helper before10BE8: a reserve sets state1/AE4 and returns without reading either actor relation; an active actor transfers the request and continues pursuit.

The natural pre-owner fixture is HOST idle frame1148/slot4/AE10, `full-flow-combat-after53-idle/snapshot-00365.bin`. All nine actor records are copied literally from that trace; supported globals are projected and other arcade RAM bytes are zero. The manifest pins its identity and source report. It is not a physical RAM capture.

RED: `validation/local-20261024/cpu-pursuit-tag-red.log` stops at guard1C5FA. GREEN: `cpu-pursuit-tag-green.json/log` passes1483 cases plus6 retained pointer guards, comparing full RAM8KiB, FG16KiB, D0-D7 and RNG against intact1C8F4→1D5A2→1C5FA and existing pursuit continuations. No owner is intercepted and no RNG call occurs.

The matrix covers slots0/4/8, active/reserve side bits, request BYTEflags, highWORD AE bits, selector1C/69, modes0/1/2/FE, self/peer/tagged team links, and invalid relations which the reserve does not consume. Active invalid partner reads retain1C608; invalid consumed targets retain1D5B2. Other pending1C5FA caller guards remain.

Run from `runtime18` after loading the host environment:

```
& $taskPython src/runtime/tests/r18/cpu_pursuit_tag_verify_rom.py --report validation/local-20261024/cpu-pursuit-tag-green.json
```

The test freezes sources before compilation and asserts the literal ROM branch sites. CCR/A registers remain outside the typed combat ABI. This proof covers the request connection and admitted continuations, not whole combat or hardware timing.
