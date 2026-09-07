# Test255 WIP26 — exact prone category-$05 live dispatch

Classification: Test255/V0.270 WIP. Test254/V0.269 remains canonical.

## ROM-EXACT live input changes
- `fighter_t` carries `raw_action` for context-dispatched arcade actions.
- State transitions clear stale `raw_action`.
- Human prone category `$05` physical P now dispatches exact raw action `$08` for every wrestler.
- Human prone category `$05` P+K consults the wrestler-specific ROM table: `$22` reaches PICK UP; other valid raws are rendered from their exact descriptor rather than a guessed semantic ground-drop name.
- K remains the recovered `$48` cover/pin path.
- `game_arcade_pose(ST_GROUND_ATTACK)` and the finite visual clock use the exact descriptor selected by `raw_action` when present.

## Test correction
- Historical `test_ground_drop` no longer asserts the obsolete universal P+K -> catalogued semantic ground-drop behavior.
- It now checks all 12 selections against exact category-$05 raw dispatch.
- Test290 locks representative `$08`, `$09`, and `$22` live paths.
- CPU semantic ground AI remains isolated and unchanged until its exact chooser is recovered.

## Focused regression passed
- ground_drop
- Test290 prone raw runtime
- ground pin input
- Test214 human ground PICK UP
- CPU ground AI

## Still pending
- callback-held completion/event semantics for STOMP raw `$23` and FIST DROP raw `$26`;
- exact callback-driven impact/release rather than guessed fixed durations;
- complete reachable ground-route audit = 0 missing;
- then TAG intervention/palmada/apron.

Library: `/WWF-Saturn/WWF-Saturn-Test255-WIP26-source.zip`
SHA256: `863d20ef4eacb132398bc9a97560ae0d94ce9b315437413a63ee0c3fcd0fe529`

No arcade ROM bytes are stored in this checkpoint.
