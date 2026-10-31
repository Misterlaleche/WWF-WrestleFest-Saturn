# SESSION-05 independent review

Reviewed uncommitted production/full_game changes against baseline e349c776d35c861a87a2f4346308f3e0b15eb37a. Read SPEC.md, SELECTION-04.md, SESSION-05.md and the exact HW1 frontend/match/support/world/video/handoff dependencies. No production, upstream, or Docker state changed during review.

## Findings

1. **[P2] Normalize the P2 Demolition expectation in the session probe** — `production/full_game/tests/session_probe.c:163–164`. The assertions assume the first selected identity always occupies actor0 and TAG Demolition always occupies actors0/1 as 10/11. Running the existing sanitized probe with `0 8 9 1 1085 1 0` reaches GAMEPLAY and then aborts at line163. Its selection snapshot already contains actor0 identity11, uncredited, and actor1 identity10, credited; both use physical port140022. That matches the original documented TAG canonicalization when actor0 lacks credited-player bit. Fix the harness expected pair for the original P2 swap and include this route in run_session_checks.py. Do not change the production identity ordering to satisfy this assertion. Reproduction evidence: `work/session05-review/tag-demolition-p2/run.log`, `selection.ram`, `selection.json`.

2. **[P2] Complete the SH2 relocatable-core validation before reporting a successful SH2 build** — `production/full_game/tools/build_session.py:48–54`. The reviewed version fails the requested SH2 build because the undefined-symbol check includes retained, unreferenced symbol-table entries; the recorded sh2-linked-build.log raises ValueError and never produces its final archive/report. The parent subsequently diagnosed that the extra symbols have no relocations and is implementing reproducible objcopy cleanup preserving all three session API exports. Re-run the build and verify the resulting unresolved symbols after that change. This is a build verification/tool defect, not evidence that the runtime actually calls those absent VDP/host functions.

## Validation and limits

- Independently ran source-lock verification: all 2,023 files PASS; git diff --check PASS.
- Independently replayed the existing ASan/UBSan session binary with TAG stride2 and Royal slot9 stride8. Both PASS, with respectively four and six active actors and the intended selected identities. Logs/snapshots are under work/session05-review.
- Read the full-suite log after completion: ordinary and sanitized native session journeys pass, including normal TAG, P2, Demolition P1, Royal0/9, attract START cancellation, alternate seed, slow events and terminal match dependency error. The remaining native clock/prematch checks also pass in that log.
- The START/credit oracle covers original RAM below3AD4, D/A and carry for explicit fixtures. It is not an end-to-end original-ROM differential proof of the full journey.
- The session preserves its single raw RAM image, shared D-register object, A-register transfers and frontend control context across AC0; the runtime import occurs only after the match driver reports the real F8C boundary. Failure and cancellation checks do not reveal a production blocker in the reviewed scope.
- Sound commands continue through the existing bus/event owner; the host records their count and does not play audio. Tests do not prove an exact command-order differential or future Saturn audio behavior.
- No executable disc, visual transport fidelity, physical IRQ timing, cartridge configuration or combat performance is established by this host/SH2 module. The stated scope correctly leaves disc04 unchanged. No defect is raised for that explicit boundary.

Disposition: address the two verification/harness findings and recheck their targeted evidence. No production-state correctness blocker found within the reviewed module scope.
