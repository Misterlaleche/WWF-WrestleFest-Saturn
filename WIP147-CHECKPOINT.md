# WIP147 CHECKPOINT — typed held callback operation API

## Scope
Encode the ROM-EXACT held callback operation classes established by WIP143–WIP146 into a reusable native API. No live physical release enabled, no held fallback removed, no `tick==12` migration, no BIN/CUE.

## Runtime/API change
Added:

```c
typedef enum {
 ARCADE_PRONE_HOLD_KEEP=0,
 ARCADE_PRONE_HOLD_CLEAR_AND_ADVANCE,
 ARCADE_PRONE_HOLD_SET_FINITE,
 ARCADE_PRONE_HOLD_STATE_TRANSITION
} arcade_prone_hold_operation_t;
```

plus helper-carry inputs and `arcade_prone_hold_operation_result()`.

The API deliberately returns `false` for a helper-dependent branch whose operation is not structurally classified. An unresolved branch is therefore **not** silently treated as KEEP or release.

## Encoded exact matrix
- raw `$0B` phase0: contact -> CLEAR_AND_ADVANCE.
- raw `$0E` phase0: contact & C11152 & !C11412 -> CLEAR_AND_ADVANCE.
- raw `$0E` phase1: contact -> CLEAR_AND_ADVANCE.
- raw `$0E` phase2: contact -> STATE_TRANSITION; never descriptor release.
- raw `$10`: contact & (C11152 | (!C11152 & C11192)) -> CLEAR_AND_ADVANCE.
- raw `$13`: contact & C11152 -> SET_FINITE `$0018`; helper-clear branch deliberately unresolved by this API.
- raw `$14`: contact -> CLEAR_AND_ADVANCE.
- raw `$23`: contact -> CLEAR_AND_ADVANCE.
- raw `$35`: contact & C11152 & !C11412 -> CLEAR_AND_ADVANCE.
- raw `$46/$47`: contact & ((C11152 & !C11412) | (!C11152 & C11192)) -> CLEAR_AND_ADVANCE.
- no contact on an in-domain held phase -> known KEEP.

## Regression
Added Test407 `tools/test407_prone_typed_hold_operations.c`.
Test407 PASS; `tools/check_host.py` compiles.
Compiler warnings observed are pre-existing misleading-indentation warnings in `arcade_action_map.c`.

## Source materialization
`WWF-Saturn-Test255-WIP147-source.zip`
SHA256: `554cf9cf77f3fcd1d494a2fbdc38ac5a9419e15e6a81a2451b89e961a866bab6`

No ROM files are included in the archive.

## Safety
The old boolean runtime path is intentionally still active and every `arcade_prone_hold_live_release_ready()` guard remains false. WIP147 only provides the richer operation representation needed for a future selective live integration.

Main program SHA1: `a726c46f350723ea73158bc78055d17d418b14e2`.
