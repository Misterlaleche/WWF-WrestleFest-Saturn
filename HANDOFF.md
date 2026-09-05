# New-chat handoff — WWF WrestleFest Saturn

Use this repository and the persistent ChatGPT Library as the source of truth when continuing development in a new conversation.

## Start here

1. Read `PROJECT-STATE.md` and `docs/HARDWARE-TEST100-FEEDBACK.md`.
2. Use this current validated persistent source snapshot:
   - Library path: `/WWF-Saturn/WWF-Saturn-Test100-V0115-source.zip`
   - SHA256: `e489380655d7f156b1e2ddb0fd00071e28460d1c8d33326a3042fc8b39fc4b96`
3. Continue from **Test101 / V0.116**. Do not resume from older historical checkpoints.
4. Each Test must implement or rigorously validate a real function; no empty number increments.
5. Preserve arcade fidelity. Use `program.bin`, original research and hardware/video references. Anything unproven stays PROVISIONAL or REGISTERED BUT NOT IMPLEMENTED.
6. Keep host tests, sanitizers/checkpoint recovery and `PROJECT-STATE.md` current.

## Test100 validated baseline

- Host regression: **96/96 PASS** (48 + 48 split).
- Clean extraction recompiles with Yaul and passes structural ISO verification.
- Real Saturn BIN/CUE generated and persisted.
- **Hardware boot: PASS on Sega Saturn NTSC-U + Terraonion MODE.**
- BIN: `/WWF-Saturn/WWF-WrestleFest-Test100-V0115.bin`
- BIN SHA256: `0ac98e88a4db30bf51a9edcd13fc3598442f22095a2b400d48e6da331954f3b3`
- CUE: `/WWF-Saturn/WWF-WrestleFest-Test100-V0115.cue`
- CUE SHA256: `4125d7532ed37f54f2e1bbf1cdcc752ed7fa86b4eebc498ccbe55590c6029ef5`

## Hardware feedback priority

The complete user-observed backlog is in `docs/HARDWARE-TEST100-FEEDBACK.md`. Immediate issues:

- CPU difficulty is too high and player stamina drains too quickly.
- Manual TAG/relevo is missing.
- Reserve/partner waiting position is wrong relative to the ropes; pin-assist entry/exit animation is doubtful.
- TAG intro is too short, lacks correct per-wrestler presentation/pose timing, announcer exit and correct four-wrestler staging.
- Rope rebound deformation is missing.
- Pin 1/2/3 visual count is missing.
- Post-match winner/loser staging is missing.
- Mode-select glow is missing.
- TAG character-select second page visibly loads instead of transitioning instantly.
- TAG CPU selection overuses Jake + Big Boss Man and underuses the rest of the roster.
- TAG partner HUD/stamina, text/box colors and portrait palettes need correction.
- Missing knockdown roll/get-up animation and rapid-button recovery cue.
- Referee remains static instead of following action.
- Royal Rumble formation is wrong; 1P marker, eliminations, timed entrants, ring entry and entrant announcement are incomplete/missing.
- Records screen is missing.
- Missing attract/presentation scenes include a supplied Legion of Doom interview/promo reference.
- User is preparing separate art for a future title-screen START/options menu with Arcade/Saturn mode split and additional options.

## Recommended Test101+ order

1. CPU difficulty/stamina correction using arcade evidence.
2. Manual TAG/relevo and active/reserve lifecycle.
3. Reserve/apron placement and pin-assist entry/exit.
4. TAG presentation timing/positions/poses.
5. Referee tracking + pin 1/2/3 indicator.
6. Knockdown recovery cue/animations.
7. Rope deformation.
8. TAG HUD/palettes/portrait fixes and roster selection behavior.
9. Royal Rumble progression/formation/1P marker/entrants.
10. Post-match automation, mode-select glow, instant character-select page transition, records and attract scenes.

## Long target

Continue toward **Test200**, intended as an alpha-class integrated build candidate. At Test200 produce the report divided exactly into:
- IMPLEMENTADO
- REGISTRADO PERO NO IMPLEMENTADO
- PROVISIONAL
- PENDIENTE DE BUILD SATURN
