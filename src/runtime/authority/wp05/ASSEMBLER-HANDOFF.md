# ASSEMBLER HANDOFF — GROUND-HELD-RELEASE-FINAL

## Authority
- Main 68000 SHA1: `a726c46f350723ea73158bc78055d17d418b14e2`.
- Dependent FINAL authority: victim `+$64/+$65` FINAL research/package.
- WIP160 is evidence/history only where ROM-pinned and consistent.

## Implementable now
- Replace held Ground timeout logic with descriptor/callback-driven release.
- Treat `$FF00` as frozen until callback mutation.
- Preserve order: motion/resolver -> contact37 -> callback -> common descriptor updater.
- Apply callback `+$42` writes to the next resolver pass.
- Preserve 32-bit Z (`+$0E:+$10`) and word-copy semantics.
- Carry `+$01/+$5C/+$5E` as actor fields and apply exact writer tuples only on proven callback events.
- Implement linked victim retype `08->06`, `09->07` for raw `$0A/$0B/$10`, plus state4 return `06->08`, `07->09` at `+$25==$FE`.
- Implement raw `$26` held frame: phase3 `$FF00`, `+$25==3`, contact37 gate `$162D6`, callback clears `+$01` and `+$22`; no timeout.
- Respect raw `$26` linked victim WORD `$0005` producer at `$1630C` and let victim FINAL logic decide later `$08/$09`.

## Must remain fail-closed
- Raw `$23` `+$44&0F` selector indices `$09..$0F` unless a future reachability proof is supplied.
- Any helper-dependent held branch when the required helper carry is not validity-tagged.
- Any inherited Z/probe/actor32 route whose predecessor event is not known.

## Exact constants / tables
- See `tables/held_actions.csv`, `tables/held_gates.csv`, `tables/motion_writers.csv`.
- Floor/contact thresholds: `$0140` for `$2818E/$2831C`; `$0100` for `$28288/$2851E`.

## Required actor/global fields
- action/main-state and action raw ID;
- `+$1C.bit7` equivalent initialization latch;
- phase/counter equivalents of `+$22/+$24/+$25`;
- `+$32` with byte-accurate bit2 preservation;
- `+$37.bit4` current-turn contact result;
- `+$42` signed probe;
- `+$44` chained-Ground selector data;
- 32-bit Z `+$0E:+$10`;
- vertical `+$5C`, gravity `+$5E`, motion selector `+$01`;
- linked actor relation and victim state4 word `+$64/+65`.

## Ordering/timing requirements
1. Integrate existing motion.
2. Run floor/contact resolver and produce current-turn contact37.
3. Apply resolver correction.
4. Run callback.
5. Callback may write probe/motion/counter/state.
6. Run common descriptor updater if callback did not replace the action/main state.
7. New callback probe/motion affects the next resolver/motion pass as dictated by ROM ordering.

## No-clobber requirements
- Do not zero `+$32.bit2` on arbitrary state changes.
- Do not clear victim `+$64/+65` generically.
- Low-byte victim retypes preserve high flags; WORD writes replace high+low.
- Do not zero inherited `+$42` or Z unless the exact predecessor callback does so.
- Do not normalize unknown helper carry to false/clear.

## Required integration tests
- `$FF00` with no callback release remains indefinitely held without a native timeout.
- Contact37 clear leaves every held phase frozen.
- Raw `$0B/$14/$23/$26` contact path clears/advances at the exact phase.
- Raw `$13` contact helper path writes `$0018` and same-pass common updater yields `$0017` residency semantics.
- Raw `$0E` retains three distinct held occurrences; do not collapse them.
- Callback `+$42` write does not affect the contact test already performed in that pass.
- New `+$5C/+$5E/+01` tuple begins affecting Z only after the callback pass that writes it.
- Raw `$26` victim state4 WORD `$0005` is preserved as such; do not preselect `$08/$09`.
- Raw `$0A/$0B/$10` victim low-byte transforms preserve high `+$64` flags.
- Unknown raw `$23` chained selector fails closed in debug/tests rather than picking a fallback branch.

## Runtime-dependent
- helper-carry branches;
- predecessor-specific actor32/Z/probe values;
- actor-slot-sensitive victim critical latch and subsequent `$05->$08/$09` result.

## Not provable from current evidence
- raw `$23` `+$44` selector `$09..$0F` runtime reachability.
