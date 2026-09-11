# Continue / Game Over R10

Authority: WP14 ROM-EXACT FINAL.

Implemented as a master-side, credit-backend-neutral controller:
- pair A then pair B sequential processing;
- valid participation masks 1/2/3;
- request code 1 for single participant, 3 for mask 3;
- exact pair input-word mapping;
- current active-low START level, not fresh-edge;
- exact countdown 9..0, 64 iterations each, 640 total;
- render/update -> live credit availability -> START level -> spend -> expiry ordering;
- START can accept on final iteration of digit 0 before expiry;
- credit availability and spend remain separate callbacks;
- dynamic no-credit/player prompt visibility and exact text IDs;
- acceptance emits exact global/pair marker semantics and same-campaign retry semantic;
- expiry emits 0x10C-byte actor reset with +0x56 word preserve contract;
- normal Game Over waits 65+65; Royal terminal wait 97.

The controller does not implement $C404/$55A; those remain WP19/operator/payment authority and must be bound without reducing them to credits>0.
It does not mutate campaign stage counters, actor memory or renderer directly; it emits integration events to their owners.

No Free Play is invented. No 600-frame/10-second timer exists. No BIN/CUE generated. Hardware PASS not claimed.
