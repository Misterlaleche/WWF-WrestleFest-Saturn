# ASSEMBLER HANDOFF — WORKPACK 08

## Authority
- Main 68000 SHA1: `a726c46f350723ea73158bc78055d17d418b14e2`.
- Depend on Animation FINAL, victim65 FINAL, Referee/Announcer FINAL and Audio FINAL.
- WIP160 ringside interpolation, 24-tick re-entry lock, camera constants and provisional ringside-combat rules are superseded as implementation authority.

## Implementable now
- Preserve actor `+$33.bit2` as logical inside-ring state.
- Implement raw `$69` exit and raw `$6C` entry with exact descriptor schedules/callback gating.
- Implement ring-out producer pair: victim state4 `$0E` + damage `$0008`, linked aggressor raw `$06`.
- Implement exact raw `$06`, state4 `$0E`, `$68`, `$6A`, `$6B`, `$69`, `$6C` schedules/state transitions where callback predicates are available.
- Implement TAG referee count-out: 80 updates/count, terminal 20, using Referee FINAL result logic.

## Must remain fail-closed
- Any `$FF00` hold without its actual callback release predicate.
- Outside move whitelist, Ground rules, stamina special-casing or CPU ringside policy not proven by their owning workpacks.
- Universal exterior bounds derived from WIP160 clamps.
- Ring/outside camera interpolation or separate background swap before WP21/25/26.
- TAG partner/apron special choreography before WP06; Royal elimination/entry semantics before WP07.

## Exact tables / constants
- `tables/transition_actions.csv`
- `tables/raw_action_producers.csv`
- `tables/ringout_toss_producers.csv`
- `tables/countout_constants.csv`
- `tables/resolver_geometry.csv`

## Required actor/global fields
- actor `+$20` main state;
- actor `+$25` callback phase;
- actor `+$33.bit2` inside-ring flag;
- actor `+$36/+37` contact resolver output where callback requires it;
- actor `+$60` raw action word;
- actor `+$64/+65` state4 word/subtype;
- actor `+$68` pending damage;
- globals `$1C169A/9B` outside count.

## Ordering/timing requirements
- raw `$6C` sets inside bit on initialization path; do not wait for visual completion.
- raw `$69` clears inside bit at its proven callback phase; do not clear at action assignment unless ROM path proves it.
- finite raw durations are underflow-inclusive (`raw N => N+1` updates).
- `$FF00` is callback-held, never a numeric timeout.

## No-clobber requirements
- Modify `+$33.bit2` bitwise; do not overwrite unrelated `+$32/+33` flags.
- Preserve victim65 byte/word semantics for `+$64/+65`.
- Preserve transition metasprite bank14 rope pieces and per-piece ordering.

## Required integration tests
1. ROM hash check passes `tools/verify_ringout_rom.py`.
2. All four ring-out producer pairs produce victim state4 `$0E` and aggressor raw `$06`.
3. raw `$69` clears only `+$33.bit2` at the proven phase.
4. raw `$6C` sets `+$33.bit2` on initialization and holds at pose43 until callback release.
5. No fallback tick releases state4 `$0E`, raw `$68`, or raw `$6C` holds.
6. TAG count increments exactly every `$50` referee updates and terminal result occurs at `$14`.
7. Royal path never starts the TAG count-out state machine.
8. Renderer preserves mixed bank14/wrestler transition composition.

## Runtime-dependent items
- live resolver route/mode for every outside actor state;
- complete producer reachability across all AI/TAG/Royal contexts.

## Not provable from current evidence
- universal outside bounds;
- exact rope-crossing trajectory/parabola/apron plane;
- outside-combat whitelist;
- standalone RING OUT background swap;
- camera/scroll transition curve.
