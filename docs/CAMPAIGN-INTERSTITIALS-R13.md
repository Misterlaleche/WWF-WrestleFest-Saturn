# Campaign Interstitials R13

Authority: WP13 CAMPAIGN/INTERSTITIALS ROM-EXACT FINAL.

Master-side integration contract implemented without importing the superseded synthetic ten-match ladder.

Closed:
- post-win ordering: B608 gate -> BDA6 gate -> BYTE progression increments -> AC0 prematch handoff;
- B608 exact gate: TAG only and WORD $1C0162 == 4;
- B608 natural duration remains runtime/callback driven and is not replaced by a constant;
- BDA6 Royal rejection and exact selector arithmetic including optional $1C0066.bit7 +1;
- accepted raw selector set {0,1,2,5,6,7};
- normalized frame bases 4,11,18;
- exact action expiry sequence 3,1,2,1,0 with reload 10;
- phase/frame 2 emits raw command word $312A and activates card;
- card expiry reload 8, six frame advances, then final frame hold until +$24==16;
- synchronized BDA6 loop 142 iterations;
- exact 24-VBlank fade after loop;
- no demonstrated skip input.

Important implementation separation:
The FINAL proves callback-driven object state and the total synchronized-loop count. R13 exposes object countdown-expiry transitions and the 142-loop/fade contract separately instead of inventing an unproven Saturn scheduling micro-order.

Audio:
$312A is emitted only as the exact raw command word. Its human-readable sample meaning remains REGISTERED_NOT_IDENTIFIED.

Still OPEN:
- B608 full presenter object implementation and its callback-produced terminal bit $1C14F8.bit6;
- renderer bindings for AE20(D0=3) initializer and gfx-set $4F objects;
- exact config-driven progression skip adjustment after BYTE increments;
- full merged post-match owner;
- hardware validation.

No BIN/CUE generated.
