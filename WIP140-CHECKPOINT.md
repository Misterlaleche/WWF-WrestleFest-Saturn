# WIP140 CHECKPOINT — raw14 control-phase-3 predecessor matrix

## Scope
ROM-EXACT closure of the ways the controller scheduler can place old phase `+$23==3`, which is the exact prerequisite for raw `$14` callback gate `+$25==3`. Research/checkpoint only; no live release/timing change and no BIN/CUE.

## Closed
At `$5B66`, `+$25` receives the actor's **old** `+$23` before the scheduler computes the new `+$23`. Therefore raw `$14` seeing `+$25==3` means exactly:

```text
pre-controller +$23 == 3
```

It does not mean descriptor frame 3 or elapsed tick 3.

### Main transition tables
The three scheduler tables are 11-column rows:

- default `$62BC`, 8 rows;
- mode `$6314`, 10 rows;
- alternate `$6382`, 10 rows.

Exact predecessors that produce new phase `3` are:

```text
column 0: phase3 -> 3
column 1: phase2 -> 3
column 2: phase4 -> 3
column 3: phase3 -> 3
column 4: phase3 -> 3; additionally phase8 -> 3 in $6314/$6382
column 5: phase3 -> 3
column 6: phase3 -> 3
column 7: phase3 -> 3
column 8: phase0 -> 3 in $6314/$6382 only
column 9: phase3 -> 3
column10: phase3 -> 3
```

In the default `$62BC` table, column8 instead maps phase0 -> 5.

Thus the principal non-self predecessors are:

```text
phase2 via column1
phase4 via column2
phase8 via column4 (mode tables only)
phase0 via column8 (mode tables only)
```

All other listed routes are phase3 self-preservation.

### Secondary controller assignment
The secondary path `$600E..$6096` selects one of three six-byte candidate tables:

```text
$62AA = 07 02 06 01 05 00
$62B0 = 09 04 08 03 07 02
$62B6 = 08 04 08 03 07 02
```

The normal `$62AA` table cannot assign phase3. Both mode tables `$62B0/$62B6` contain phase3 at candidate index3, so the secondary occupancy scan can directly assign `+$23=3` when that candidate is the first available entry selected by the scan.

## Ground consequence
Raw `$14` physical contact requires the intersection:

```text
old controller phase +$23 == 3
-> current +$25 == 3
AND
current-turn floor/contact classifier sets +$37.bit4
```

The predecessor matrix is now statically closed, but the exact runtime instance still depends on dynamic controller column `+$A8`, global table mode, occupancy arbitration, and contact37. Therefore no fixed-frame replacement is justified.

## ROM anchors
`$5B3E..$5BDE`, tables `$62BC/$6314/$6382`, secondary path `$600E..$6096`, secondary tables `$62AA/$62B0/$62B6`, raw14 gate `$14C0A..$14C2A`.

## Safety
No runtime field changed. No physical release enabled. No held fallback removed. No `tick==12` migration. No ROM bytes persisted.

Main program SHA1: `a726c46f350723ea73158bc78055d17d418b14e2`.
