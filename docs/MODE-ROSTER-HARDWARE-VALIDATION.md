# MODE + ROSTER hardware validation

Date: 2026-09-11  
Target: Sega Saturn NTSC-U + Terraonion MODE  
Canonical combined checkpoint: **WF-SATURN-MODE-ROSTER-INTEGRATION-TEST03**  
Reference BIN SHA-256: `c9e81bb6e7a775079ec3c2783ad06d73ad48aea560b4a0e89c78dcf51144fc05`  
External handoff ZIP SHA-256: `65daa21e89f505d6ff23efc80389145e5e32b4ada6bc8fb98df9f43417d1861a`

## Verdict

**HARDWARE-PASS for the combined isolated MODE + ROSTER frontend block.**

Validated route:

`MODE -> confirm TAG or ROYAL -> ROSTER -> navigate -> confirm wrestler`

This status makes the behavior integration-eligible. It does **not** make the isolated TEST03 harness the canonical game main loop.

## Mandatory master import invariants

1. Do **not** replace the CLEAN-R01/master `main.c` with TEST03 `REFERENCE-SOURCE/main.c`.
2. Preserve display blank/synchronization before MODE -> ROSTER CRAM/VRAM reconfiguration.
3. Preserve transparent NBG0 moving ROSTER foreground over fixed NBG2 background.
4. Preserve the resident ROSTER foreground model.
5. Confirmation updates only the selected 96x96 region.
6. Start the confirmation patch upload at VBLANK-IN.
7. Preserve composition order: resident portrait/background -> green confirmation mask -> cursor.
8. Preserve TAG/ROYAL navigation table selection from MODE result.
9. Preserve proven scroll geometry/cadence, including LEFT_SCROLL 0x0140 and 26*8 = 208 px page transition.
10. Do not reset scroll, rewrite CRAM, rebuild full NBG0, or rewrite fixed background during wrestler confirmation.
11. Bind wrestler ID only through ROM-backed roster/move mapping; do not infer from presentation slot.
12. The next state after ROSTER confirmation belongs to its own proven owner; TEST03 does not authorize an invented post-confirm transition.

## Master validation gate

After source-level import, re-run on physical Saturn:

- TAG -> ROSTER -> scroll/navigation -> confirm.
- ROYAL -> ROSTER -> scroll/navigation -> confirm.

PASS requires:
- no yellow/orange MODE->ROSTER flash;
- no foreground-disappearance confirmation frame;
- cyan -> bright-green confirmation;
- cursor remains visible;
- correct TAG/ROYAL navigation;
- background/scroll composition unchanged.

## Source package

`WF-SATURN-MODE-ROSTER-HARDWARE-PASS-HANDOFF-R01.zip`

The protected/reference assets and reference BIN/CUE remain in the external handoff ZIP and are not duplicated into GitHub.
