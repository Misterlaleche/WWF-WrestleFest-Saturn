# WWF Saturn WIP172 — FINAL handoff audit / integration gate

Status: AUDITED STAGING ONLY. No canonical runtime source modified. No BIN/CUE generated.

## Authority order
1. Current canonical source Test160/V0.175 or later WIP160+ source once materialized and SHA-verified.
2. PUNTO 2 actor32/raw10 COMPLETE package (mandatory before Ground/raw10 mutation; pending local materialization in this session).
3. victim +$65 FINAL (structural authority for state4 low byte, high flags, collision FIRST→SECOND, scheduler/contact37 ordering).
4. AI CANONICAL AUDITED FINAL (supersedes previous AI FINAL; numeric/raw behavior only where proven).
5. Animation FINAL package (tables/interpreter rules; preserve explicit BLOCKERS/PROVISIONAL items).
6. Presentation FINAL AUDITED + Attract/Bio FINAL AUDITED additive extension.
7. Audio FINAL DEFINITIVE A53 handoff; A49/A44 policy where applicable; no invented music loops.
8. Records FINAL package.
9. Credits FINAL package.
10. Referee/Announcer FINAL package.

## Fresh validation run against user wwfwfest.zip
Main ROM authority expected SHA1: a726c46f350723ea73158bc78055d17d418b14e2.

PASS:
- Referee/Announcer package consistency.
- Referee/Announcer critical ROM opcode audit.
- Records 14/14 model tests.
- Animation package consistency, table structure, deep consistency, CLI tests.
- Presentation: 16/16 C tests compile with -std=c11 -Wall -Wextra -Werror and run PASS.
- Presentation: 2/2 Python regressions PASS.
- Attract/Bio ROM test PASS against wwfwfest.zip.
- Attract/Bio host C runtime compile with warnings-as-errors and runtime test PASS.
- Credits ROM exact test PASS against wwfwfest.zip.
- Credits package audit PASS.
- Audio A53 final audit test PASS.
- Audio A47, A49 status and A51 evidence tests PASS.

## Non-negotiable integration guards
- Never replace the current WIP160+ gameplay tree with donor-source-A16 from audio package.
- Never silently overwrite a newer canonical module with an older handoff file.
- Attract/Bio must bind to the actual 14-state attract controller; legacy forced state $02 adapter is forbidden.
- Attract/Bio isolated visual RNG remains PROVISIONAL until shared canonical $21B4 RNG ordering is available.
- Credits must return through frontend-equivalent $000006FC, not directly title.
- Credits $4122/$4F22 are palettes, not tilemaps.
- Records TAG and Royal are independent 11-entry tables; previous 'no records' result is superseded.
- Audio: do not invent loop_start=0, do not invent FM0C loop, do not hide EOF with endless zeros. Continuous original behavior remains continuous-state synthesis or explicitly-labelled Saturn approximation.
- Animation: do not generic-clear +$65 on state exit; preserve byte-vs-word high-byte semantics; do not promote blocked three-actor choreography/unknown semantics.
- victim+$65: preserve callback -> collision -> damage/tier order, FIRST->SECOND precedence and current-turn contact37 semantics.
- AI: use current canonical audited FINAL only; do not translate weights into percentages unless proven; preserve numeric selectors/states where semantic labels are not closed.
- Referee count routine must preserve inherited +$25 rather than force a fabricated initial phase.
- Announcer exit must not invent generic facing writes when the visual orientation is encoded by exact frame selectors.

## Build gate
A Saturn build is NOT authorized by WIP172 alone. It becomes worthwhile when all are true:
- canonical source WIP160+ is materialized and SHA verified;
- PUNTO 2 actor32/raw10 COMPLETE is reconciled;
- additive modules apply without overwrite conflicts;
- host regression of canonical source remains green;
- SH-2/Yaul compile+link passes after integration;
- generated asset packs required by Presentation/Attract/Audio are rebuilt locally from the verified user ROM and pass hashes/contracts;
- no previous hardware-stability guard (HWFIX bounded fallback / storage arena) is regressed.
