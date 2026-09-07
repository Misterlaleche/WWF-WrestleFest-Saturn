# Test255 WIP48 — actor32 bit2 state continuity

## Scope
Close ROM-proven `+$32 bit2` persistence across main-state transitions relevant to ground callbacks, without enabling live `$FF00` release.

## ROM-EXACT
- Main state 0 init clears actor `+$32 bit2` at `$114CC`.
- Main state 2 callback `$11C82` preserves actor `+$32 bit2`; its audited straight-line transitions write only states 3/6/4 and contain no bit2 clear.
- Main state 5 dispatches through descriptor `$11DCA` to bare RTS `$12612`, therefore state 5 itself preserves inherited actor `+$32 bit2`.
- WIP46 established raw `$23/$35` terminal `$FE` transitions to main state 5; combining the two exact audits means these terminal paths preserve inherited actor32 bit2.
- State-7 terminal raws (`$0B/$10/$13/$14/$46/$47`) are deliberately not promoted to a preserve/clear policy because state 7 bit2 behavior is not pinned by the available audit.

## Code/tests
- Added `arcade_actor32_bit2_main_state_policy()` exposing only proven state policies: state0=CLEAR, state2=PRESERVE, state5=PRESERVE.
- Added `arcade_prone_terminal_preserves_actor32_bit2()` for exact terminal-state composition.
- Test331 PASS.
- Test332 PASS.
- Regression Test329/Test330 PASS.
- Static Test321 PASS.
- `tools/check_host.py` syntax restored and PASS after correcting the test registration newline issue.

## Runtime status — unchanged
- Live `contact37 -> release=true`: **REGISTERED BUT NOT IMPLEMENTED**.
- 32-tick `$FF00` fallback: **PROVISIONAL**.
- legacy `tick==12`: **PROVISIONAL**; raw `$47` exact callback profiles remain unscheduled until physical timing is safe.

## Remaining contact blocker
1. inherited `+$42` source/lifetime for non-immediate-probe held gates;
2. inherited raw `+$0E` transitions for raws without exact target-Z rebase;
3. state-7 actor32 bit2 policy only if required to prove continuity into/out of a reachable hold;
4. contexts for `$11E04` clear and `$0F3BC` set remain intentionally unclassified unless reachability requires them.

## Fidelity
- **ROM-EXACT:** state0 clear, state2 preserve, state5 preserve, raw `$23/$35` terminal state5 continuity.
- **REGISTERED BUT NOT IMPLEMENTED:** live physical-release bridge.
- **PROVISIONAL:** fixed held fallback and native damage scheduler.

Source checkpoint:
- Library: `/WWF-Saturn/WWF-Saturn-Test255-WIP48-source.zip`
- SHA256: `77d919463ce19a58e0366ce4926802492b77dac657f35725de522d40a38eff82`

No ROM image included. No BIN/CUE generated.
