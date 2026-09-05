# New-chat handoff — WWF WrestleFest Saturn

Use this private repository and persistent ChatGPT Library as source of truth.

## Start here

1. Read `PROJECT-STATE.md` and `docs/HARDWARE-TEST100-FEEDBACK.md`.
2. Current validated source snapshot:
   - `/WWF-Saturn/WWF-Saturn-Test130-V0145-source.zip`
   - SHA256 `ab2b4c89d8a70d7121293e7be4c2023fff6e4881c494857021786d1208132b7a`
3. Continue from **Test131 / V0.146** only.
4. Do not resume from older divergent snapshots. Test130 is the canonical continuation of Test119 and preserves the validated Test102→Test114→Test118→Test119 line.
5. Every Test must implement or rigorously validate a real function. Preserve ROM/manual/video evidence labels; anything unproven stays PROVISIONAL or REGISTERED BUT NOT IMPLEMENTED.
6. Do not generate another BIN/CUE routinely. Test100 remains the latest hardware image and is confirmed working on real NTSC-U Saturn + Terraonion MODE. Build another only after a large integrated advance or a Saturn-only blocker.

## Current validated state

- **Test130/V0.145**.
- **126/126 host binaries PASS**, executed in bounded batches.
- Tests127-130 targeted ASan/UBSan PASS.
- SH-2 ELF compile/link with Yaul PASS; no disc image generated.
- Royal Rumble now preserves the canonical queue/replacement/callout lifecycle while adding exact six-slot anchors, reverse free-slot scan, exact 294-tick post-elimination delay and recovered pool-selection structure.
- A live guide-backed Body Slam elimination route exists from movable headlock at the ropes for wrestlers whose arcade repertoire contains Body Slam; Saturn rope-coordinate threshold remains provisional.
- Difficulty DIP bits are recovered exactly. Direct ROM audit shows Difficulty is consumed for CPU stamina setup, not as a direct CPU-speed/aggression multiplier.
- The original `$24CC` weighted selector and two 12-wrestler distance-policy tables are recovered. Do not connect them to native movement until the raw state transition and raw→Saturn distance mapping are demonstrated.

## Immediate continuation

Test131 should prove what the Test130 weighted choice actually does: the branch around `$1F318` writes raw actor `+$20=2` when the first weighted bucket is selected. Recover the raw state-2 handler (`$1D7B6` from the CPU dispatch table) sufficiently to determine whether it corresponds to approach/run/chase or another behavior. Only then wire the per-wrestler probabilities into live Saturn AI.

In parallel continue auditing damage/stamina/recovery consumers to address the Test100 hardware observation that CPU feels disproportionately hard and player stamina drains too quickly. Preserve the proven Difficulty→CPU-stamina behavior and avoid arbitrary global nerfs.

Other priorities: exact TAG apron placement/entry animation, missing knockdown roll/get-up frames, additional Royal eliminations and Hawk/Animal assets, TAG/Royal winner-loser staging, mode-select glow, portrait/HUD palette fidelity, records, attract/promo scenes and audio.

## Long target

Continue concrete checkpoints toward Test200. At Test200 report exactly:
- IMPLEMENTADO
- REGISTRADO PERO NO IMPLEMENTADO
- PROVISIONAL
- PENDIENTE DE BUILD SATURN
