# WIP135 CHECKPOINT — exact held Z integration order and fixed-point scale

## Scope
ROM-EXACT closure of per-actor ordering and numerical scale needed to reproduce held Ground flight/contact timing after WIP133/WIP134. Research/checkpoint only; no live release/timing change and no BIN/CUE.

## Exact actor-pass ordering
Common actor update `$F518..$F55E` executes in this order:

```text
1. CLR.W +$36
2. if +$01 != 0: JSR $280DC       ; geometry/floor classifier
3. apply +$38/+$3A/+$3C correction; high-word position correction
4. JSR $1C03E                     ; current state/raw callback
5. JSR $247C / $27B8              ; common post-callback helpers
6. JSR $2208                      ; movement integrator
```

Therefore a held callback can initialize `+$01=2`, `+$5C`, `+$5E`; the first vertical motion step then occurs **after that callback in the same actor pass**. On each later actor pass, floor/contact classification observes the 32-bit Z state produced by the preceding movement step before executing the held callback.

## Exact Z fixed-point scale
Mode-2 movement `$2252..$22BE` performs:

```text
MOVE.W +$5C,D0
ANDI.L #$0000FFFF,D0
SWAP D0
ASR.L #8,D0
ADD.L D0,+$0E
SUB.W +$5E,+$5C
```

Thus signed 16-bit vertical velocity contributes `velocity << 8` to the 16.16 Z accumulator `+$0E:+$10` each movement pass.

Examples:
- `+$5C=$0400` = +4.000000 pixels/pass;
- `+$5C=$0500` = +5.000000 pixels/pass;
- `+$5C=$0600` = +6.000000 pixels/pass;
- gravity `$0058` = 88/256 = 0.34375 pixels/pass²;
- gravity `$0048` = 72/256 = 0.28125 pixels/pass².

The action profiles already closed in WIP131 therefore have exact discrete parabolic trajectories, including fractional carry in `+$10`.

## Contact ordering consequence
WIP130 floor contact evaluates only the high/integer word before the callback:

```text
candidate = high16(Z) + probe42
candidate < T -> contact37=1 and high16(Z) corrected to T
candidate >= T -> contact37 remains 0
```

The low/fractional word is preserved by the word-width correction. Movement then runs after the callback and may immediately move the corrected 32-bit accumulator again if the callback leaves movement mode active.

## Saturn implementation requirement
A faithful held runtime must preserve this exact order:

```text
floor/contact test on current 32-bit shadow high word
-> high-word correction
-> raw callback/contact effects
-> 16.16 vertical integration
-> velocity -= gravity
```

Moving the integrator before contact classification, clearing the fractional word on floor correction, or using integer-only Z can shift contact/release by one or more frames.

## Safety
No runtime field changed. No physical release enabled. No held fallback removed. No `tick==12` migration. No ROM bytes persisted.

## ROM anchors
`$F518..$F55E`, `$2208`, `$2252..$22BE`, especially `$2280..$22A6`, floor resolver `$280DC` and correction `$F53E/$F542`.

Main program SHA1: `a726c46f350723ea73158bc78055d17d418b14e2`.
