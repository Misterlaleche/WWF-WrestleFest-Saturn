# State4 callback $1B7A2

`state4_1b7a2_verify_rom.py` compares the native dispatcher and callback with the intact arcade ROM (SHA1 `a726c46f350723ea73158bc78055d17d418b14e2`). Run from `runtime18` after loading `../toolchains/host-env.ps1`:

```
& $taskPython src/runtime/tests/r18/state4_1b7a2_verify_rom.py --report validation/local-20261024/state4-1b7a2-green.json
```

The natural fixture is the pre-dispatch HOST frame427/slot0 capture from `full-flow-combat-after27-41-a600/snapshot-00116.bin`. Its header, bytes and provenance are pinned in the manifest. The actor records are copied literally from the trace; supported globals are projected into arcade RAM and other bytes are zero. It is not a physical Saturn or Beetle RAM capture.

RED (`state4-1b7a2-red.log`) stops at the pending callback. GREEN (`state4-1b7a2-green.json` and `.log`) passes 5604 cases and five explicit pointer guards. The full test freezes the production sources before compiling. It compares all RAM8KiB, FG16KiB, D0-D7, RNG and the selected animation descriptor, with no ROM intercepts.

The matrix covers BYTE/WORD latch/index/prior/subtype distinctions, counters including FF00 and underflow, facing and coordinate wrap, self/peer/tagged aliases, unread invalid relations, initialization and both terminal branches. Direct callback fixtures use the original JSR/MOVEM return stack and execute the intact common dispatcher tail; this exposes the callback-local descriptor1B796 even though the normal subtype14 dispatch selects1B78A. Required branch sites are asserted.

Unknown initial relation pointers retain guard1B7B6 after the original prefix writes. Other pending owners retain their guards. CCR and A-registers are outside the existing typed combat ABI. This evidence establishes only this owner and its tested tails, not global combat or hardware equivalence.
