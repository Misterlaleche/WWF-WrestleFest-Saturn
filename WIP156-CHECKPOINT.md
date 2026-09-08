# WIP156 CHECKPOINT — exact Ground vertical motion step (`+$01/+$5C/+$5E`)

## Scope
Materialize the ROM arithmetic needed for the remaining WIP149 Ground physical-release prerequisites without wiring it into live gameplay.

## ROM-EXACT closure
The active actor pass calls `$2208`, which dispatches on byte `+$01`:
- `+$01==0`: no movement integration;
- `+$01==1`: horizontal/depth path only; does not touch Z or `+$5C`;
- `+$01==2`: `$2252` motion path including Z/vertical velocity.

For `+$01==2`, `$2280..$22A6` performs:

```text
signed_velocity = sign_extend_word(+$5C)
Z16_16 += signed_velocity << 8
+$5C = WORD(+$5C - +$5E)
```

Thus `+$5C` is consumed as signed 8.8 velocity and actor `+$0E:+$10` remains a 16.16 accumulator. The low Z fraction is naturally affected by velocity and must not be truncated.

The later `$22AA..$22BC` signed/unsigned guard does not rewrite a negative 16-bit subtraction result: every negative word is unsigned `>= $8000`, so the BCC skips the `$2000` store. The demonstrated wrapping word subtraction is therefore sufficient for this path.

## Native API
Added pure helper:

```c
bool arcade_actor_vertical_motion_step(uint8_t actor01,
                                       uint32_t z32,
                                       int16_t velocity5c,
                                       uint16_t gravity5e,
                                       arcade_actor_vertical_step_t *out);
```

The helper:
- preserves Z/+5C for actor01 0/1;
- integrates signed +5C into full 16.16 Z for actor01 2;
- subtracts +5E from +5C with 16-bit wrapping semantics;
- rejects actor01 values outside the demonstrated dispatcher domain 0..2.

## Regression
Added Test419. PASS.
Also rechecked ROM anchors `$2208`, `$2280`, `$2294`, `$2298`, `$22A6` against main program SHA1 `a726c46f350723ea73158bc78055d17d418b14e2`.
Related Tests415/417/418 pass; Test416 passes with its full existing link dependency set.

Source ZIP: `WWF-Saturn-Test255-WIP156-source.zip`
SHA256: `0e0c76210704bdcc38c586759ad356cc2b45b9a49670f422dcf646e520646c99`

## Safety
- No `fighter_t` motion fields added yet.
- No live physical held release enabled.
- No 32-tick fallback removed.
- No `tick==12` migration.
- No contact37/helper result synthesized.
- No BIN/CUE.
- No ROM bytes persisted.

## Next
Materialize exact provenance/validity shadows for `+$01`, `+$5C`, `+$5E` only at demonstrated writes, then combine them with WIP153 Z16.16 and WIP147/WIP148 typed held operations before selectively enabling any one held action.
