# WIP134 CHECKPOINT — held Z rebases preserve fractional low word

## Scope
ROM-EXACT audit of actor Z rebases used by Ground/held callbacks after WIP133 established Z as the 32-bit accumulator `+$0E:+$10`. Research/checkpoint only; no live release/timing change and no BIN/CUE.

## Closed
The ROM common mode-2 integrator updates Z with a 32-bit `ADD.L` at `$2294`, so `+$0E` is the integer/high word and `+$10` is the retained fractional/low word.

Static executable audit finds no direct `CLR.W +$10(A0)` and no direct `MOVE.W #imm,+$10(A0)` site. The Ground-related position rebases write only the high word `+$0E`.

Representative exact high-word copies include:
- raw `$0E` family: target Z high-word copied to attacker at `$140B6/$1416E` on its branch paths;
- raw `$10`: target Z high-word copied at `$14266`, followed by its action-specific high-word offset path;
- raw `$13`: `$14A30 MOVE.W +$0E(A2),+$0E(A0)` followed by the action setup/offset helper;
- raw `$08`: `$13198 MOVE.W +$0E(A2),+$0E(A0)`;
- other callback-local Z copies likewise use word-width operations.

None of these word rebases replaces the low/fractional `+$10` half of the 32-bit accumulator.

## Consequence
The earlier statement that raw `$0E/$10/$13` have an exact Z rebase remains correct only for the **integer/high word**. It does not make their physical contact frame independent of predecessor history.

For every moving held Ground Action, ROM-EXACT timing requires carrying the actor's fractional `+$10` through selection/rebase and then applying the exact `+$5C/+$5E` mode-2 integrator. A 16-bit-only `raw_z_shadow` cannot reproduce this generally.

Thus the remaining physical-timing blocker is now uniform:

```text
32-bit Z accumulator (+$0E:+$10)
+ exact launch velocity/gravity
+ probe42
+ actor32 floor variant
-> current-turn contact37
```

No held raw is promoted to live physical release from a high-word rebase alone.

## ROM anchors
Common Z integrator `$2252..$22BE`, `ADD.L` `$2294`; representative Z-high writes `$13198`, `$140B6`, `$1416E`, `$14266`, `$14A30`.

## Safety
No runtime field changed. No physical release enabled. No held fallback removed. No `tick==12` migration. No ROM bytes persisted.

Main program SHA1: `a726c46f350723ea73158bc78055d17d418b14e2`.
