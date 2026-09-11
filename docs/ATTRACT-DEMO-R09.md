# Attract Demo Master Contract R09

Authority: WP17 ATTRACT DEMO GAMEPLAY ROM-EXACT FINAL + original ROM hierarchy.

This master-side module does not implement a second gameplay engine. It owns the exact 14-state presentation contract and normal threshold exit cadence.

Closed in this candidate:
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

Hard architecture rule:
- no synthetic joystick/button autoplay;
- ordinary gameplay loop $000F8C + ordinary CPU remains the engine;
- gameplay branch must implement $01F7F0 equivalent at $01C4EE/$01CC2C/$01CC92/$01F084;
- state5 selector output flows through ordinary state5 execution;
- shared $21B4/$24CC RNG service/order must be canonical before exact demo choreography can be claimed;
- Bio overlay shares that RNG stream.

Observed gameplay dependency while R09 was built:
BLOCK-GAMEPLAY-MASTER-01-RUNTIME-13 at commit 91f4c697bec0b04678cc1e811b4ee60e606d7131.
It links cleanly but still fails closed at mandatory WP28 phase $02088E; direct match/bootstrap and canonical shared RNG service are not closed there. R09 does not merge gameplay or fabricate those gaps.

No BIN/CUE generated. Hardware PASS not claimed.
