Raw87 reversal1AA9A, 2026-10-24

From runtime18 after sourcing ../toolchains/host-env.ps1:
  & $taskPython src/runtime/tests/r18/action87_verify_rom.py --report REPORT.json
Capture reproduction (existing files only, no combat execution):
  & $taskPython src/runtime/tests/r18/action8d_capture_host.py

Natural HOST7493/actor1 reached raw87 with ordinary policy3,4,2,24,4,0,1.
The capture is a typed projection checked byte-for-byte against all9actors,
D0-D7 and HOST layout7664/384/7340. Opaque arcade RAM is not reconstructed.
Snapshot SHA740176b8a45e2a7f3cbcced7e1942ea6417ded3624f45a2e92b79954efa8eaf8.

ROM12614[87] selects1AA8A, kind2/two frames: durations18/10, poses144/145.
1AA9A initial entry translates16 through real10BD0, then sets WORD44=72.
Every callback decrements44 independently of animation progress; zero
releases the pair using exact A0/A2 store order. Both facing BYTE stores,
raw tagged A2 in LONG92, Z+=72, flags and self-alias effects are preserved.
This owner does not invent damage, health, winner, input or RNG effects.

At the first unrepresented target write1AACA, guard preserves prior stores:
counter44=0, message15D2=0F1A, own bit18.7 and live D0 selector87*4. Other
invalid links remain unconsumed while the countdown is nonzero. Unknown
selector still reports12614. The ROM's null-A2 substitution at1C0A0 does
not make its unrepresented target an invented actor in the native model.

RED action87-red.log: first unsupported raw87 returns12614.
Initial GREEN attempt caught D0 prefix omission after2305 full comparisons;
whole1C03E leaves D0.W=21C before1AA9A. Added that literal shadow write.
action87-final-pinned.json/log:2306 whole1C03E comparisons +31 retained guards
(30 compare original error prefixes) PASS. Natural fixture is mandatory.
Comparison: full rawRAM16KiB, FG16KiB, D0-D7, globals/RNG, tick descriptor.
Original ROM remains mapped read-only; no instruction/helper interception.
Six continuous72-call cases cover the complete two-actor reversal.

Native include SHA a7ca5213e512283dc1ea939d1fa11ea2ff493d3ad0a30c6dc2d813bc603b249a.
Independent review follows separately. These unit/fixture comparisons are
not a full native Saturn victory or physical60Hz timing qualification.
