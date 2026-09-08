# WIP136 CHECKPOINT — raw0B chained contact timing is fraction-invariant

## Scope
ROM-EXACT numerical closure of raw `$0B` floor-contact timing for the WIP132 chained-Ground provenance where starting high-word Z is exactly the current floor threshold T, probe42 is zero, and actor fractional `+$10` may be arbitrary. Research/checkpoint only; no live release/timing change and no BIN/CUE.

## Exact launch
Raw `$0B` first-entry callback calls `$258E` with motion profile `$23` at `$13A9A`.
This initializes:

```text
+$5C = $0400   ; +4 px/pass initial vertical velocity
+$5E = $0058   ; 88/256 = 0.34375 px/pass² gravity
+$01 = 2       ; 32-bit XYZ integration mode
```

By WIP135, the callback executes before `$2208`, so movement step 1 occurs at the end of the launch actor pass. Each later actor pass evaluates floor/contact before applying the next movement step.

## Closed recurrence
For n completed movement integrations after launch, displacement relative to starting Z is:

```text
Delta(n) = [ n*$0400 - $0058*n*(n-1)/2 ] / 256 pixels
```

with the full 16.16 accumulator retaining arbitrary initial fractional low word F in `[0,$FFFF]`.

For n=24:

```text
24*$0400 - $0058*(24*23/2) = 288
Delta(24) = +1.125 px
```

Even with the smallest fraction F=0, the high/integer word is still at least T+1. Therefore the floor candidate is not below T and contact37 remains clear.

For n=25:

```text
25*$0400 - $0058*(25*24/2) = -800
Delta(25) = -3.125 px
```

Adding any possible fractional F<1 px still leaves the 32-bit Z strictly below T. Therefore on the actor pass immediately following movement integration 25, the `$280DC` floor branch necessarily sets `+$37.bit4` and corrects the high word back to T.

## Consequence
For chained raw `$0B` entry with exact starting high Z=T and probe42=0:

- fractional `+$10` cannot shift the contact turn;
- there are exactly 25 vertical movement integrations from launch to the first guaranteed below-threshold state;
- the next pre-callback geometry pass necessarily produces contact37.

This closes the subpixel sensitivity for this specific provenance/action pair. It does **not** authorize global raw `$0B` release because first-entry provenance outside WIP132 remains open, and runtime still lacks the 32-bit shadow/integrator required for general correctness.

## ROM anchors
Raw0B init/contact `$13A9A..$13AA8`; profile loader `$258E`; common movement `$2252..$22BE`; actor ordering `$F518..$F55E`; floor resolver `$280DC`.

## Safety
No runtime change. No physical release enabled. No held fallback removed. No `tick==12` migration. No ROM bytes persisted.

Main program SHA1: `a726c46f350723ea73158bc78055d17d418b14e2`.
