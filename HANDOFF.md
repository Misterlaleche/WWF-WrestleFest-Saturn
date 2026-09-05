# New-chat handoff — WWF WrestleFest Saturn

Use this private repository and persistent ChatGPT Library as source of truth.

## Start here

1. Read `PROJECT-STATE.md` and `docs/HARDWARE-TEST100-FEEDBACK.md`.
2. Current validated source snapshot:
   - `/WWF-Saturn/WWF-Saturn-Test137-V0152-source.zip`
   - SHA256 `461912913a2edbf2b4be532c8378d81f74abbe3d088ff481647fda5283b59261`
3. Continue from **Test138 / V0.153** only.
4. Do not resume from older divergent snapshots. Test137 is the canonical continuation of Test133 and preserves the validated Test102→Test114→Test118→Test119→Test130→Test133 line.
5. Every Test must implement or rigorously validate a real function. Preserve ROM/manual/video evidence labels; anything unproven stays PROVISIONAL or REGISTERED BUT NOT IMPLEMENTED.
6. Do not generate another BIN/CUE routinely. Test100 remains the latest hardware image and is confirmed working on real NTSC-U Saturn + Terraonion MODE. Build another only after a large integrated advance or a Saturn-only blocker.

## Current validated state

- **Test137/V0.152**.
- **133/133 host binaries PASS** in bounded batches.
- Tests122-137 targeted ASan/UBSan PASS.
- SH-2 ELF compile/link with Yaul PASS. No newer disc image was produced.
- Test134 replaces the old provisional 20-point arcade Punch damage with the exact ROM-backed 3-point value.
- Test135 adds the live rapid-button prompt to player-victim submissions.
- Test136 maps visible ground-drop actions to the exact arcade descriptors for Stomp/Elbow/Knee/Fist drops.
- Test137 adds CPU PICK UP action `$22` / descriptor `$15B6C` against prone live opponents; completion timing/state translation remains provisional.
- Difficulty direct handling remains evidence-based: the DIP field changes CPU stamina setup, not a fabricated CPU-speed/aggression multiplier.
- Wrestler-specific weighted AI tables and raw state-2 control-flow evidence through Test133 remain integrated but only semantically connected where proven.
- Royal Rumble preserves six-slot lifecycle, exact 294-tick replacement wait, entrant callout and live Body Slam rope elimination route.

## Immediate continuation

Test138 should focus on fatigue/exhaustion behavior visible in the arcade: recover the relevant animation/state evidence before mapping tired walk or stationary exhausted poses. In parallel continue exact CPU ground-choice probability and raw state-2 handler analysis so Test100 difficulty can be corrected through original decision rules rather than arbitrary cooldowns.

Other priorities: exact TAG apron placement/entry animation, missing roll/get-up frames, additional Royal eliminations and Hawk/Animal assets, TAG/Royal winner-loser staging, mode-select glow, portrait/HUD palette fidelity, records, attract/promo scenes and audio.

## Long target

Continue concrete checkpoints toward Test200. At Test200 report exactly:
- IMPLEMENTADO
- REGISTRADO PERO NO IMPLEMENTADO
- PROVISIONAL
- PENDIENTE DE BUILD SATURN
