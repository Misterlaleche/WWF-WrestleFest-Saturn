# WIP123 CHECKPOINT — raw23 exact phased contact/terminal lifecycle

## Scope
ROM-EXACT closure of raw `$23` callback phases around the initial held gate, secondary probe, active contact tree, producer split and terminal state. Research/checkpoint only; no live timing migration and no BIN/CUE.

## Phase A — preparatory held gate `$15DFE`
When current-turn `+$37.bit4` is set:
- calls `$1110E`;
- clears actor `+$01`;
- clears actor `+$22`;
- writes global `$1C1800=$000E`.

If `+$25==$FE`, the callback places the attacker in main state5 and advances/selects its `$44` subphase (`+$44 += 1`, or `$0005` when `+$36==1`).

This phase does not directly write linked-target damage or a state4 subtype.

## Phase B — active branch `$15E42..$16038`
If actor `+$60.bit7` is clear, the secondary `+$20` probe is armed at `$15EA8` when `+$25==2 && +$22==0` and cleared at `$15EBE` on contact.

At active contact `$15EAE`:
1. `$11412` carry set OR `$11152` carry clear -> fallback `$15F8E`:
   - attacker main state4 + word `$0005`;
   - facing bit7 toggled;
   - attacker `+$26.bit7` set;
   - linked target pending damage `+$68=$0003`;
   - effect `$32`;
   - linked target `+$9A/+$AA` cleared.

2. `$11412` carry clear AND `$11152` carry set -> successful linked-target route:
   - clears actor first-entry/contact bookkeeping;
   - clears target `+$9A/+$AA` and increments target `+$DE`;
   - then splits on actor/target `+$33` bits.

   2a. Special choreography when actor `+$33.bit0=1`, target `+$33.bit0=1`, actor `+$33.bit2=0`:
   - actor profile/control word `$4C=$801D`;
   - actor `+$60.bit7` and `+$35.bit0` set;
   - target main state5;
   - target `+$60=$0064`;
   - target pending damage `+$68=$0014` (20);
   - helper/effect choreography via `$10B9A/$215B6`.

   2b. Otherwise normal producer:
   - actor `+$22=$0014`;
   - target main state4 + word `$0007` at `$15F70`;
   - target pending damage `+$68=$0014` (20);
   - helper `$10F56`.

If no active contact and `+$25==$FE`, `$15FDC` writes attacker main state7 and sets attacker `+$26.bit7`.

## Phase C — actor `+$60.bit7` branch `$15FEC`
This branch bypasses the Phase-B damage tree.

If target `+$32.bit4` is set OR actor `+$FE.bit7` is set:
- attacker main state7;
- target main state4 + word `$0009` at `$1601A`;
- actor `+$35.bit0` cleared;
- applies the `$FFD0` impulse helper `$10BD0`.

No direct `+$68` pending-damage write occurs on this `$09` producer branch in the audited callback window; no damage value is inferred from another phase.

If neither gate is true, the branch returns without this producer.

## Consequence
Raw `$23` now has a bounded phased machine model:
- preparatory contact gate;
- active `$07`/state5/fallback tree;
- separate `$09` producer branch;
- exact no-contact `+$25==$FE` terminal state7 for the active non-bit7 branch.

The remaining live blocker is inherited `+$0E` provenance and faithful mapping/timing of current-turn contact/release inputs. The 32-tick fallback and `tick==12` bridge remain untouched.

## ROM anchors
`$15DFE`, `$15E1C`, `$15EA8`, `$15EAE`, `$15EBE`, `$15EC8`, `$15ED2`, `$15F22`, `$15F34`, `$15F40`, `$15F64`, `$15F70`, `$15F7C`, `$15F8E`, `$15FAE`, `$15FD4`, `$15FDC`, `$15FEC`, `$16006`, `$1600E`, `$1601A`, `$16026`.

## Safety
No physical release enabled. No held fallback removed. No `tick==12` timing migration. No ROM bytes persisted.

Main program SHA1: `a726c46f350723ea73158bc78055d17d418b14e2`.
