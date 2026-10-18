# Independent R17 closure review

Reviewed recovered runtime source directly and disassembled `work/recovery/main68k.bin`. Paths below are relative to `work/recovery/live/BLOCK-GAMEPLAY-MASTER-01-RUNTIME-17-7A00-OWNER-INTEGRATED-AUDIT-PASS`.

## Verdict

The scheduler has actual native phase bindings for both A and B, VBlank ACK, mandatory next-iteration RNG, and newly restored Royal $2017A. This establishes a bounded native owner integration. Passing existing A/B tests does **not** establish full active gameplay or complete D0–D7 composition. Closure must explicitly retain the external WP06/WP07 gameplay joins; these are not render callbacks. Full standalone gameplay remains blocked.

## Demonstrated defects repaired during review

- `src/wf_r17_fixed_four_10120.c:97`: lacked all shadow updates. ROM $10130/$1013C–$10144/$10158 leaves D1.low=$0010 and D2.low=last state's index*4. State1–4 sample fetches write D0.low=animation index*2; timer/terminal exits preserve D0. $247C saves D0–D2, $27B8 saves D0 and $2188 touches no D registers. $2052 saves D0 and touches no other D registers. These are now implemented with upper-half preservation, callback register preservation and rollback. This matters because $7A00 can immediately consume D2 through $21B4.
- `src/wf_r17_transient_reset_10e6a.c:3`: omitted terminal D0.low=$FFFF from ROM $10E70/$10E80. Added exact WORD update.
- Dedicated tests now cover all five fixed-object state paths, high halves, preserved registers and transient-reset register exit. Parent build agent must run these after this delta; no passing claim is made here.

## Material limits still visible in code

1. `src/wf_r17_post7a00.c:1397–1415,1479`: $18C4 intentionally delegates $6E34 and $9132 to WP06/WP07. `src/wf_gameplay_bindings.c:220–221,326` copies optional callbacks from a context initially cleared to zero. No native implementation of either join exists in recovered source. `tests/test_runtime17_branch_a_e2e.c:11,40` substitutes a successful gameplay no-op for $6E34. The matching WP06/WP07 FINAL ZIPs inside `sources/zips finales.zip` contain no .c/.h source to restore. These are explicit contract exclusions, not accidental unbound top-level phases; they still prevent a claim of complete normal TAG gameplay.
2. `tests/test_runtime17_branch_a_e2e.c:42–54` and branch-B counterpart use CPU state7 and flags32.bit7 to suppress wrestler action dispatch. The tests prove gated scheduler order and ACK plumbing, not active combat. They assert RNG changes, not the exact ROM-derived D0–D7/RNG tuple after a real complete frame.
3. The demonstrated $F4C2 own-register omission was also repaired during review: `src/wf_r17_scheduler_owners.c` now models $250E iterator D0.low (terminal9), correction D1.low writes, and $1C03E D0 preservation. Dedicated regression covers empty scan and real resolver plus RTS state4 child. This bounded repair does not prove D1–D7 exits of every dispatcher action child; those remain child-owned and need a complete clobber matrix before a global composition claim.
4. `src/wf_scheduler.c:84–96`: blocked pre-ACK phase leaves frame_stage zero. Retrying repeats $FA0 RNG and preceding phases after their mutations. Thus WF_BLOCKED before frame-ready is a terminal unresolved-dependency condition, not a resumable normal-frame state. ACK waiting itself is correctly resumable. Tests/integrators must not retry unresolved phases as if they were VBlank waits.
5. `src/wf_saturn_entry.c:14–34` is explicitly link-validation entry and returns WF_BLOCKED. It does not run native frames or provide bootstrap/input/renderer integration. This is acceptable for a source/link gate, not evidence of a runnable Saturn gameplay image.

## $2017A delta review

Direct disassembly $2017A–$202B8 matches the implementation's gates, descending status/slot scans, nine actor scan, overlapping status writes, D5 selected pointer, activation of first free slot4–7, and D7 preservation. $206FE is raw video clearing with MOVEM D0/A0, so an external render callback is appropriate here. The owner currently sets bit2 before blocking on a missing clear callback, matching the ROM instruction order; callers must treat the missing dependency as terminal. Focused tests cover key scans and aliases but do not by themselves cover every missing/failing callback, no-free-slot and already-set-bit2 path.

## Owner boundary classification

- Native top-level owners: actual A/B phase table and bindings, including $2017A.
- Allowed external presentation: video/FG0 batches, renderer/resource transfers, audio sink, $206FE clear. Their callbacks must meet explicit register contracts.
- External gameplay: $6E34/$9132; referee phase6/result joins; dispatcher child/action dependencies. Missing callbacks fail closed. A recording/no-op callback can test invocation but cannot close its gameplay semantics.
- Scene transitions after ACK are explicitly fail-closed in scheduler; normal return-to-$F9A is implemented.
