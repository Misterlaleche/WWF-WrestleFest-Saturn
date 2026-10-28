CPU1D398/1D3F4 physical-null partner link, 2026-10-24

Run from runtime18 after . ../toolchains/host-env.ps1:
  & $taskPython src/runtime/tests/r18/cpu_1d3f4_capture_host.py
  & $taskPython src/runtime/tests/r18/cpu_1d3f4_verify_rom.py --report REPORT.json
No capture command runs combat, an emulator or a new input policy.

Natural evidence: human-tag-input-correct-sweep/policy-003, frame284,
phase1C150, actor1 ID7, state8001/AF1. Its partner actor0 LINK26=80000000.
The fixture contains the actual blocked state, including already executed
owner prefix; no earlier state/registers are fabricated. The unit test then
replays whole1C8F4 on that state in both implementations. This is not a claim
that the blocked dump precedes the original first1C8F4 instruction.
Capture compares all9actors and D0-D7 byte for byte with individual dumps
and their offsets within the7664-byte HOST runtime, then exports known raw
scalars/records. Other arcadeRAM bytes are zero. The snapshot is32 native
endian D bytes followed by rawRAM16KiB. Manifest pins every dump/source hash.

ROM1D3F4 loads raw LONG26(partner) toA2.1D3F8 reads BYTE21(A2) over the24-bit
bus.80000000 therefore reads canonical ROM21=FF, compares unequal to5 and
continues1D428. The previous native raw-LONG nonzero guard rejected this.
Only physical-null and real actor links are supported; other ROM/RAM or
interior pointers keep1D3F4. No generic bus reader or default target added.

The same owner's matrix proved three further literal corrections:
-1D3F8 and1D43A compare full BYTE21, without the main-state mask7F.85 is not5,
  and80/81/84 are not0/1/4. Only these two local reads changed.
-1D49E MOVEQ0,D0 occurs before the progression dependency at1D4A0.
  Supported table reads then supply progression<<2 before the real24CC call.
-1D4BE MOVE.L A1,+7A preserves the original raw LONG/tag from partner7A.
  The existing model had rebuilt a canonical actor pointer.
No shared RNG, state5, resolver, action or input helper changed.

Final2479 whole intact1C8F4 calls and25 prefix guards PASS:
RAM16KiB, FG16KiB, D0-D7 and RNG/globals compared, no intercepts/ROM patches.
Includes1810 actual1D3F8 instructions reading real ROM21 with physicalA2=0;
all256 partner/null tags;128 owner-highBYTE inactive values; BYTEY-distance
threshold8 versus WORDX threshold64, unsigned wraps; linked state/selector
BYTE cases; all10 progression rows and all three live RNG choices. Choice0
executes original downstream state5 raw39; choice1 executes raw1C when its
original LONG target-link test iszero, and skips even tagged physical-null
when that LONG is nonzero. Existing callbacks are executed, not intercepted.
25 guards compare original prefixes at1D3F4,1D3AA,1D436,1D4A0 and24CC.
CORE test_runtime17_f4c2 passes with assertions enabled.

Artifacts: validation/local-20261024/cpu-1d3f4-{RED,BYTE-state-RED,
progression-prefix-RED,target-tag-RED,final}.log; final.json; before sources.
SourceSHA25616cd9c4ce1028d9663d3015ec477bfa05ca106c8911e7b40c93ce96e6b591f14.
SnapshotSHA256d15b776c5993fe109c3d184c63b4b5b30c9ca7837d0659d275243dc13f25e95f.

Unrepresented partner/target addresses and progression outside0..9 retain
guards, as does missing live RNG. Original A/stack/CCR and physical timing
remain outside typed ABI. No global combat probe, new trajectory, result
change, completed WIN, hardware validation or global ROM-exactness claim.
