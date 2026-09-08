# WIP148 CHECKPOINT — typed held operation clock application

## Scope
Implement the callback-before-common-updater counter semantics established by WIP145 using WIP147's shared typed held-operation API. No live runtime wiring.

## Added
- shared `arcade_hold_operation.h` for held operation types;
- `arcade_action_clock_apply_hold_operation()`.

Exact behavior:
- KEEP: `$FF00` remains frozen;
- CLEAR_AND_ADVANCE: emulates callback `CLR.W +$22`, then same-pass common tick advances/finishes the descriptor phase;
- SET_FINITE: writes the supplied counter then performs the same-pass common tick (`$0018 -> $0017` for raw `$13`);
- STATE_TRANSITION: returns false and does not tick the obsolete descriptor.

Test408 verifies raw `$0B` KEEP/CLEAR, raw `$13` `$0018` re-arm inclusive residency, and raw `$0E` state-transition non-tick behavior.

## Source materialization
`WWF-Saturn-Test255-WIP148-source.zip`
SHA256: `08d5726f3abe7966bc474918612cf7bd6ab9427426aedd0043a2a06efdc885c9`

## Safety
No call from `game.c`; all live release guards remain unchanged. No fallback removal. No `tick==12` migration. No BIN/CUE. No ROM bytes persisted.
