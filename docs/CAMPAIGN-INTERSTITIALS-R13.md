# Campaign Interstitials R13 — A01 corrected

Authority: original `wwfwfest.zip` ROM > WP13 CAMPAIGN/INTERSTITIALS ROM-EXACT FINAL.

Master-side integration contract implemented without importing the superseded synthetic ten-match ladder.

## Exact post-win control order

Direct main-ROM audit closes the following order:

1. Before any interstitial, compare BYTE $1C0163 with 9.
2. If equal, branch to final completion $001B4E. B608/BDA6 are not called.
3. Otherwise JSR $00B608.
4. JSR $00BDA6.
5. ADDI.B #1,$1C0163.
6. ADDI.B #1,$1C0165.
7. If $1C0066.bit7 is set and the newly incremented $1C0163 is 3 or 8, ADDI.B #1,$1C0163 once more.
8. If resulting $1C0163 == 5, prepare the demonstrated table at $1C0598 from four actors beginning $1C05B0, stride $10C, using active gate +$00.bit7 and actor byte +$57.
9. Handoff to $00000AC0.

R13 now fail-closes the progression==5 preparation as an explicit owner-completion wait instead of silently advancing before the table is materialized.

## B608

- TAG only: $1C0161.bit0 == 0.
- WORD selector $1C0162 == 4.
- natural duration remains callback/runtime-driven;
- terminal condition remains $1C14F8.bit6;
- no fixed duration or skip is invented.

## BDA6

- Royal rejected;
- optional selector adjustment by $1C0066.bit7;
- accepted adjusted selectors {0,1,2,5,6,7};
- normalized frame bases 4,11,18;
- action frames 3,1,2,1,0 with reload 10;
- action frame/phase 2 activates card and emits raw command word $312A;
- card reload 8, six frame advances, then final-frame hold until +$24==16;
- synchronized outer loop 142 iterations;
- exact 24-VBlank fade after loop;
- no demonstrated input skip.

Audio:
$312A remains an exact raw producer token only. Human-readable sample meaning remains REGISTERED_NOT_IDENTIFIED and no direct WP24 dispatch is invented.

Still OPEN:
- B608 full presenter-object callback implementation;
- renderer binding for AE20(D0=3) and gfx-set $4F objects;
- application of the progression==5 actor-table preparation by the final actor/global owner;
- full merged post-match owner;
- hardware validation.

Validation after A01 correction:
- host test PASS with -Wall -Wextra -Werror;
- SH-2 freestanding compile PASS;
- no BIN/CUE;
- hardware PASS not claimed.
