# New-chat handoff — WWF WrestleFest Saturn

Use this private repository and persistent ChatGPT Library as source of truth.

## Start here

1. Read `PROJECT-STATE.md` and `docs/HARDWARE-TEST100-FEEDBACK.md`.
2. Current **validated canonical** source snapshot:
   - `/WWF-Saturn/WWF-Saturn-Test137-V0152-source.zip`
   - SHA256 `461912913a2edbf2b4be532c8378d81f74abbe3d088ff481647fda5283b59261`
3. There is also a **non-canonical WIP continuation** carrying the in-progress Test138-143 edits from this chat:
   - `/WWF-Saturn/WWF-Saturn-Test143-WIP-from-Test137.zip`
   - SHA256 `a47b53be720a06c9d70104e78c87c232ab769ca5aceb339a832c112087866aaa`
   - IMPORTANT: this WIP is not a validated checkpoint and its Makefile metadata still identifies Test137/V0.152. Use it only to continue/close the already-started Test138-143 work; do not claim it as a Saturn build or canonical release until full host regression, sanitizers and SH-2 compile/link are rerun and metadata/package integrity are updated.
4. Continue from the Test137 canonical line. Reconcile the WIP first; do not resume from older divergent snapshots.
5. Every Test must implement or rigorously validate a real function. Preserve ROM/manual/video evidence labels; anything unproven stays PROVISIONAL or REGISTERED BUT NOT IMPLEMENTED.
6. Do not generate another BIN/CUE routinely. Test100 remains the latest hardware image and is confirmed working on real NTSC-U Saturn + Terraonion MODE. Build another only after a large integrated advance or a Saturn-only blocker.

## Current validated state

- **Test137/V0.152** canonical.
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

## In-progress Test138-143 work in the WIP snapshot

- Test138/139: `arcade_ai_stamina_bucket()` implements the recovered fine stamina buckets: `>=128->0`, `101-127->1`, `80-100->2`, `64-79->3`, `37-63->4`, `1-36->5`, `0->8`; `arcade_ai_stamina_gate_match()` implements the direct raw `+$109` equality predicate. Do not label the semantic meaning of `+$109` as fatigue until proven.
- Test140: raw `+$109` consumer/producer audit is in progress; semantic label still intentionally withheld.
- Test141: `game_arcade_pose()` maps `ST_RISE` to recovered arcade descriptors: role-specific `$181FC/$181EC` for the first 26 ticks, then `$11E36` for the final 17 ticks, matching the already recovered 13+13+17 rise timing.
- Test142: `postmatch_update()` adds first native TAG/Rumble winner/loser staging after the result. Winner/loser ownership is exact from resolved match state; Saturn coordinates/speed remain PROVISIONAL pending arcade post-match coordinate recovery.
- Test143: result rendering keeps the ring/post-match staging visible rather than covering the centre with the old large result plate. The large plate removal is intentional; exact arcade post-match typography/layout remains pending.
- Host regression for the Test143 working tree had reached **137/137 PASS** in bounded batches and targeted ASan/UBSan for the new work passed, but this WIP was NOT closed as canonical because the final SH-2 validation/package metadata step was not completed in that chat.

## Immediate continuation in the new chat

First, materialize both the Test137 canonical snapshot and the Test143 WIP, verify that the WIP is a direct continuation, then close Test138-143 properly: rerun the relevant host tests, targeted sanitizers, SH-2 compile/link with Yaul, update Makefile version/title only after validation, package a self-contained source checkpoint, and then update `PROJECT-STATE.md`/`HANDOFF.md`.

After that continue the two active research fronts:

1. Recover the exact arcade descriptors/states for TAG and Royal victory, defeat and winner clap/celebration. Keep current post-match movement provisional until exact coordinates/timing are demonstrated.
2. Continue the stamina/AI raw-state route so fatigue/exhaustion affects live CPU behavior only when the semantics are proven. Do not invent tired-walk or aggression cooldowns.

Other priorities: exact TAG apron placement/entry animation, missing roll/get-up frames, additional Royal eliminations and Hawk/Animal assets, mode-select glow, portrait/HUD palette fidelity, records, attract/promo scenes and audio.

## Long target

Continue concrete checkpoints toward Test200. At Test200 report exactly:
- IMPLEMENTADO
- REGISTRADO PERO NO IMPLEMENTADO
- PROVISIONAL
- PENDIENTE DE BUILD SATURN
