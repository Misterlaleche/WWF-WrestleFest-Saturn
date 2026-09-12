# Attract Demo Master Contract R09

Authority: original `wwfwfest.zip` ROM > WP17 ATTRACT DEMO GAMEPLAY ROM-EXACT FINAL > WP15 FRONTEND TRANSITION ENGINE FINAL.

This master-side module does not implement a second gameplay engine. It owns the exact 14-state presentation contract and normal threshold exit cadence.

Closed:
- all 14 state IDs, modes, actor counts, lineups and thresholds;
- state 01 Royal/Rumble classification and six actors;
- state 00 special two-entry override policy marker;
- state 02..0D 12-character override policy marker;
- state 01 override-disabled marker;
- bio featured IDs for 02..0D;
- special setup markers for 01/02 and $8490 pre-hook states 06/0A;
- one counter increment per synchronized gameplay update;
- exact normal transition cadence 24 + 33 + 33 = 90 VBlanks;
- next-state increment after 90 VBlanks;
- state 0D returns to $06FC;
- credited START is external to demo gameplay and belongs to global credit/start handler ($52BE on accepted START).

## Audit correction A01

The original R09 event named the first 24-VBlank stage as a generic palette fade. That label was wrong.

Direct ROM at $001120 calls JSR $008F50. WP15 FINAL proves $008F50 is the stripe/tile mask-close routine using tiles $4BE..$4C5 at three VBlanks per step, not the common palette fade routine $0264E2.

ROM authority therefore resolves the first stage as:

`$008F50 stripe-mask close -> 24 VBlanks -> wait 33 -> wait 33`.

The 90-VBlank total was already correct; the renderer event semantics have been corrected to emit the exact $008F50 stripe-mask close.

Hard architecture rule:
- no synthetic joystick/button autoplay;
- ordinary gameplay loop $000F8C + ordinary CPU remains the engine;
- gameplay branch must implement $01F7F0 equivalent at $01C4EE/$01CC2C/$01CC92/$01F084;
- state5 selector output flows through ordinary state5 execution;
- shared $21B4/$24CC RNG service/order must be canonical before exact demo choreography can be claimed;
- Bio overlay shares that RNG stream.

Current gameplay dependency:
BLOCK-GAMEPLAY-MASTER-01-RUNTIME-15, source checkpoint 72de727a77603c0a81f552fa059c0d2072cd249b; gameplay branch documentation HEAD 7f40fde30cb7af32cf95cd4d9469884a342da8b8.

RUNTIME-15 links cleanly, closes WP28 $020958 and binds audited WP21 camera phases $026936/$02983C. The next fail-closed phase is $00F560. Direct match/bootstrap and canonical shared RNG service remain unclosed.

No BIN/CUE generated. Hardware PASS not claimed.
