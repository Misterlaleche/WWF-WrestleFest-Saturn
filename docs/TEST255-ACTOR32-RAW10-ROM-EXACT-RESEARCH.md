# Test255 — actor +$32.bit2 / raw $10 — ROM-EXACT research

Main 68000 SHA1: `a726c46f350723ea73158bc78055d17d418b14e2`.

## Direct lifecycle

Exactly nine direct mutations exist:

- `$114CC` A0 CLEAR — state0 first-entry.
- `$11E04` A0 CLEAR — state6 first-entry (`$11DE0` callback; first-entry gate is `+$1C.bit7==0`).
- `$0F3BC` A0 SET — orientation-change path; old/new `+$2E` differ.
- `$1C4CA` A0 SET — gated transition, writes `+$20=2` first.
- `$1C52E` A0 SET — gated transition after `$1F7F0` carry-clear / `+$33.bit4`, writes `+$20=2` first.
- `$1CED0` A0 SET — gated transition; `$1D030/$24CC` result must be nonzero; writes `+$20=2` first.
- `$1D85A` A0 SET — raw `$75` AI/control path.
- `$1F354` A0 SET — AI state0-approach transition, writes `+$20=2` first.
- `$214A4` A2 SET — linked-role swap.

The only direct A0 consumer is `$1D938 BTST #2,$32(A0)` in the state2 CPU/control family. The SET branch reaches `$1DA00`; that path does not clear actor32.

## Raw $10 selector and callback

Raw `$10` selector is `$E382..$E406`; success loads `D0=$0010` at `$E3FC`.

It requires:

- `A1=+$7A(A0)`;
- actor `+$34.bit5==0`;
- actor `+$21` is `0` or `1`;
- actor `+$AF==0`;
- target `+$26` long is zero;
- target `+$21==4`;
- target state4 low subtype `+$65==1`;
- `abs(+$0A delta) < $10`;
- `abs(+$06 delta) < $50`;
- actor/target `+$2E` byte values equal.

The selector neither tests nor mutates actor `+$32.bit2`.

Raw `$10` descriptor is `$141D8`, callback `$141EC`. The callback also has no direct actor32 SET/CLEAR/test. Therefore actor32 is inherited locally by raw `$10`.

## Safe entry policy

- raw10 entered from state0: **actor32 known CLEAR**, because `$114CC` executes on first-entry before Ground can be selected.
- raw10 entered from controller-generated state1 (`$F228/$F2A2` provenance): **actor32 known CLEAR**; this provenance descends from state0 init and `$11652..$11C61` contains no actor32 mutation.
- raw10 entered from any other state1 provenance: **actor32 INHERITED**. Do not force clear.

There are 36 direct `MOVE.W #1,+$20(A0)` sites. State4 CPU/control alone contains `$1DEF8/$1DF42/$1DFB8`, while `$1DA7E..$1DFE2` contains no actor32 mutation. Therefore native/ROM state1 is not a synonym for actor32 CLEAR.

Exact policy:

```text
state0 entry                    -> CLEAR
controller state1 provenance    -> CLEAR
other state1 provenance         -> last proven actor32 event
unknown state1 provenance       -> UNKNOWN / blocker remains
```

`state_set()` must never mutate actor32 globally.

## Producer -> raw10 classification

- `$0F3BC`: **CONDITIONALLY REACHABLE** at the actor32-provenance layer; no mandatory clear follows the SET, but later state1 plus all `$E382` predicates are required.
- `$1C4CA`: **CONDITIONALLY REACHABLE** under the same rule; state0/state6 first-entry would cut the history.
- `$1C52E`: **CONDITIONALLY REACHABLE** under the same rule.
- `$1CED0`: **CONDITIONALLY REACHABLE** under the same rule.
- `$1F354`: **CONDITIONALLY REACHABLE** under the same rule.
- `$1D85A` raw75: **NOT PROVABLE** to raw10. Raw75 callback `$1A0AC` can route to state6 (later CLEAR at `$11E04`), state0 (later CLEAR at `$114CC`), or state2 (preserves SET). It can therefore contribute a still-SET continuing history, but the complete preserved state2 -> state1 -> raw10 choreography is not statically closed.
- `$214A4` A2 role swap: **NOT PROVABLE** to raw10 from current bounded evidence.

No SET producer is safely classifiable as universally reaching raw10.

## Raw75 consequence

Raw75 is not an unconditional history cut. Its preserving terminal route writes `+$20=2` without clearing actor32. Therefore it must not be treated as `NOT REACHABLE` merely because its other exits enter state0/state6.

## Target subtype $01

The authoritative victim `+$65` research remains separate. Direct action producer `$18BA6` (raw `$50` callback `$18B94`) writes linked A2 state4 + word `$0001`; collision SECOND S4 `$24A38` can also produce linked A2 subtype `$01`. Raw10 additionally requires target `+$26==0`, so subtype `$01` alone does not establish raw10 availability.

## Integration boundary

The actor32 blocker may be removed for the already-proven neutral raw10 provenance classes (state0 and controller-generated state1). For non-neutral state1, raw10 must remain blocked unless an exact actor32 shadow has a known last event. Unwired ROM SET events remain `REGISTERED BUT NOT IMPLEMENTED`; no native-state heuristic is authorized.
