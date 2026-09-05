# New-chat handoff — WWF WrestleFest Saturn

Use this private repository and persistent ChatGPT Library as source of truth.

## Start here

1. Read `PROJECT-STATE.md`, `docs/HARDWARE-TEST100-FEEDBACK.md`, `docs/TEST138-144-CLOSURE.md` and `docs/HARDWARE-TEST144-CHECKLIST.md`.
2. Current **validated canonical** source snapshot:
   - `/WWF-Saturn/WWF-Saturn-Test144-V0159-source.zip`
   - SHA256 `6ea546f185d71951596ca19419c7fbd50a2def0d24e0346ec91ca36350960eee`
3. Test144 hardware-validation package:
   - `/WWF-Saturn/WWF-WrestleFest-Test144-V0159-BIN-CUE.zip`
   - SHA256 `b61ad9a53a3d159dbac9c34e4111f60f2aa6b11af5b98706fcd9f5c83f42646e`
   - Raw BIN SHA256 `7610c9dfc015baa0573742286f2381d5b7b63bd2ccccfcb61844bd3c5199ac46`
   - CUE SHA256 `bf558f9489edcd0809da6f884beddb070b1d4896eb7de4164d65f02c3400b1cd`
4. **Hardware status:** Test144 build/sector checks pass but real Saturn validation is pending. Test100/V0.115 remains the latest image confirmed on NTSC-U Saturn + Terraonion MODE until Test144 is physically tested.
5. Continue from **Test145/V0.160**. Do not resume from older snapshots.
6. Every Test must implement or rigorously validate a real function. Preserve evidence labels: **ROM-EXACT**, **PROVISIONAL**, **REGISTERED BUT NOT IMPLEMENTED**.

## Canonical lineage decision

The user explicitly selected Test137/V0.152 plus `/WWF-Saturn/WWF-Saturn-Test143-WIP-from-Test137.zip` for reconciliation. That line is now closed through Test144.

The separate `/WWF-Saturn/WWF-Saturn-Test140-V0155-source.zip` and later merged-from-Test140 Test143 WIP are **non-canonical research archives**. Do not use them as the source baseline. Their useful `$23856`, `$1EE38` and low-stamina-boundary findings are preserved for future revalidation under new Test numbers so they do not overwrite canonical Tests138-140.

## Tests138-144 closed state

- **Test138 ROM-EXACT:** stamina `+$66` → bucket `+$BD`: `>=128→0`, `101..127→1`, `80..100→2`, `64..79→3`, `37..63→4`, `1..36→5`, `0→8`.
- **Test139 ROM-EXACT:** `$1E45C-$1E464` compares raw `+$109 == +$BD`; no semantic label assigned to `+$109`.
- **Test140 ROM-EXACT audit:** explicit `+$109` read `$1E45C`, increment `$2006C`, direct compare sites around `$21282/$2128A`; producer/consumer path proven, semantics unclaimed.
- **Test141 ROM-EXACT:** `ST_RISE` renders `$181FC/$181EC` for 13+13 ticks then `$11E36` for 17 ticks.
- **Test142:** TAG/Royal winner/loser ownership exact from match result; Saturn staging coordinates/speeds **PROVISIONAL**.
- **Test143:** large centre result plate removed so the ring remains visible; exact arcade typography/layout **REGISTERED BUT NOT IMPLEMENTED**.
- **Test144 ROM-EXACT:** on `+$109 == +$BD`, the recovered branch clears raw `+$20` low byte and writes raw `+$60=$004B` for `+$61==$4A/$64`, otherwise `$0056`; on mismatch tested outputs remain unchanged. This proves a stamina-derived raw state consequence but not a fatigue/aggression/cadence interpretation.

## Validation

- Host regression: **139/139 PASS**.
- Targeted ASan/UBSan Tests138-144: **PASS**.
- SH-2/Yaul compile+link: **PASS**.
- Saturn ISO structural checks: **PASS**.
- Final MODE1/2352 BIN/CUE sector regeneration/round-trip: **PASS**.
- Independent libcdio payload read + bitwise EDC: **PASS**.
- Hardware Test144: **PENDING**.

## Immediate continuation from Test145

1. Recover exact TAG/Royal victory, defeat and winner-clap/celebration descriptors/states/coordinates/timing. Keep Test142 staging provisional until proven.
2. Continue the stamina/AI raw-state route until `+$109/+$60/+$61` semantics are demonstrated. Do not wire guessed tired-walk, cooldown, aggression or cadence behavior.
3. Complete/prove **All Super Throws / Signature Finisher Moves**. The current source already has overlay/state-machine routes for ten trademark moves, but their individual entry/callback/damage/variant evidence levels remain authoritative; the completeness target is **REGISTERED BUT NOT FULLY IMPLEMENTED/PROVEN**.
4. Revalidate useful evidence from the non-canonical Test140 research archive under new future Test numbers before integration.
5. Continue exact TAG apron/entry/exit, missing recovery frames, Royal assets/eliminations, mode-select glow, records, attract/promo scenes and audio.

## Long target

Continue concrete checkpoints toward Test200. At Test200 report exactly:
- IMPLEMENTADO
- REGISTRADO PERO NO IMPLEMENTADO
- PROVISIONAL
- PENDIENTE DE BUILD SATURN
