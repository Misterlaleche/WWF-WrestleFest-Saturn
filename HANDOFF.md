# New-chat handoff — WWF WrestleFest Saturn

Use this private repository and persistent ChatGPT Library as source of truth.

## Start here

1. Read `PROJECT-STATE.md` and `docs/HARDWARE-TEST100-FEEDBACK.md`.
2. Current validated source snapshot:
   - `/WWF-Saturn/WWF-Saturn-Test114-V0129-source.zip`
   - SHA256 `a58835923d3ad97da97c765ac652cd2c925bebc5a8d4f0edfd58e8204d32204d`
3. Continue from **Test115 / V0.130** only.
4. Do not resume from the obsolete divergent Test105/Test115 snapshots that were removed from Library; Test114 was cleanly rebased on canonical Test102.
5. Every Test must be concrete implementation or rigorous validation. Preserve ROM/reference evidence labels; anything unproven stays PROVISIONAL or REGISTERED BUT NOT IMPLEMENTED.
6. Do not generate a new BIN/CUE routinely. Test100 is the latest hardware image and is confirmed working on real NTSC-U Saturn + MODE. Build another only at a major integration checkpoint or for a Saturn-only blocker.

## Current validated state

- Test114/V0.129.
- 110/110 host tests PASS (55 + 55).
- Tests103-114 ASan/UBSan PASS.
- SH-2 compile checks for main/game/session/HUD PASS.
- Test101 Rumble active-recovery gate and Test102 normal Kick corner TAG are preserved.
- Test103-114 close a large part of the Test100 hardware-feedback backlog: TAG partner HUD/stamina, render-only reserve correction, non-static rope traversal animation, staged/longer TAG intro timeline, referee tracking, pin 1/2/3 indicator, full-pool CPU roster selection, RAM-cached select pages, recovery mash prompt, rope deflection visual and Royal six-position opening + 1P marker.

## Immediate continuation

Test115 should start the Royal Rumble progression block. Current game still needs exact/controlled lifecycle for eliminations, replacement entrants, entrant announcement and ring entry. Do not assume a pure fixed-time entrant schedule without ROM/reference evidence; distinguish six simultaneous actor slots from the overall Rumble participant roster/queue.

In parallel, keep investigating CPU decision cadence/damage/stamina from ROM to address the Test100 hardware report that CPU is disproportionately hard and player stamina drains too fast. Do not replace that with arbitrary global weakening.

Other confirmed backlog: exact apron placement/rope animation, post-match winner/loser staging, mode-select glow, portrait/HUD palette fidelity, records, additional attract/promo scenes, ring-out hardware validation and audio. The future title Start options screen remains separate until the user's art is supplied.

## Long target

Continue toward Test200. At Test200 report exactly:
- IMPLEMENTADO
- REGISTRADO PERO NO IMPLEMENTADO
- PROVISIONAL
- PENDIENTE DE BUILD SATURN
