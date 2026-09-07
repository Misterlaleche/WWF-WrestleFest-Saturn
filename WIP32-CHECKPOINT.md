# WWF WrestleFest Saturn — Test255 WIP32 checkpoint

## Scope
ROM-exact closure of the global collision-mode producer and actor +$32 bit2 audit needed by prone `$FF00` floor-contact callbacks.

## ROM-exact findings added after WIP31
- Normal combat writes low byte `$1C007F` from `$D12[$1C0162]`: `0,5,1,5,0,5,0,1,5,0`.
- Ring-transition setup writes `$1C007F` from `$FA00/$FA0A`, selected by `$1C1678 bit6`.
- `$FA00`: `2,6,0,6,2,6,2,2,6,2`.
- `$FA0A`: `0,5,0,5,0,5,0,0,5,0`.
- Correct collision-producer starts are `$2818E`, `$28288`, `$2831C`, `$28480`, `$2851E`.
- `$28480` produces `+$37 bit1` on the Z gate, not bit4.
- Test305 pins the complete immediate `+$32 bit2` mutation inventory: 7 BSET sites, 2 BCLR sites; state-base init clears at `$114CC`; no held-prone callback mutates bit2 directly.
- Test306 proves that with `+$32 bit2` clear, all normal-combat stage modes 0/1/5 converge to the same bit4 floor threshold `$140`.

## Tests
- Test302: transition collision-mode low-byte producer.
- Test303: composed transition context -> producer -> candidate Z -> contact bit4.
- Test304: normal combat `$D12` producer.
- Test305: actor `+$32 bit2` mutation inventory.
- Test306: base-combat bit2-clear `$140` invariant.

## Still open
Do not yet claim `ground timing provisional = 0`. The remaining proof is whether any of the seven exact BSET producers can leave actor `+$32 bit2` asserted when a prone `$FF00` action begins. Until that lifecycle is closed, the Saturn bridge must not hard-code bit2=0.

No ROM image is included in this checkpoint. No BIN/CUE is generated.
