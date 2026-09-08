# WIP171 CHECKPOINT — records UI/initials + referee count program + no-clobber integration

## Scope
Continue subsystem implementation while preserving the WIP160+ gameplay authority boundary. No BIN/CUE generated.

## Records frontend
Added `wf_records_ui.c/.h`:
- exact 11-row pixel coordinate table for initials, TAG values, Royal values and new-entry marker;
- three-character initials runtime;
- interactive selector domain `$31..$4B`, stored glyph domain `$41..$5B`;
- RIGHT/LEFT asymmetric wrap preserved;
- horizontal repeat at 8 update calls;
- confirm only on new press of action mask `$0030`;
- global timeout `$0700` update opportunities, not reset by input;
- timeout leaves unfinished record bytes blank/current as in the arcade table.

`wf_port_services` now owns initials entry and performs a second Backup-RAM persistence attempt when entry completes or times out, so the final table state is not lost after the initial qualifying insert.

## Referee count runtime
Added `wf_referee_count.c/.h`, modeling ROM `$20022` without inventing a universal pin-start phase:
- preserves inherited `+$25` on begin;
- initializes `+$23=$14`, selector `$05=4`, sound `$54=$3162`;
- exact byte countdown to expiry;
- each expiry increments `+$25` and linked-chain `+$109`;
- even phases increment displayed count and sound command;
- selector/timer alternation is `5/32` and `6/56` according to the ROM branch;
- phase6 adds `$10` to the selected timer and exposes a phase6 event for mode-specific linked-actor side effects;
- phase7 terminal is primary state9 in TAG and state5 in Rumble;
- primary-state6 cancel rule is exact: cancel iff `+$25<6 && linked +$35.bit0==0`.

The runtime deliberately does not claim an absolute pin-onset -> 1/2/3 timeline because `$20022` does not reset `+$25`.

## Installer hardening
`tools/apply_overlay.py` now uses no-clobber semantics:
- absent file -> copy;
- byte-identical existing file -> accept;
- differing existing canonical file -> abort with `CONFLICT`;
- never silently replaces newer gameplay/audio/frontend code;
- remains idempotent on repeated application.

A deliberate-difference test confirms the conflict guard.

## Validation
Unified host suite with `-std=c11 -Wall -Wextra -Werror` PASS:
- Records ranking/WFRK
- Records initials + pixel layout
- Credits state/timing
- attract biography compile
- A44 router/runtime
- A49 exact metadata
- A49->A44 exact adapter
- referee/announcer tables
- announcer exit runtime
- referee inherited-phase count program
- port services boot/records/initials/credits
- Presentation contract compile

## Package
`WWF-Saturn-Test255-WIP171-integration-overlay.zip`
SHA256 `2bafb5f4b7af8fb64f4d47a5b4f8c03ee314aebe36c5410b7e3409e1b07b89ed`
Archive integrity PASS. No arcade ROM, reconstructed program, BIN/CUE or ISO included.

## Safety
- no WIP160+ gameplay/AI/animation file replaced;
- no global held release enabled;
- WIP168 raw23 release remains context contract only until canonical source is materialized;
- `tick==12` and 32-tick held fallback unchanged;
- no invented announcer facing mutation;
- no invented referee pin-start phase.

## Next
Apply this overlay to the latest materializable WIP160+ source when available. Then wire `wf_port_services` into boot/frontend, connect exact Credits renderer events, bind announcer selectors to recovered pose rendering, and map referee count phase events to the existing actor/render/audio fields. AI and animations are reconciled numerically against FINAL contracts rather than overwritten wholesale.
