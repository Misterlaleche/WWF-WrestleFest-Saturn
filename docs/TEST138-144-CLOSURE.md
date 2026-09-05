# Tests138-144 / V0.153-V0.159 — canonical closure

Evidence labels are strict: **ROM-EXACT**, **PROVISIONAL**, or **REGISTERED BUT NOT IMPLEMENTED**.

## Canonical lineage decision

The user explicitly selected Test137/V0.152 plus `WWF-Saturn-Test143-WIP-from-Test137.zip` as the continuation to reconcile. That lineage is now closed and promoted through Test144/V0.159.

The separate Test140/V0.155 snapshot and the later Test143 merged-from-Test140 snapshot are retained only as non-canonical research archives. Their `$23856`, `$1EE38` and `< $0018` findings are not discarded, but they must be revalidated and assigned new future Test numbers before integration so they do not overwrite the canonical Test138-140 numbering below.

## Tests138-140 — ROM-EXACT raw stamina route

- **Test138:** `$1E48C-$1E4D8` classifies actor stamina `+$66` into raw bucket `+$BD`: `>=128 -> 0`, `101..127 -> 1`, `80..100 -> 2`, `64..79 -> 3`, `37..63 -> 4`, `1..36 -> 5`, `0 -> 8`.
- **Test139:** `$1E45C-$1E464` compares raw actor `+$109` for equality with `+$BD`. The predicate is exact; no fatigue/aggression/cadence label is assigned to `+$109`.
- **Test140:** bounded ROM audit pins the explicit `+$109` read at `$1E45C`, byte increment at `$2006C`, and direct compare sites around `$21282/$2128A`. This proves producer/consumer flow without inventing the field's semantic name.

## Test141 — ROM-EXACT rise presentation

`ST_RISE` uses recovered role-specific `$181FC/$181EC` descriptors for the first 26 ticks (13+13), then `$11E36` for the final 17 ticks.

## Tests142-143 — post-match visibility

- **Test142:** TAG/Royal winner/loser ownership comes from resolved match state. The initial Saturn post-match coordinates and movement rates are **PROVISIONAL** pending recovery of the arcade coordinate/timing routine.
- **Test143:** the old opaque centre result plate is removed so the ring and staged wrestlers remain visible. Exact arcade typography/layout is **REGISTERED BUT NOT IMPLEMENTED**.
- Exact victory clap, dedicated winner pose and defeated/sad pose remain **REGISTERED BUT NOT IMPLEMENTED**.

## Test144 — ROM-EXACT stamina/raw-109 gate consequence

For `$1E45C-$1E48A`, when `+$109 != +$BD` the helper leaves tested outputs unchanged. On equality it clears the low byte of raw state `+$20`, then writes raw `+$60=$004B` for `+$61==$4A/$64`, otherwise `+$60=$0056`.

This proves a stamina-derived raw-state consequence. It does **not** yet prove that `+$109`, `+$60` or `+$61` means fatigue, tired walk, aggression, cadence or any other gameplay concept.

## Validation

- Full host regression: **139/139 PASS**.
- Targeted ASan/UBSan for Tests138-144: **PASS**.
- SH-2/Yaul compile+link with the pinned project toolchain: **PASS**.
- Yaul ISO structural verification: **PASS** (Saturn header, USA area, V0.159/title, ISO9660, 35 CD files and exact `A.BIN`).
- Final MODE1/2352 BIN/CUE conversion: **PASS**; all sectors regenerated/compared and payload round-trip exact.
- Independent libcdio CUE read + bitwise EDC verification: **PASS**.
- Real Saturn Test144 hardware validation: **PENDING**.

## Signature / Super Throws

The source already contains overlay/state-machine routes for the ten trademark moves in `finisher_data.h`/`finisher_assets.h`; this is implemented source functionality, not proof that every route is fully arcade-exact. Per-record evidence boundaries for entry conditions, callbacks, damage and variants remain authoritative.

The completeness target **All Super Throws / Signature Finisher Moves** is **REGISTERED BUT NOT FULLY IMPLEMENTED/PROVEN**. Future work must establish wrestler -> trigger/condition -> raw action/state -> descriptor/composition -> effect from ROM evidence before upgrading any remaining route.
