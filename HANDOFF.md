# New-chat handoff — WWF WrestleFest Saturn

Use this private repository and persistent ChatGPT Library as source of truth.

## Start here

1. Read `PROJECT-STATE.md` and `docs/HARDWARE-TEST100-FEEDBACK.md`.
2. Current validated source snapshot:
   - `/WWF-Saturn/WWF-Saturn-Test118-V0133-source.zip`
   - SHA256 `a3b7b197828548c51fe0beda249e603c959c949b2d6f75146fe846418c93cff3`
3. Continue from **Test119 / V0.134** only.
4. Do not resume from obsolete divergent snapshots; Test118 is on the canonical Test102→Test114 line.
5. Every Test must be concrete implementation or rigorous validation. Preserve ROM/reference evidence labels; anything unproven stays PROVISIONAL or REGISTERED BUT NOT IMPLEMENTED.
6. Do not generate a new BIN/CUE routinely. Test100 is the latest hardware image and is confirmed working on real NTSC-U Saturn + MODE. Build another only at a major integration checkpoint or for a Saturn-only blocker.

## Current validated state

- Test118/V0.133.
- 114/114 host binaries PASS.
- Tests115-118 targeted ASan/UBSan PASS.
- Test115-118 add a real Royal Rumble entrant lifecycle: complete ten-wrestler unique queue, six active slots separated from queue ownership, CPU elimination/replacement, announcement/entry phase, immediate human elimination, and final-survivor resolution only after queue exhaustion.
- Rumble entrant ordering entropy, announcement timing, entry timing and entry geometry are still PROVISIONAL pending ROM/reference recovery.
- Test103-114 hardware-feedback features remain preserved.

## Immediate continuation

Test119 should connect the new Rumble replacement state to visible entrant announcement/presentation and make entry-state ownership robust against normal nearest-opponent slot swapping. Continue extracting ROM/reference evidence before claiming exact timings or geometry.

In parallel, keep investigating CPU decision cadence/damage/stamina from ROM to address the Test100 hardware report that CPU is disproportionately hard and player stamina drains too fast. Do not replace that with arbitrary global weakening.

Other backlog: exact apron placement/rope animation, post-match winner/loser staging, mode-select glow, portrait/HUD palette fidelity, records, attract/promo scenes, ring-out hardware validation and audio. The future title Start options screen remains separate until the user's art is supplied.

## Long target

Continue toward Test200. At Test200 report exactly:
- IMPLEMENTADO
- REGISTRADO PERO NO IMPLEMENTADO
- PROVISIONAL
- PENDIENTE DE BUILD SATURN
