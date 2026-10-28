RAW54 $18F62/$18F82 -- bounded ROM-derived action owner

Production: src/wf_r18_action54.inc, included after the shared action helpers.
Inventory added include/case54 to wf_r17_actions_r11.c; this owner did not edit
that shared source, inputs, entry, CPU, human_raw23 or any combat probe.

Authority: intact ROM68K.BIN SHA1 a726c46f350723ea73158bc78055d17d418b14e2.
12614[54] is 00018F62. Both descriptors point to 18F82, kind1/count2,
durations FF00/0020; poses 01C0/01C0 (18F62) or 0220/0220 (18F72).

Contract:
- Initial BYTE1C.bit7 clear: CLR.B01, then select by BYTE65==9.
- Latched BYTE25==FE: state WORD7, clear WORD64, set BYTE26.bit7;
  retain descriptor18F62 for this call even though WORD64 was cleared.
- Otherwise only WORD24==0 consumes A2's BYTE21. Exact BYTE4 clears own
  WORD22, places linked actor at own X+/-64,Y-1,Z+20 via original10B9A,
  writes linked WORD68=5, and calls sound2052 with D0.W=50.
- Other continuing paths select primary only when BYTE65==9.
- D1/D2 low WORDs become FFFF/0014 only on the placement branch; upper
  halves survive. Whole1C03E restores its saved D0 after a successful call.
- Invalid nonactor links block only when consumed at18FB0. Initial,
  terminal and nonzero-WORD-phase paths do not eagerly validate LINK.
  Literal null uses dispatch dummy7F000 whose BYTE21 is50; tagged physical
  zero reads ROM21=FF. Both fail the BYTE4 comparison without a write.

Evidence:
validation/local-20261024/action54-functional-red.log: natural HOST6724
blocked at12614 before the owner existed. An earlier capture-harness error
referenced nonexistent wf_world.h; corrected before this functional RED.
action54-green.json: initial1723+25, private frozen-only wiring.
action54-final.json/.log: final4310+73, actual shared include/case integration,
private_test_wiring=false. Callback coverage26/26 PCs; 10B9A coverage13/13.
Includes all256 BYTE states x9 actors, all256 latch bytes, selector high
bytes, BYTE-vs-WORD phase/subtype, self/tagged aliases, fractional preservation,
WORD wraps, FF00 timer, both descriptors, terminal tail, real2052 branches,
72 consumed-link guards with complete RAM/D8 prefix and retained raw35 guard.
The original full1C03E,10B9A,2052 and tick execute without ROM interception.
Checks compare raw16KB RAM, FG16KB, D0-D7 including RNG/globals, descriptor,
and immutable ROM. The typed runtime does not expose A registers or CCR;
no claim of those comparisons. Native audio device writes/PCM are outside
this action state API; the oracle executes2052, including its D0 save/restore.

Capture: existing human-tag-stage9-more-actions/policy-003 dump only, no new
combat execution. Snapshot SHA256289444ca5c7924a3fd2f6661a3edd0251855516c4b0c96f9f2d1633ece8fb056.
All9actors and D0-D7 match the original dump byte-for-byte. Actor1 ID7,
HP1,state8005,selector0054,LINK001C0AEC,frame6724. HOST layout7664/384/7340
is checked by C sizeof/offsetof and producer header hashes. Remaining typed
fields are exported over zero raw RAM; no A-register capture is invented.

Reproduce from workspace root in PowerShell:
  . ./toolchains/host-env.ps1
  $env:PYTHONDONTWRITEBYTECODE='1'
  & $taskPython runtime18/src/runtime/tests/r18/action54_capture_host.py
  & $taskPython runtime18/src/runtime/tests/r18/action54_verify_rom.py --report runtime18/validation/local-20261024/action54-rerun.json
Native isolated C compilation/oracle execution requires the authorized
escalated runner. Tests freeze source before compiling; assertions remain on
via -UNDEBUG. No global combat or native Saturn replay was run for this owner.

Frozen evidence and exact hashes:
validation/local-20261024/action54-research/final-manifest.json
validation/local-20261024/action54-research/frozen-sources.zip
Owner a77625e530ed8d75cb00bd086c963660a0c45423bb08938c25e65bced96d721c.
Composed source AT FINAL GREEN (shared source continues changing under other
owners): c1895930073de4aa85ca788db89974cfb2e3f6205a800c936e805bf3fdbc9d5f.
Archive be5c021c4bcea1038763677c2183c7701d60d8e15c37765ab69930e79c4934e1.
