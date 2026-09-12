# HANDOFF

Read `README.md`, `PROJECT-STATE.md`, `docs/SATURN-PIPELINE.md`, `docs/MODE-ROSTER-HARDWARE-VALIDATION.md`, `docs/AUDIO-HARDWARE-VALIDATION.md`, `docs/RUNTIME-AUDIO-RECONCILIATION.md`, `docs/AUDIO-PRODUCER-CLEANR01-OWNER-MAP.md`, and `docs/AUDIO-ROM-EXACT-R04-CLOSURE.md` before doing any work.

## Absolute project rule

`wwfwfest.zip` ROM > audited ROM-EXACT FINAL packages > Saturn/Yaul documentation > current Saturn code.

Never invent missing behavior.

## Current immediate work

1. Treat **MODE + ROSTER TEST03** as the canonical hardware-proven combined checkpoint for these two frontend sections.
2. Import its proven behavior into the master/runtime without replacing the canonical CLEAN-R01 main-loop/state ownership.
3. After import, revalidate on real Saturn both routes: `TAG -> ROSTER -> confirm` and `ROYAL -> ROSTER -> confirm`, preserving the no-flash handoff and localized green confirmation patch.
4. Continue the next proven frontend/prematch integration boundary; do not invent the post-ROSTER transition if its owner/evidence is not yet bound.

5. Audio:
   - canonical package: `WWF-SATURN-AUDIO-ROM-EXACT-SOFTWARE-CLOSED-R04-MASTER-REAUDITED.zip`;
   - canonical package SHA256: `c4898b654eb105a88cc24500de96c00b68d188582aa7d7f7dbb13f1b3315ae36`;
   - the earlier R04 ZIP is superseded as an audited handoff because its SHA256 manifest had absolute paths and referenced a missing temporary file; implementation is unchanged;
   - transport is HARDWARE-PASS;
   - original command-byte dispatcher is present;
   - full producer matrix contains 173 ROM-verified rows;
   - owner map R01 classifies 14 rows `INTEGRATION_READY`, 4 `OWNER_SCOPE_ONLY`, 155 `UNRESOLVED`;
   - use only the 14 ready rows for direct runtime insertion;
   - low-stamina is producer `0x009088`, JSR `0x00908C`, command `0x0B`;
   - `0x0263CC -> 0x0B` remains `REGISTERED_NOT_IDENTIFIED`;
   - ring-out/count-out ready rows belong to WP08;
   - finisher ready rows belong primarily to WP01 with WP22/WP10 support;
   - R04 closes the software music path with live Z80 + pinned Nuked-OPM rendering into the existing CA-driven WP24 transport;
   - `0x1F` is sent back into the original driver VM; no PCM-domain linear fade;
   - final real-Saturn validation of live-VM CPU budget under full game load is still mandatory.
   - R04 status: **AUDIO SOFTWARE CLOSED — MASTER REAUDITED**;
   - AUDIO HARDWARE CLOSED is forbidden until live Z80+Nuked-OPM performance, CPU/memory budget, end-to-end runtime integration and final real-Saturn validation all pass;
   - WP24 / Audio owns backend, transport, Z80/OPM VM, reproduction and audio behavior only;
   - Gameplay / Scene Flow / other blocks keep ownership of event producers;
   - Audio must not invent producers or semantic mappings;
   - every UNRESOLVED producer remains UNRESOLVED and fail-closed until ROM evidence proves it;

5. Current canonical gameplay runtime source is still absent from this repository.
   - Do not import WIP160/175/176 as the base.
   - Insert audio hooks only when the canonical CLEAN-R01 equivalent state/callback exists.
   - `OWNER_SCOPE_ONLY` rows are blocked until their semantic trigger is proven.

## What other chats must preserve

- No BIN/CUE concatenation; integrate source modules.
- One shared Saturn platform layer.
- No guessed timings/palette/input/audio semantics.
- No placeholder/fake music presented as final.
- No finite diagnostic PCM capture relabelled as an arcade loop.
- Do not restore TEST-03 fixed-time refill.
- Do not restore historical `wf_audio_observer.c` slot-0 semantics as ROM truth.
- Real Saturn + MODE remains the hardware gate.
- MODE + ROSTER TEST03 is hardware-proven; do not regress to the separate candidate harnesses or reintroduce their freeze/timeouts.
- Do not clear/rebuild the full ROSTER NBG0 on confirmation; use the resident foreground and localized 96x96 VBLANK-IN update.
- Preserve MODE -> ROSTER display blank/synchronization before changing ROSTER CRAM/VRAM.
- See `docs/MODE-ROSTER-HARDWARE-VALIDATION.md`.
