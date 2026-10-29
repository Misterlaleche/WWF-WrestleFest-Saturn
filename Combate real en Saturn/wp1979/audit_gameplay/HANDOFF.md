# wp1979 final gameplay handoff — frozen ROM revision 4

The final input/assets below use the pinned native runtime plus the demonstrated CPU and human corrections. **One continuous match completes at logic frame 7372, with real legal TAG exchanges by both teams and a human-team win.** All requested wrestlers become legal participants. `native/`, `reference_r18/` and all repositories remain untouched by this audit.

## Exact files to integrate

| File | Use |
|---|---|
| `fixes/state6/src/wf_cpu_runtime.c` | Final CPU source, including state6 live-register/return correction |
| `fixes/src/wf_r17_human_raw23.c` | Final human source, local EA42 register correction |
| `fixes/state6/proposed-combined-v4.patch` | Both TUs against pinned original source |
| `fixes/state6/proposed-state6.patch` | Incremental CPU change if v3 is already installed |
| `host/tag-rom-v4-input.csv` | Frozen ordinary-input tape,1761 changes |
| `assets/HOTSPR-ROM-v4-32.BIN` | Disc HOTSPR.BIN,6357 exact tiles |
| `assets/wp1979_rom_hot_pages_both_tags.h` | Unchanged211-page hot header |
| `host/tag-rom-v4-events.csv` | Logic-frame TAG/result timeline |
| `host/tag-rom-v4-resident.log` and matching roles CSV | Complete replay/residency evidence |
| `assets/wp1979-gameplay-manifest.json` | Final source/file hashes, counters and limits |
| `fixes/state6/STATE6-CAUSAL.md` | Exact6093 cause, instruction bytes and red/green proof |

The frozen original route paths `fixes/state6/routes/tag-after0/input.csv` and `HOTSPR-32.BIN` are byte-identical to the delivery copies. They will not be overwritten. Earlier v2 and v3 route/source artifacts are preserved separately.

## Native launch and match

Existing production `wf_r18_direct_boot`: human 0, partner 1, cabinet 0300, native seed 1085 after software register reset. Actor slots 0/1/4/5 are Hulk Hogan/Ultimate Warrior/Ted DiBiase/Mr. Perfect. Initial stamina 110/106/145/130. No post-bootstrap actor edits, forced results, teleports, scripted animation or RNG reseeding.

| Exchange | Outgoing TAG starts | Legal role swaps | Entry action ends | Nominal legal-swap time |
|---|---:|---:|---:|---:|
| Hogan→Warrior |476|529|556|8.816667s|
| DiBiase→Perfect |5715|5768|5795|96.133333s|

Final pin counts6994/7082/7170; original endpoint `$11B6` at7372; results4001/4000/8000/8000; `$169A=0300`, `$16A0=0015`. Original result presentation waits131 VBlanks. All four actors remain present for all 7372 logic steps. The generated policy and frozen CSV replay have byte-identical complete role/position/health traces.

A TAG requires the original `tag_exchange_188bc` exchange of `+33.bit0`. Presence `+00.bit7`, temporary human control `+33.bit1`, reserve role `+56.bit6` and apron mode `+32.bit0` are tracked separately. Rescue interventions do not count as TAGs. The human request is an ordinary action-button edge near the native corner with a ready partner; outgoing/entering raw actions are4C/4D.

## Residency and code identity

Final atlas:6357 tile IDs,19720 unique32-byte chunks,50856 map bytes padded50880,631040 dictionary bytes,16384 header bytes;698304 total file bytes. LWRAM631040 bytes leaves150 fallback tile slots. Every tile reconstructs byte-for-byte to canonical SPRITES.PAK. Same211 distinct metadata pages in209 main +14 extra slots. Complete selected replay: **0 post-boot ROM reads,0 sprite reads,0 blocked owners**. Residency is certified for this tape; arbitrary manual-action cache coverage is not exhaustive.

The game remains native C.114 production TUs are linked, with two changed source files. ROM supplies immutable original tables and graphics metadata. A 68000 engine is used only by the independent host test oracle and is not linked into the Saturn program.

## Demonstrated corrections and tests

CPU1CCB4 copies the exact tagged partner pointer, setsB7.6 and selects state 5/raw 3D. Two1CFB4 branches call the existing1C5FA helper and obey original carry. HumanEA42 supplies the original local D3=0 fromDEB2 while preserving the saved D0–D3 context. The downstream1C094 invalid-link guard remains intact.

CPUstate6 uses live D2–D7 at original1EDFE/24CC, and returns at1EE36 rather than advancing state 5 early. The observed6093 fixture proves the causal RNG difference: original2F50D97B/B5=03; v3 A2397F08/B5=13. Captured legacy register cache values were zero. The final proposal matches original RAM, RNG and all eight D registers.

Independent evidence:50 earlier CPU/action ROM cases plus4 retained guard cases;33 humanEA42 RAM/RNG/register cases;149 state6 cases (v3 red136 failures →v4 green149 passes). Fresh984 complete original phases from native snapshots match every byte of all four actor records in984/984 cases. All three original ordinary-input regressions pass beyond their former guards. The old6127 input completes at9015 with a human-team loss. Full canonical fixture/instruction evidence is retained under `fixes/`.

This closes the identified CPU TAG, EA42 and state6 families. It is not a claim of complete ROM match equivalence, exhaustive manual-input support or physical Saturn validation. Native rendering cadence, audio and final BIN/CUE measurements belong to the parent integration. Nominal times above are logic frame/60, not measured video timestamps.

## Historical versions

v2 was the9980-frame tape before EA42 correction; v3 was the10675-frame tape before state6 correction. Their original paths remain unchanged, with manifests and reports archived under `handoff/pre-correction-v2/` and `handoff/pre-state6-v3/`. The v3 tape on final code reaches14409 with results8001/8000/8000/8000; it is not the selected demonstration. Use only the source/input/atlas tuple in this revision 4 handoff for final measurement.

## Reproduction

The shared workspace supplies canonical source/assets, existing host objects and test-only oracle engine. Rebuild the final harness with `python3 wp1979/audit_gameplay/fixes/state6/build.py`. The tests and exact environment-driven replay commands are included in `fixes/state6/STATE6-CAUSAL.md` and `fixes/state6/verify_final.py`. This audit bundle is not a standalone Saturn distribution; the parent packages the complete native project and BIN/CUE.

## Frozen SHA-256

- CPU source: `0d921d63c76829c39e45c6487717fd5ee346eed3a541c4a724f3ab8fcdcf2e86`
- Human source: `eb35cc13ea8e548919eda614482169ced7ea23a76ceffd410550a21cf5f8bb20`
- Combined patch: `15ab9e2b7f46909c0c23001a72ec35a017f9648c3dd4ffa636306ed9026d5166`
- Input: `1095900ede97757e497e66ac2a9c6f103782b399c5d41168acc3f2ff72ef7b3f`
- HOTSPR: `1c28c71853c146dc11d514ee1c642fc7068675ec18eea772201ca37e28292ec6`
- ROM-page header: `5b1736d94cc2be346e3f089dc0e6f65c4b7cb82e6b3dd2e161b6d69e30096208`
- Role trace: `7c1724e5badd5717ed2633f1ac7c1a349098a80b4125ad81e6ee64317c97a31e`
