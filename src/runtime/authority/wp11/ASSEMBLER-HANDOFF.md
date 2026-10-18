# ASSEMBLER HANDOFF — MATCH-RULES-WIN-LOSS-FLOW

## Authority

- Main 68000 SHA1: `a726c46f350723ea73158bc78055d17d418b14e2`
- Set authority: `wwfwfest.zip`
- FINAL dependencies:
  - Referee/Announcer ROM-EXACT FINAL
  - Records ROM-EXACT FINAL
  - Credits ROM-EXACT FINAL
  - Presentation ROM-EXACT FINAL
  - victim `+$65` ROM-EXACT FINAL where actor ordering intersects
  - Audio FINAL for sound reproduction semantics
- WIP160: historical/donor only; provisional finish enums are **not** authoritative.

## Implementable now

- TAG end detector: actor0/actor2 result `+$A0 & $C000` -> post-match `$11B6` equivalent.
- Result marker semantics in closed paths: high `$40` winner-side, high `$80` loser/no-winner side.
- TAG pin: referee terminal count -> `+$FE` result roles -> exact recovery/choreography -> result markers.
- Royal pin: increment credited elimination `+$C4`, elimination flags, **do not finish match directly**.
- Count-out: terminal at 20; explicit BOTH branch before single-side branches.
- Royal survivor state machine equivalent to `$2017A`.
- Royal last survivor -> winner choreography -> only then win latch -> Credits.
- Royal zero eligible survivors -> cleanup latch -> Records qualification -> frontend.
- TAG continuation / loss split at `$19BA` semantics.
- TAG campaign advancement: gate 9, `++$1C0163`, `++$1C0165`, optional DIP skip at resulting 3/8, roster snapshot at resulting 5, next pre-match.
- Shared Credits -> Records qualification -> frontend.
- Actor snapshot/restore semantics from `$0D1C/$0D80`.
- `$90D6` recovery table `{24,32,12,0}` at exact call sites only.

## Must remain fail-closed

Do **not** implement as ROM-EXACT until stronger evidence exists:

- standalone submission victory/defeat and any drain/escape threshold;
- time-limit finish;
- DQ;
- generic DRAW state separate from the proven double-count-out no-winner path;
- KO/double-KO finish reason;
- cage-escape finish;
- retry-same-match / continue-after-loss node inside terminal match flow;
- guessed score reset/preserve policy.

If legacy code exposes these as finish enums, leave them unreachable from ROM-exact gameplay unless another FINAL package later supplies exact evidence.

## Exact tables / constants

- mode: `$1C0161.bit0`: 0 TAG, 1 Royal.
- TAG top result mask: `$C000` on representatives' `+$A0`.
- outside count terminal: `$14` = 20.
- pin referee terminal phase: `+$25==$07`.
- recovery table at `$912A`: words `$0018,$0020,$000C,$0000`.
- campaign Credits gate: `$1C0163==$09`.
- campaign optional skip: `$1C0066.bit7` and new progress `$03/$08`.
- roster snapshot: resulting `$1C0163==$05`, destination `$1C0598`.
- Royal loss top gate: `$1C16C5==$03`.
- Royal win top gate: `$1C16C4!=0`.
- raw action `$8E` final win-latch countdown: `$0200` in callback `$1AF7E`, writer `$1AFCA`.

See `tables/` for machine-readable forms.

## Required actor/global fields

Preserve byte/word width exactly:

- actor `+$00`, `+$20`, `+$25`, `+$26`, `+$32`, `+$33`, `+$4A`, `+$56/+57`, `+$66`, `+$68`, `+$6A`, `+$70`, `+$72`, `+$7A`, `+$86`, `+$8A`, `+$A0`, `+$AE`, `+$C4`, `+$FE/+FF`, `+$104`, `+$109` where consumed by these flows;
- globals `$1C0066`, `$1C007C`, `$1C0092..$9E`, `$1C0156`, `$1C0161`, `$1C0163`, `$1C0164`, `$1C0165`, `$1C1586`, `$1C169A/$169B`, `$1C169E`, `$1C16C4`, `$1C16C5`.

## Ordering/timing requirements

- Do not convert referee count phases into one fixed duration from pin-start.
- `$90D6` recovery executes inside specific referee result paths before final post-match transition.
- TAG only leaves gameplay after result marker reaches `+$A0` and the top-loop detects it.
- Royal pin elimination is resolved before the later `$2017A` population scan closes the match.
- Royal win Credits gate is delayed until raw `$8E` finishes and writes `$1C16C4`.
- `$65C0` is conditional ranking qualification; no initials screen if no candidate qualifies.

## No-clobber requirements

- Never treat `+$A0` as a byte-only field. Rebuild restores word then masks with `$000F`.
- Never clear stamina on generic next-match transition; preserve `$66/$72/$70` according to snapshot/restore and apply only proven recovery.
- Preserve wrestler ID low byte `+$57` when clearing high byte at `+$56`.
- Keep `+$FE` result role and `+$A0` result marker distinct.
- Keep `$1C16C4` and `$1C16C5` distinct.
- Do not collapse `$65C0` and `$06FC` into a guessed “game over screen”.

## Required integration tests

1. TAG pin: terminal count produces one winner side and one loser side, then exits through TAG aftermath.
2. Royal pin: increments exactly one credited `+$C4` on the pinned wrestler and does not immediately end match when >1 eligible survivor remains.
3. Count-out at 19 does not close; at 20 does.
4. Both-side count-out selects `$8005/$8005` path before either single-side path and yields no `$40` winner marker.
5. Single-side count-out selects `$8002/$8003` orientation correctly in both directions.
6. `$90D6` table all four indices; clamp live stamina to max; gate on `+$FF.bit0` exactly.
7. Royal survivor scan cases: D3=0, D3>1, D3=1/D4!=0, D3=1/D4=0.
8. Royal last survivor does not hit Credits before winner action `$8E` sets win latch.
9. TAG campaign: Credits check at progress 9 happens before increment.
10. TAG normal advance increments `$1C0163/$1C0165`; DIP skip only at resulting 3/8; roster snapshot only at resulting 5.
11. `$0D1C/$0D80` equivalent preserves fields and clears result high bits with exact `$000F` mask.
12. Records qualification may return without initials; terminal caller still reaches frontend.
13. Negative tests: submission/time/DQ/double-KO/cage-escape legacy finish enums cannot be entered from ROM-exact state machine without a new authoritative package.

## Runtime-dependent items

- active referee action in live ambiguous scenarios;
- multi-elimination Royal final population state;
- exact qualification outcome at `$65C0`;
- reachability of outside-count paths in each Royal subcontext.

## Not provable from current evidence

See `UNRESOLVED.md`. These are fail-closed classifications, not blockers.
