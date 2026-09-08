# WIP98 CHECKPOINT — Irish Whip through raw-$29 / $0C-FF grapple ownership

## Changes
- Migrated `tools/test_irish_whip.c` away from obsolete instant-HOLD fixtures.
- Every in-ring whip case now wins the exact raw `$29` struggle and proves `$0C/$FF` ownership first.
- Category `$09` physical K must emit raw `$17` before the current Irish-whip semantic bridge.
- Category `$09` P+K is asserted as raw `$15`, no longer the obsolete native cancel.
- Existing forced run, rope rebound/return, direction selection, interruption and ringside guard behavior remain covered.
- Updated `check_host.py` Irish-whip dependencies for `arcade_action_map.c` and `arcade_pair_state.c`.

## Fidelity boundary
- ROM-EXACT: raw `$29` struggle, `$0C/$FF` ownership, category `$09` K -> raw `$17`, P/P+K -> raw `$15`.
- PROVISIONAL: semantic bridge raw `$17` -> current native Irish Whip behavior; raw `$15` -> current grapple rota.
- No callback semantics are invented for `$14E40` / `$14CF4`.

## Validation
PASS: `irish_whip`, `submissions`, Tests377-380.

Source ZIP SHA256: `1cd0f15604ccde6e0b58ac948930b84c29edba47d42a5ac79182d01a0cfb9d2f`

No BIN/CUE generated.
