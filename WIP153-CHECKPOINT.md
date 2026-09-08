# WIP153 CHECKPOINT — Ground 32-bit Z materialization

## Scope
Materialize the WIP149 prerequisite that actor Z must preserve the full ROM `+$0E:+$10` 16.16 accumulator before any callback-held physical release can become live.

## ROM-EXACT model
`fighter_t` now carries `raw_z_fp_shadow` as the full 32-bit Z alongside the legacy `raw_z_shadow` high-word mirror.

Pure helpers model exact word semantics:
- construct/extract high `+$0E` and fractional `+$10` words;
- replace/add only the high word while preserving fraction;
- raw `$0E/$10/$13` target-Z setup copies target **word `+$0E` only** and applies its proven high-word delta while retaining the attacker's own `+$10` fraction.

This follows the audited `MOVE.W` behavior; target fractional Z is not copied.

## Runtime integration
- Match construction initializes high `$0140` with fraction zero as `0x01400000` wherever the existing high-word shadow was already initialized to `$0140`.
- `ground_attack_begin_raw()` uses the 32-bit rebase helper for raw `$0E/$10/$13` and synchronizes the compatibility high-word mirror.
- Shared floor correction updates only the high word through `raw_z_set_high_preserve_fraction()`; low `+$10` remains unchanged.

No new Z provenance is invented for raw `$0B/$14/$23/$35/$46/$47`; their inherited-Z blocker remains.

## Safety
- No held release enabled.
- No 32-tick fallback removed.
- No tick12 damage migration.
- No new contact37 semantics.
- No actor32 policy regression.
- No BIN/CUE and no ROM bytes persisted.

## Tests
- Test415 pins 16.16 word semantics and fraction preservation for raw0E/raw10/raw13 and high-word correction/addition.
- Test416 pins runtime 32-bit Z storage and use of the fraction-preserving rebase/correction paths.
- Regressions Test312, Test317, Test349, Test393 and Test396 pass.

## Next
The remaining WIP149 physical-release prerequisites are current motion/phase materialization (`+$01`, vertical velocity `+$5C`, gravity `+$5E`, descriptor phase `+$22/+24/+25`) and exact per-turn helper/contact inputs. Physical release stays disabled until the relevant action has all of them.

Source ZIP: `WWF-Saturn-Test255-WIP153-source.zip`
SHA256: `b640c5a3380e1736035091b4ccb6470a29c9c250314d7388f37327048c4c71bf`
