# WIP126 CHECKPOINT — bit5 controller-chain exit to normalized state0

## Scope
ROM-EXACT closure of the two controller `+$32.bit5` SET families `$FABA/$FAE4` through their state5/raw6A→raw6B chain and eventual state0 handoff. No live release/timing change and no BIN/CUE.

## raw6A / raw6B dispatch
State5 dispatcher `$1C03E` indexes `$12614` by low `+$60`.
- raw `$6A` -> descriptor `$19B3C`, callback `$19B50`;
- raw `$6B` -> descriptor `$19BBA`, callback `$19BC0` family.

## Closed chain
### raw6A
`$19B50` first-entry:
- clears actor `+$01`;
- explicitly fixes `Z=$0100`.
At `+$25==$FE`:
- remains main state5;
- changes raw/control `+$60` from `$6A` to `$6B`.

### raw6B
On first entry `$19BC8`:
- writes actor `+$01=1`.
The callback later tests its exit helper. On the demonstrated exit at `$19C1A`:
- writes actor main state0;
- immediately executes `$19C20 BCLR #5,+$32(A0)`.

Thus both controller bit5 SET origins `$FABA/$FAE4` converge through raw6B to state0 with:
- bit5 CLEAR;
- `+$01` nonzero (`1`) at the state0 transition unless another explicitly demonstrated writer intervenes;
- no unknown Z injection inside the chain (raw6A explicitly uses Z `$0100`, while `$FAE4` also entered with Z `$0100`).

## Ground consequence
By WIP124, Ground is not selectable until after the first state0 actor pass. For this controller provenance class that pass cannot be skipped by either of the two previously-open conditions:
- bit5 has already been cleared at `$19C20`;
- `+$01` is set by raw6B init.

Therefore the first selectable state0 Ground frame after the `$FABA/$FAE4` bit5 families receives a real `$280DC` geometry/floor pass. Its Z then follows the exact WIP124 threshold/no-correction rule rather than an unclassified bit5/`+$01` bypass.

## Third bit5 family
`$1AE70` remains a separate state5/raw `$8E` special chain and is not folded into this controller proof.

## ROM anchors
`$12614`, `$127BC/$127C0`, `$19B50`, `$19B58`, `$19B62`, `$19BA6`, `$19BAC`, `$19BC8`, `$19C1A`, `$19C20`.

## Safety
No physical release enabled. No held fallback removed. No `tick==12` migration. No ROM bytes persisted.

Main program SHA1: `a726c46f350723ea73158bc78055d17d418b14e2`.
