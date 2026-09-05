# Test100 / V0.115 — Saturn hardware feedback

Target hardware: Sega Saturn NTSC-U + Terraonion MODE.

## Confirmed hardware result

- Test100 BIN/CUE boots and runs on real Saturn hardware.
- This closes the previous `Hardware boot: NOT YET PERFORMED` status for Test100.

## User-observed gameplay / fidelity issues

### CPU difficulty and stamina

- CPU is disproportionately difficult in normal play.
- Player stamina depletes too quickly.
- Revisit CPU decision frequency, aggression/priority, counter windows, stamina-dependent policy, player damage/stamina drain and recovery.
- **Recover and reproduce the rival CPU behavior per arcade difficulty level**, not merely a global easy/normal/hard multiplier. Difficulty should govern the same decision cadence, aggression, counter/reversal tendency, move selection, stamina/tier behavior and opportunity rules used by the arcade where recoverable.
- Do not simply weaken CPU globally; aim for arcade-like behavior and recovered difficulty/stamina rules where possible.

### TAG Tournament — missing/incorrect systems

- Manual tag/relevo to the partner is not yet implemented and is now a high-priority missing core mechanic.
- Partner waiting position is wrong: reserve wrestlers remain too far in front of the ring ropes instead of reading visually as waiting behind/on the apron side of the ropes.
- Partner entry into the ring to break/help during a pin has a doubtful/non-arcade-looking animation/path and needs to be reworked.
- HUD must show the partner with stamina in TAG; current text/box colors do not match the arcade.
- Wrestler portraits show palette/color errors.
- CPU TAG opponent selection currently repeats Jake + Big Boss Man at the start and fails to exercise the broader roster, including second-page wrestlers.

### TAG Tournament pre-match presentation

Arcade reference from user hardware/video capture:
- Presentation lasts substantially longer than the Saturn port.
- Female ring announcer presents each wrestler individually.
- Each wrestler performs a pose after being introduced.
- Ring announcer then leaves the ring/stage.
- These timings must be preserved as presentation timing contracts for future speech/music/SFX integration.
- During the TAG presentation, all four wrestlers have distinct positions matching the arcade reference; the Saturn port currently places them incorrectly so that sprites overlap/occlude each other.

### Rope behavior

- Rope deformation is missing when a wrestler runs into / rebounds from the ropes.
- Current ropes remain rigid; arcade ropes visibly yield and recover.
- This needs a rope state/animation coupled to rebound events rather than a static background-only representation.

### Ring-out / ringside

- User has not yet been able to throw an opponent out of the ring and therefore has not yet verified ringside gameplay in Test100.
- Treat ring-out/ringside as hardware validation pending, not as confirmed working.

### Pin count feedback

- Arcade shows the visible count indicator 1, 2, 3 during the referee's pin count.
- Saturn port currently lacks this on-screen indicator.

### Win / loss automation and post-match staging

- Arcade automatically transitions after the result:
  - defeated wrestlers show a sad/defeated pose;
  - winners move to the center of the ring;
  - winners perform the victory clap/celebration.
- Test100 does not yet reproduce this sequence.
- **Implement the corresponding victory and defeat animations/staging in both TAG Tournament and Royal Rumble**, with mode-specific winner/loser resolution matching the arcade rather than a generic shared result screen.

### Mode select presentation

- Arcade TAG / Royal mode-select highlights the currently selected text with a luminous/pulsing effect.
- Test100 lacks this effect.

### Character select

- TAG character-select transition to the second section/page visibly feels like a load in the Saturn port.
- Arcade transition is effectively instantaneous.
- Future implementation should preload/cache the second page or otherwise remove the visible loading transition.

### Knockdown / get-up feedback

- When the player is knocked down, arcade includes additional rolling / recovery / stand-up animation frames that are missing in Test100.
- Arcade also shows an animation/indicator telling the player to press a button rapidly during recovery; this cue is missing.

### Referee behavior

- In TAG combat, Test100 referee remains static near the center.
- Arcade referee follows the action around the ring.
- Referee AI/navigation should track relevant actors and pin/submission/ring-out situations while maintaining legal positioning.

## Royal Rumble — missing/incorrect systems

### Initial formation

- Test100 places the initial wrestlers side-by-side.
- Arcade uses a distinct staged formation, as shown in the user's second reference screenshot.
- Initial coordinates/order must be recovered and reproduced.

### Player identification

- Arcade displays a green `1P` marker above/near the human-controlled wrestler at match start.
- This is important for multi-human Rumble readability and should generalize to 2P/3P/4P if supported by the recovered arcade logic.

### Elimination and entrant flow

- User has not yet been able to eliminate an opponent in Test100; elimination remains hardware-unverified.
- Missing arcade behavior:
  - new wrestlers enter at timed intervals;
  - entrant physically approaches / climbs into the ring;
  - an inset/box animation announces the new entrant.
- These are core Royal Rumble progression mechanics, not optional presentation polish.

### Royal Rumble result presentation

- Implement arcade-faithful **victory and defeat animations/staging for Royal Rumble** as a first-class mode requirement.
- The winner should receive the correct post-match presentation; eliminated/defeated human state should use the arcade defeat presentation rather than falling directly to a generic flow.

## Presentation / attract scenes

- User supplied an arcade reference showing a Legion of Doom interview/promo scene with interviewer; this scene is not yet present in the port.
- Add to presentation/attract backlog as an arcade scene that needs implementation.

## Records

- Records screen still does not appear.
- Remains a required arcade feature.

## Future Saturn options screen

User will prepare the visual art/design in a separate chat. Planned title-screen START/options layer should eventually expose at least:
- difficulty;
- Arcade Mode / Saturn Mode;
- sound mode;
- load records;
- load settings;
- OST player;
- SFX player;
- legal/copyright information;
- additional Saturn-specific options as they become defined.

This future options screen must not silently alter Arcade Mode behavior. Saturn-only quality-of-life/content options should remain cleanly separated from the arcade-faithful ruleset.

## Recommended implementation order from Test101

1. Recover and reproduce **CPU behavior by arcade difficulty level** together with player/CPU stamina rules; avoid arbitrary global nerfs.
2. Implement/complete manual TAG and correct partner active/reserve transition.
3. Correct partner/apron visual placement and pin-assist entry/exit lifecycle.
4. Fix TAG presentation duration, individual introductions/poses and four-wrestler staging.
5. Implement referee tracking + pin 1/2/3 indicator.
6. Add missing knockdown roll/get-up and rapid-button recovery cue.
7. Add rope deformation/rebound animation.
8. Fix TAG HUD/palettes/portraits and roster/opponent selection.
9. Complete Royal Rumble formation, 1P marker, eliminations, timed entrants and entrant announcement.
10. Implement arcade-faithful **victory/defeat animation and post-match staging in both TAG Tournament and Royal Rumble**.
11. Add mode-select glow, instant character-select page transition, records and remaining attract/presentation scenes.

All recovered behavior must continue to distinguish ROM/manual/video-backed facts from native provisional approximations.
