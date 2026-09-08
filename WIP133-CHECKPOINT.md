# WIP133 CHECKPOINT — 32-bit/subpixel Z provenance requirement

## Scope
ROM-EXACT audit of the Z integrator behind held Ground contact timing, specifically whether 16-bit actor `+$0E` alone is sufficient to reproduce the contact frame. Research/checkpoint only; no live release/timing change and no BIN/CUE.

## Closed coordinate representation
Common movement `$2208` mode2 path `$2252..$22BE` treats Z as a **32-bit accumulator beginning at actor `+$0E`**:

```text
MOVE.W +$5C(A0),D0
...
SWAP D0
ASR.L #8,D0
ADD.L D0,+$0E(A0)
```

Therefore:
- `+$0E` is the high/integer word used by the floor classifier;
- `+$10` is the low/fractional word carried by the same 32-bit Z accumulator;
- vertical velocity `+$5C` contributes at sub-word precision;
- `+$5E` is subtracted from `+$5C` each mode2 movement pass.

The floor classifier `$280DC` reads only the high word `+$0E` when building `D2 = Z + probe42`, and the correction application `$F542` uses `ADD.W` on `+$0E`. It does **not** clear/rewrite the fractional low word `+$10`.

## Reset/write audit
- State0 initialization `$114B2..$115F1` does not clear `+$10`.
- Controller-state1 family does not clear `+$10`.
- The held launch initializers audited in WIP131 set `+$5C/+$5E` and movement mode but do not clear `+$10`.
- Static executable scan finds no direct `CLR.W +$10(A0)` and no direct `MOVE.W #imm,+$10(A0)` site.

Thus a floor correction that leaves high word `+$0E=T` can still preserve a nonzero fractional `+$10` into state7→state0 and into a later chained Ground Action.

## Consequence
WIP132 correctly closes the **integer/high-word** starting Z of chained held Ground actions at threshold T, but exact future contact frame cannot be derived from that 16-bit value alone. The retained fractional low word can shift when the high word crosses the threshold under the next action's vertical integration.

For moving held actions, ROM-EXACT physical timing therefore requires preserving at least:

```text
32-bit Z accumulator (+$0E:+$10)
16-bit vertical velocity +$5C
16-bit gravity +$5E
probe42
actor32/floor variant
```

The current Saturn `uint16_t raw_z_shadow` model is structurally insufficient for exact subpixel contact timing if used as the sole Z state. This is an implementation-model blocker, not an unknown ROM rule.

## Safety
No runtime field changed in this checkpoint. No physical release enabled. No held fallback removed. No `tick==12` migration. No ROM bytes persisted.

## ROM anchors
`$2208`, `$2252..$22BE`, especially `$2280..$22A6`; floor read `$280FE/$28106`; correction apply `$F53E/$F542`; state0 `$114B2..$115F1`.

Main program SHA1: `a726c46f350723ea73158bc78055d17d418b14e2`.
