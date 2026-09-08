# WIP149 CHECKPOINT — selective live held-integration readiness contract

## Scope
ROM-EXACT integration contract derived from WIP130–WIP148 before the first live physical held release. Research/checkpoint only: no runtime release enabled, no fallback removed, no `tick==12` change, no BIN/CUE.

## Runtime state that must exist before live release
A faithful held Ground pass requires, in ROM order:

1. **32-bit Z accumulator** corresponding to actor `+$0E:+$10` (16.16 fixed point).
2. **probe42** and **actor32 floor variant** for the current action phase.
3. Current motion state including `+$01`, vertical velocity `+$5C`, gravity `+$5E`.
4. Current descriptor counter/phase equivalent to `+$22/+24/+25`, including `$FF00` holds and typed callback operations from WIP147/WIP148.
5. Current-turn floor/contact result computed before callback, not latched from a previous turn.
6. Helper predicates required by the callback branch:
   - `$11152`: linked target state4 subtype `$08/$09` plus exact profile10 overlap;
   - `$11192`: linked target state5, `+$61==$61`, plus profile10 overlap;
   - `$11412`: reciprocal-link test.
7. Target ROM-domain state/subtype shadow where an exact branch writes or tests state4/state5 data.

The actor-pass order must remain:

```text
floor/contact classify current Z
-> apply high-word floor correction
-> resolve raw callback + typed hold operation + state/damage side effects
-> if same descriptor remains active, run one common descriptor-clock tick
-> run 16.16 movement integration
-> velocity -= gravity
```

## Per-action readiness

### Structurally closest to live integration under proven neutral/chained provenance
- raw `$0B`: CLEAR_AND_ADVANCE is contact-only, but starting 32-bit Z provenance is still needed outside chained Ground.
- raw `$14`: CLEAR_AND_ADVANCE is contact-only, but additionally requires controller old-phase `+$25==3`; cannot be reduced to elapsed frames.
- raw `$23`: preparatory hold clear is contact-only; later active branch uses secondary probe and helper predicates; phased machine from WIP123 must be preserved.

### Helper-gated operations
- raw `$0E` phase0: contact + C11152 + !C11412 -> CLEAR_AND_ADVANCE.
- raw `$0E` phase1: contact -> CLEAR_AND_ADVANCE.
- raw `$0E` phase2: STATE_TRANSITION, never ordinary descriptor release.
- raw `$10`: contact + (C11152 or C11192 on the carry-clear branch) -> CLEAR_AND_ADVANCE.
- raw `$13`: contact + C11152 -> SET_FINITE `$0018`, not immediate phase advance.
- raw `$35`: contact + C11152 + !C11412 -> CLEAR_AND_ADVANCE.
- raw `$46/$47`: contact + ((C11152 & !C11412) | (!C11152 & C11192)) -> CLEAR_AND_ADVANCE.

## Provenance classes already safe at the input level
- main-state0 Ground selection: probe42=0 and actor32 bit2 clear, but `+$01=0` preserves predecessor Z.
- controller-generated state1 Ground selection: probe42=0, actor32 bit2 clear, `+$01=1`; primary-wrestler bit5 SET provenance is closed away, so current-turn floor classification executes.
- chained Ground after a demonstrated held contact: integer/high Z begins at exact floor threshold T; fractional low word remains inherited and must be retained.

## Implementation blocker
The current historical `uint16_t raw_z_shadow` model is insufficient for live ROM-EXACT held timing. The next runtime materialization must introduce a 32-bit Z shadow (or equivalent high+fraction pair) before any physical release guard is changed.

Likewise, the old boolean `release_hold` path must not be promoted. WIP147/WIP148 typed operations are the required interface.

## Safety
- No physical release enabled.
- No 32-tick fallback removed.
- No `tick==12` damage bridge changed.
- No ROM bytes persisted.

Main program SHA1: `a726c46f350723ea73158bc78055d17d418b14e2`.
