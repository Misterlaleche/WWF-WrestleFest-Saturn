# New-chat handoff — WWF WrestleFest Saturn

Use this private repository and persistent ChatGPT Library as source of truth.

## Start here

1. Read `PROJECT-STATE.md` and `docs/HARDWARE-TEST100-FEEDBACK.md`.
2. Current **validated canonical** source snapshot:
   - `/WWF-Saturn/WWF-Saturn-Test140-V0155-source.zip`
   - SHA256 `4f30e2d92545a263f206f72564562c301b5eaebd55d4d297f84d5326ca6aa8bf`
3. Current **merged WIP continuation** carrying Test141-143 on top of canonical Test140:
   - `/WWF-Saturn/WWF-Saturn-Test143-MERGED-WIP-from-Test140.zip`
   - SHA256 `f8513c20569f909fcd2b4cc56de52d0a1709dc2554a62cc60d6afe17cc831e1f`
   - IMPORTANT: this WIP is deliberately not canonical yet. The changed Test141/142 paths and canonical Test138-140 guards pass normal plus ASan/UBSan targeted builds, but the full host suite exceeded this runtime execution window and SH-2/Yaul is not installed here. Do not call it a Saturn build or canonical checkpoint until full host regression and SH-2 compile/link are rerun.
4. The older `/WWF-Saturn/WWF-Saturn-Test143-WIP-from-Test137.zip` is superseded by the merged WIP above and must not be used as the base.
5. Every Test must implement or rigorously validate a real function. Preserve ROM/manual/video evidence labels; anything unproven stays PROVISIONAL or REGISTERED BUT NOT IMPLEMENTED.
6. Test100 remains the latest hardware BIN/CUE and is confirmed working on real NTSC-U Saturn + Terraonion MODE. Do not generate another BIN/CUE routinely.

## Canonical state through Test140

- **Test140/V0.155** canonical.
- Full host regression: **136/136 PASS** at validation time.
- Tests138-140 targeted ASan/UBSan PASS.
- Test138: exact ROM `$23856` wrestler-indexed 12x3 stamina-tier weighted rows.
- Test139: exact ROM `$1EE38` ten-slot 10x3 stamina-tier weighted rows.
- Test140: exact low-stamina warning boundary: active actor required and `+$66 < $0018`; stamina 24 is not low, 23 and below are.
- Earlier Test134-137 work remains integrated: 3-point arcade Punch damage, live submission rapid-button prompt, exact visible ground-drop descriptors, CPU PICK UP action `$22/$15B6C`.

## Test141-143 merged WIP

- Test141: `ST_RISE` rendering uses recovered arcade descriptors `$181FC/$181EC` for the first 26 ticks by actor role, then `$11E36` for the final 17 ticks, matching the recovered 13+13+17 rise timing. Targeted normal and ASan/UBSan PASS.
- Test142: native TAG/Rumble post-result staging is tied to resolved winner/loser ownership. Ownership is exact; Saturn coordinates/speed remain PROVISIONAL. Targeted normal and ASan/UBSan PASS.
- Test143: result rendering keeps the ring/post-match staging visible instead of covering the centre with the old large plate. Exact arcade post-match typography/layout remains PROVISIONAL.
- Merge rule: retain canonical Test138-140 tables/boundaries from Test140. Do not reintroduce the older WIP `arcade_ai_stamina_bucket()` branch in place of those canonical implementations unless later ROM evidence proves it belongs as a separate mechanism.

## Immediate continuation

First close the merged Test141-143 WIP: run full host regression in bounded batches, run targeted sanitizers, and perform SH-2 compile/link with Yaul. Only then update metadata/package integrity and promote Test143 to canonical.

After that continue these research fronts:

1. Recover exact arcade victory/defeat/winner clap or celebration descriptors, positions and timing for TAG/Rumble; keep current staging provisional until demonstrated.
2. Continue decoding the semantic consumers of the exact `$23856` and `$1EE38` stamina-tier AI tables. Do not invent fatigue aggression/cadence behavior.
3. Exact TAG apron placement/entry/exit animation and illegal-partner behavior.
4. Missing roll/get-up frames, ground policy probabilities, additional Rumble eliminations, Hawk/Animal assets, records, attract/promo scenes and audio.

## Long target

Continue concrete checkpoints toward Test200. At Test200 report exactly:
- IMPLEMENTADO
- REGISTRADO PERO NO IMPLEMENTADO
- PROVISIONAL
- PENDIENTE DE BUILD SATURN
