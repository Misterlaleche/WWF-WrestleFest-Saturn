# ASSEMBLER HANDOFF — DAMAGE-STAMINA-RECOVERY

## Authority
- Main 68000 SHA1: `a726c46f350723ea73158bc78055d17d418b14e2`
- Dependent FINAL packages: victim `+$65` FINAL; CPU FINAL bridge; Records FINAL.
- Superseded: contradictory WIP/native recovery-reset, regeneration-tier, `$90D6` REGAIN labeling, max-zero classifier, difficulty/damage heuristic statements documented in `AUDIT-LOG.md`.

## Implementable now
- Actor stamina fields `+$66/+68/+6A/+6C/+6E/+70/+71/+72/+106` with exact widths.
- `$24E58` pending-damage processor: defer on `+$FE!=0`, unsigned saturating subtract, classifier call, negative HUD queue, pending clear.
- `$24EC2` exact tier classifier including max-zero preserve behavior.
- Scheduler order in `tables/timing-order.csv`.
- Stamina initialization tables and difficulty deltas.
- Royal `$8ECC` recovery with pausing `+$106`, threshold 128, +1 live/+1 HUD queue, no tier recompute.
- State1 `$11796/$1192C` +1 recovery with 148 callback-period table, no tier recompute.
- Cabinet full-refill `$8B3A/$8C3E` semantics and Clear Stage `$90D6` semantics.
- `$10C60/$10D04` numeric timer mechanics/tables.
- Direct pending-damage constants/modifiers from `damage-writers.csv`; only attach human move names where another FINAL/mapping proves the context.
- `$1E48C` stamina bucket only in that CPU context; preserve `+$BD` as reusable byte storage globally.

## Must remain fail-closed
- Human-readable meaning for every raw action damage writer not already mapped by canonical evidence.
- Universal move/ground/throw semantics for each `$10C60` D0 mode.
- Any global `stamina==0 => knockdown/KO` transition: the central damage path contains none.
- Any global difficulty damage multiplier or AI-speed multiplier.
- Any total/cadence calculation for hold pulses from a single +$68 write.
- Any generic score/Records relation inferred from damage-adjacent counters.

## Exact tables / constants
- Critical tier: live `<=24` when max nonzero.
- Warning branch: live `<24`.
- Royal natural recovery: +1 at eligible counter `+$106 >=128`, then counter clears.
- State1 local period table: 12 × WORD `148`.
- Royal fixed CPU stamina path: `135`.
- All remaining initialization/difficulty/power-up/timer tables are in `tables/` and verified by `tools/audit_damage_stamina_recovery.py`.

## Required actor/global fields
- Preserve `+$66/+68/+6A/+6C/+6E/+70/+71/+72/+AA/+AB/+106/+BD` with exact widths and no generic renaming that changes storage semantics.
- Preserve `+$FE` as the pending-damage defer gate used by `$24E58`.
- Preserve victim `+$64.bit7` critical latch rules from the victim65 FINAL.

## Ordering/timing requirements
1. pre-callback cabinet refill check;
2. actor callbacks;
3. collisions;
4. pending damage + classifier;
5. low-stamina warning;
6. Royal natural recovery.
- Do not recompute tier after recovery unless a proven callsite later invokes `$24EC2`.
- Actor-slot ordering effects on victim state4 critical latch remain runtime-dependent and must be preserved rather than flattened.

## No-clobber requirements
- Do not clear Royal `+$106` merely because a frame is ineligible; pause it.
- Do not clear pending `+$68` while `+$FE!=0` in the central processor.
- Do not overwrite a positive recovery HUD queue with an additive abstraction; original paths distinguish MOVE vs ADD. Central damage uses MOVE then NEG.
- Do not turn `+$71` into an independent byte field; it is the big-endian low byte of WORD `+$70`.
- Do not globalize `+$BD` as stamina bucket.

## Required integration tests
- SHA1 verifier passes against user-owned ROM.
- Pending damage below live, equal to live, and above live saturates correctly; `+$FE` defers unchanged pending.
- Classifier tests at live 25, 24, 23 and max-zero preservation.
- Warning distinguishes 24 from 23.
- Royal recovery pauses counter across excluded frames and resumes; 128th eligible increment heals exactly +1; no automatic tier change.
- State1 recovery respects max, reloads 148, writes `+$6A=1`, and does not recompute tier.
- Full refill clears tier; Clear Stage does not.
- HUD signed queue handles damage/recovery and clamps display.
- victim65 test: critical latch outcome preserves scheduler/slot-order dependence.
- CPU test: tier-indexed raw weights remain raw weights; `+$BD` reuse is not clobbered.

## Runtime-dependent items
- Pre-hit vs post-hit tier observed by a newly-entering victim state4 callback depends on actor slot order and same/next-frame execution.
- Which static pending-damage writer sites are reachable in a particular move/roster/mode situation requires the corresponding action/choreography trace.
- Tier may temporarily remain stale after a recovery/power-up path until the next proven classifier invocation.

## Not provable from current evidence
- Universal human-readable move name for every one of the 93 immediate +$68 writer sites.
- A universal knockdown/KO stamina threshold; no such transition exists in the central damage processor.
- Total hold damage or cadence inferred solely from one pulse operand.
- A generic score/Records linkage for damage-adjacent counters.
- Universal semantic labels for every `$10C60` D0 mode.
