# Test255 WIP30 — prone raw-Z input/dispatch closure

Status: source/reverse-engineering checkpoint only; no BIN/CUE generated.

## ROM-EXACT closed

- Test298 pins the shared collision-producer dispatcher at `$28124` and the 14-entry pointer table at `$28154`.
- Producer selection is `($1C007E & 7)` plus actor `+$32 bit2`, exactly as the 68000 code computes it.
- The table is byte-exact: `$2818E,$28288,$2831C,$2818C,$28480,$2851E,$2818C,$2818C,$2818C,$2818C,$2818E,$28288,$28480,$2851E`.
- Test299 pins the exact raw-Z candidate passed to that producer: ROM `$280F6..$28106` loads actor `+$0E` into D2 and adds actor `+$42`; the resulting 16-bit value is the input to the floor/contact gate.
- Together with WIP28/WIP29, the `$FF00` chain is now: descriptor hold -> callback phase -> `+$37 bit4` -> shared producer -> candidate raw Z `+$0E + +$42` -> producer-specific floor threshold.

## Still open before live bridge

- Prove the indirect/copied producer of global `$1C007E` during live prone actions. Direct absolute producers remain only 3/4 and are not sufficient evidence for the prone paths.
- Recover the exact `+$0E`/`+$42` progression for each held phase, including raw `$0E`'s three consecutive holds.
- Only then wire the live Saturn fighter raw-Z bridge and remove the legacy 32-tick fallback.

## Validation

PASS: Test294..Test299 directed ROM/host chain.
