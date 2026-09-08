# WIP132 CHECKPOINT — chained held-Ground starting-Z closure

## Scope
ROM-EXACT closure of starting `+$0E` for a Ground Action selected after normal state7 recovery from a prior held Ground Action that achieved a real floor contact. Research/checkpoint only; no live release/timing change and no BIN/CUE.

## Closed contact-to-recovery Z chain
WIP130 proves that a floor contact turn (`+$37.bit4=1`) is the below-threshold correction branch and that common `$F542` applies `+$3C=T-Z`, leaving attacker `+$0E=T` before the raw callback.

Executable audit finds no direct attacker `+$0E(A0)` write between the held contact gate and the direct state7 terminal for:
- raw `$0B`: `$13AA8..$13B94`;
- raw `$10`: `$142D8..$143B4`;
- raw `$13`: `$14A72..$14ADC`;
- raw `$14`: `$14C1A..$14CD8`;
- raw `$23` active non-bit7 branch: `$15EAE..$15FE8`;
- raw `$35`: `$1780A..$178C4`;
- raw `$46/$47`: `$144F0..$145D8`.

Therefore after a demonstrated contact, each listed raw carries exact `Z=T` into its direct state7 terminal.

WIP128 proves the standard state7 callback `$11E42` does not rewrite Z and its normal `$11E90` recovery writes state0 while preserving inherited `+$0E`.

WIP129 proves state0 itself keeps `+$01=0` and does not normalize Z before Ground selection; in this chained provenance that is now harmless because inherited Z is already exactly T.

## Consequence
For a later Ground Action selected from normal state0 recovery after one of the listed successful held contacts:

```text
starting Z = exact stage floor threshold T
probe42 = 0 after state0 init
actor32.bit2 = clear after state0 init
```

Thus the inherited-Z blocker is closed for this chained-Ground provenance class, including later selections of raw `$0B/$14/$23/$35/$46/$47`.

This does not close first Ground entry from arbitrary non-Ground predecessors, nor raw `$0E` branch-dependent recovery paths.

## ROM anchors
Floor correction/contact: `$F518..$F546`, `$280DC`, `$281DE/$28292/$2836C/$28576`.
Held contact/terminal windows listed above.
State7 preservation: `$11E42`, `$11E90`.
State0 neutralization: `$114BC/$114CC/$11500`.

## Safety
No physical release enabled. No 32-tick held fallback removed. No `tick==12` migration. No ROM bytes persisted.

Main program SHA1: `a726c46f350723ea73158bc78055d17d418b14e2`.
