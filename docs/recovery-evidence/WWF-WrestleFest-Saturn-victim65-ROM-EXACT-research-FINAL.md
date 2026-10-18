# WWF WrestleFest Saturn — `victim +$65` ROM-EXACT research — FINAL

## Status

**Scope:** initial producers, collision producers, state-4 consumers/retypes, high-byte flag interaction, timing, reset/clear policy, and collision FIRST→SECOND precedence for actor field `+$64/+$65`.

**Evidence basis:** WWF WrestleFest main 68000 program reconstructed from `31e14-0.ic18` (even bytes) + `31e13-0.ic19` (odd bytes), WIP35 source/documentation, and exact opcode/control-flow audits.

**Main program SHA1:** `a726c46f350723ea73158bc78055d17d418b14e2`

This document **supersedes every contradictory statement in incremental research versions v1–v9**. In particular it supersedes the early one-table collision-dispatch interpretation and the earlier misread predicates in `$24408/$246EA`.

No Saturn runtime code was changed, no GitHub commit was made, and no BIN/CUE was generated as part of this research.

---

# 1. What `+$65` is — and is not

The actor structure uses a word at `+$64`; on the big-endian MC68000:

```text
+$64 = high byte / flags
+$65 = low byte
```

For actors whose current main state is 4, the low byte `+$65` is the **state-4 subtype selector**.

The state-4 dispatcher proves this directly:

```text
$1C05E  LEA     $1AFD4,A1
$1C064  MOVE.W  $64(A0),D0
...
         ANDI.W #$00FF,D0
         LSL.W  #2,D0
         MOVEA.L (A1,D0.W),A1
```

Only the low byte is used for the subtype index.

The field is **state-dependent**, not globally a “ground-action ID”. A concrete counterexample is `$14940 MOVE.W #$0077,$64(A2)` on a path that places A2 in main state 5. Therefore the phrase “state-4 subtype” is valid only when the actor state proves state 4.

Before raw-action callbacks, the dispatcher executes:

```text
MOVEA.L $26(A0),A2
```

so in those callbacks `A2` is the linked actor. Collision code additionally uses receiver `A1`, and some SECOND handlers write a linked `A2` of their own.

---

# 2. State-4 subtype dispatch table

The exact descriptor/callback table for low-byte subtypes `$00..$2A` is:

```text
00  desc $1B080  cb $1B090
01  desc $1B0BC  cb $1B0CC
02  desc $1B114  cb $1B134
03  desc $1B114  cb $1B134
04  desc $1B114  cb $1B134
05  desc $1B1B8  cb $1B1D0
06  desc $1B268  cb $1B288
07  desc $1B278  cb $1B288
08  desc $1B300  cb $1B318
09  desc $1B30C  cb $1B318
0A  desc $1B3D2  cb $1B3DE
0B  desc $1B43A  cb $1B440
0C  desc $1B43A  cb $1B440
0D  desc $1B43A  cb $1B440
0E  desc $1B460  cb $1B478
0F  desc $1B66C  cb $1B684
10  desc $1B700  cb $1B706
11  desc $1B700  cb $1B706
12  desc $1B700  cb $1B706
13  desc $1B72E  cb $1B73E
14  desc $1B78A  cb $1B7A2
15  desc $1B84E  cb $1B85E
16  desc $1B8BA  cb $1B8CA
17  desc $1BA82  cb $1BA92
18  desc $1BAF0  cb $1BB08
19  desc $1BB40  cb $1BB4C
1A  desc $1BBB6  cb $1BBC6
1B  desc $1BC22  cb $1BC36
1C  desc $1BC9E  cb $1BCAA
1D  desc $1B78A  cb $1B7A2
1E  desc $1BCF6  cb $1BD0E
1F  desc $1BD60  cb $12612
20  desc $1B3D2  cb $1B3DE
21  desc $1BD66  cb $1BD6C
22  desc $1BDD8  cb $1BDE8
23  desc $1BE6C  cb $1BE7C
24  desc $1BECC  cb $1BED8
25  desc $1BF08  cb $1BF1C
26  desc $1BF96  cb $1BFA6
27  desc $1BFFE  cb $1C004
28  desc $1BD60  cb $12612
29  desc $1BAFC  cb $1BB08
2A  desc $1B1B8  cb $1B1D0
```

`$12612` is an RTS; subtypes `$1F/$28` therefore have no internal state-4 callback logic at that address.

---

# 3. Complete action-side initial state-4 producer audit

A generalized scan of executable writes to `+$64/+65` combined with state writes yields **51 direct action-side state-4 producer sites**: 50 on linked actor `A2` and one on `A3`.

`$14940 MOVE.W #$0077,$64(A2)` is deliberately excluded from this list because the surrounding route places A2 in main state 5, not state 4.

```text
$12A6A  A2  MOVE.W #$0029,$64(A2)
$12B92  A2  MOVE.B #$19,$65(A2)
$12D80  A2  MOVE.W #$0019,$64(A2)
$12DE6  A2  MOVE.W #$0019,$64(A2)
$12FDA  A2  MOVE.B #$19,$65(A2)

$1381A  A2  MOVE.W #$000F,$64(A2)
$1383E  A2  MOVE.W #$010F,$64(A2)
$138B8  A2  MOVE.W #$0006,$64(A2)
$13928  A2  MOVE.W #$0006,$64(A2)
$139D4  A2  MOVE.W #$000F,$64(A2)

$14A90  A2  MOVE.W #$0007,$64(A2)
$14C52  A2  MOVE.W #$0007,$64(A2)
$14F48  A2  MOVE.W #$0022,$64(A2)
$150E2  A2  MOVE.W #$0025,$64(A2)
$15154  A2  MOVE.W #$0021,$64(A2)
$15348  A2  MOVE.W #$0006,$64(A2)
$154C2  A2  MOVE.W #$001C,$64(A2)
$15536  A2  MOVE.W #$0003,$64(A2)
$155E6  A2  MOVE.W #$0020,$64(A2)

$15F70  A2  MOVE.W #$0007,$64(A2)
$1601A  A2  MOVE.W #$0009,$64(A2)
$160AC  A2  MOVE.W #$0016,$64(A2)
$161A6  A2  MOVE.W #$0002,$64(A2)
$1630C  A2  MOVE.W #$0005,$64(A2)
$164EC  A2  MOVE.W #$0005,$64(A2)
$167B0  A2  MOVE.W #$0025,$64(A2)
$16842  A2  MOVE.W #$0024,$64(A2)
$1696C  A2  MOVE.W #$0015,$64(A2)
$16C06  A2  MOVE.W #$0006,$64(A2)
$16D5A  A2  MOVE.W #$0026,$64(A2)
$16D9C  A2  MOVE.W #$0016,$64(A2)
$16F3E  A2  MOVE.W #$0013,$64(A2)
$17100  A2  MOVE.W #$002A,$64(A2)
$1723E  A2  MOVE.W #$0002,$64(A2)
$1736A  A2  MOVE.W #$0025,$64(A2)
$173F8  A2  MOVE.W #$0005,$64(A2)
$17542  A2  MOVE.W #$0023,$64(A2)
$17858  A2  MOVE.W #$001A,$64(A2)
$17C74  A2  MOVE.W #$0015,$64(A2)

$18222  A2  MOVE.W #$000F,$64(A2)
$18290  A3  MOVE.W #$000F,$64(A3)
$186E6  A2  MOVE.W #$0005,$64(A2)
$18BA6  A2  MOVE.W #$0001,$64(A2)
$1904A  A2  MOVE.W #$0002,$64(A2)
$190E6  A2  MOVE.W #$0005,$64(A2)
$192A2  A2  MOVE.W #$0002,$64(A2)
$19360  A2  MOVE.W #$0002,$64(A2)
$19410  A2  MOVE.W #$0002,$64(A2)
$1947A  A2  MOVE.W #$0027,$64(A2)
$197BA  A2  MOVE.W #$0006,$64(A2)
$1ACCE  A2  MOVE.W #$0002,$64(A2)
```

The action-side initial low-byte domain is therefore:

```text
01 02 03 05 06 07 09 0F 13 15 16 19 1A 1C 20 21
22 23 24 25 26 27 29 2A
```

There is **no demonstrated direct action-side initial subtype `$08` producer**.

Two producers (`$12B92/$12FDA`) write the low byte only and preserve the current high byte. `$1383E` explicitly writes word `$010F`, proving that the high byte can be initialized non-zero by a word producer.

---

# 4. The five exact initial `$05` action producers

Exactly five direct linked-actor initial producers write word `$0005`:

```text
$1630C  raw $26  -> victim +$65=$05
$164EC  raw $28  -> victim +$65=$05
$173F8  raw $33  -> victim +$65=$05
$186E6  raw $44  -> victim +$65=$05
$190E6  raw $56  -> victim +$65=$05
```

Because these are word writes of `$0005`, they establish:

```text
+$64 high byte = $00
+$65 low byte  = $05
```

Known WIP-backed identities:

```text
raw $26 = FIST DROP
raw $28 = Smash Suplex
raw $44 = Demolition/Crush finisher family
```

The human-readable move identities for raw `$33/$56` remain deliberately unlabeled.

Known direct surrounding facts include:

```text
raw $26: pending damage +$68 = $0016
raw $28: pending damage +$68 = $0012
raw $33: branch includes +$25==03, +$37.bit4==1, +$60.bit7==0; damage $0012
raw $44: pending damage +$68 = $0018
raw $56: pending damage +$68 = $0003 on the audited terminal branch
```

---

# 5. Exact state-4 consumer/retype graph

This section closes what each dispatched subtype does to the low byte where a direct transition is present.

## `$00`

Callback `$1B090` has no subtype rewrite. On its terminal active path it can write main state 0. Low subtype `$00` is not separately cleared.

## `$01`

Callback `$1B0CC` has no subtype rewrite. Countdown `+$46` eventually returns to main state 0; low subtype remains stale.

## `$02/$03/$04`

Shared callback `$1B134`:

```text
current-turn +$37.bit4 == 1
-> state 4
-> MOVE.B #$05,$65(A0)
```

Exact transitions:

```text
02 -> 05
03 -> 05
04 -> 05
```

The byte write preserves the high `+$64` flags.

## `$05/$2A`

Shared callback `$1B1D0`.

On the first entry, subtype `$05` may latch high-byte bit7 when `+$71==$02`:

```text
$1B208  CMPI.B #$02,$71(A0)
$1B210  BSET   #7,$64(A0)
```

Subtype `$2A` bypasses this latch path.

The first callback invocation is initialization-only and does not consume `$05/$2A`.

On the later active contact gate:

```text
+$37.bit4 == 0 -> remain current subtype
+$37.bit4 == 1 -> transition
```

Results:

```text
2A -> 08
05 + bit7 clear -> 08
05 + bit7 set   -> 09
```

## `$06/$07`

Shared callback `$1B288`. When `+$25==$FE`:

```text
06 -> 08
07 -> 09
```

Again low-byte writes preserve high flags.

## `$08/$09`

Shared callback `$1B318`. No low subtype rewrite. On its terminal exit path it writes main state 7 and:

```text
$1B3CA  BCLR #7,$64(A0)
```

Thus the critical high-byte latch is explicitly cleared, but low `$08/$09` can remain stale outside state 4.

## `$0A/$20`

Shared callback `$1B3DE`.

When active with `+$25==$FE`:

```text
20 -> state4 + WORD $0002 -> 02
```

This word write clears the high byte.

Subtype `$0A` instead exits to state 0 or state 1 depending on `+$32.bit0`; no low rewrite is performed there.

## `$0B/$0C/$0D`

Shared callback `$1B440`:

```text
SUBI.B #$0B,$65(A0)
ADDI.B #$02,$65(A0)
```

Exact low-byte transitions:

```text
0B -> 02
0C -> 03
0D -> 04
```

High flags are preserved.

## `$0E`

Callback `$1B478` has a demonstrated active route to `$05`:

```text
+$25 == $03
+$37.bit4 == 1
and (+$33.bit2 == 0 OR global $1C0161.bit1 == 1)
-> state4 + low05
```

Alternative branches can route to state 5 instead. The low05 transition is exact; a single universal outcome for all `$0E` frames is not asserted.

## `$0F`

Callback `$1B684`:

```text
+$25 == $03 && +$37.bit4 == 1
-> 0F -> 05
```

## `$10/$11/$12`

Shared callback `$1B706`: no low subtype rewrite. Countdown `+$44` eventually exits to state 0; low value is left stale.

## `$13`

Callback `$1B73E`:

```text
+$37.bit4 == 1 -> 13 -> 05
```

## `$14/$1D`

Shared callback `$1B7A2`, when `+$25==$FE`:

```text
if +$1F == $02:
    14/1D -> 1B
else:
    14/1D -> 02
```

The `$02` path uses a word write and therefore resets the high byte.

## `$15`

Callback `$1B85E`:

```text
+$37.bit4 == 1 -> 15 -> 05
```

## `$16`

Callback `$1B8CA` has an exact active `$16->05` route when:

```text
+$25 == $01
+$37.bit4 == 1
and (+$33.bit2 == 0 OR global $1C0161.bit1 == 1)
```

Alternative branches can route to state 5.

## `$17`

Callback `$1BA92`:

```text
+$37.bit4 == 1 -> 17 -> 05
```

## `$18/$29`

Shared callback `$1BB08`, active when `+$25==$FE`:

```text
29 -> state4 + WORD $0003 -> 03
18 -> state0, no low rewrite
```

## `$19`

Callback `$1BB4C`:

```text
+$37.bit4 == 1 -> 19 -> 05
```

## `$1A`

Callback `$1BBC6`:

```text
+$37.bit4 == 1 -> 1A -> 09
```

## `$1B`

Callback `$1BC36`:

```text
+$37.bit4 == 1 -> state4 + WORD $0005 -> 05
```

Word write resets high flags.

## `$1C`

Callback `$1BCAA`, active when `+$25==$FE`:

```text
1C -> state4 + WORD $0002 -> 02
```

## `$1E`

Callback `$1BD0E`, active when `+$25==$FE`:

```text
1E -> state4 + WORD $100F
```

Therefore low subtype becomes `$0F`, while high byte becomes `$10`.

## `$1F/$28`

Callback `$12612` is RTS. There is no internal state4 subtype transition in this callback.

## `$21`

Callback `$1BD6C`, after its active contact gate:

```text
+$71 == $02 -> 21 -> 09
otherwise   -> 21 -> 08
```

## `$22`

Callback `$1BDE8`:

```text
+$25 == $01 && +$37.bit4 == 1
-> state4 + WORD $0005
-> 22 -> 05
```

## `$23`

Callback `$1BE7C`:

```text
+$37.bit4 == 1 -> 23 -> 05
```

## `$24`

Callback `$1BED8`, active with `+$25==$FE`:

```text
24 -> 05
```

## `$25`

Callback `$1BF1C`:

```text
+$25 == $02 && +$37.bit4 == 1
-> state4 + WORD $0005
-> 25 -> 05
```

## `$26`

Callback `$1BFA6`:

```text
+$37.bit4 == 1 -> 26 -> 05
```

## `$27`

Callback `$1C004` initializes a countdown (`+$22=8`) and eventually:

```text
27 -> state4 + WORD $0002 -> 02
```

---

# 6. External prone linked-target retype cycle

Exactly three direct linked-target arithmetic writer pairs were found:

```text
$13796/$1379C
$13ADE/$13AE4
$14320/$14326
```

Each performs:

```text
SUBI.B #8,$65(A2)
ADDI.B #6,$65(A2)
```

Therefore:

```text
08 -> 06
09 -> 07
```

The WIP audit associates these with raw callbacks `$0A/$0B/$10`.

Because state4 callback `$1B288` later performs:

```text
06 -> 08
07 -> 09
```

when `+$25==$FE`, the ROM contains an exact held/prone cycle:

```text
08 <-> 06
09 <-> 07
```

All four transitions use low-byte operations and preserve the high flag byte.

---

# 7. Write-width rule

This is mandatory for a faithful implementation.

### Low-byte write

Examples:

```text
MOVE.B #imm,$65(Ax)
SUBI.B/ADDI.B ...,$65(Ax)
MOVE.B table(...),$65(Ax)
```

These preserve every high-byte `+$64` flag.

### Word write

Example:

```text
MOVE.W #$0005,$64(Ax)
```

This replaces both high flags and low subtype.

Thus:

```text
MOVE.W #$0005 -> high=$00, low=$05
MOVE.W #$100F -> high=$10, low=$0F
MOVE.W #$010F -> high=$01, low=$0F
```

This distinction explains why a low-byte transition can carry a previously latched bit7 forward, whereas many word producers reset it.

---

# 8. Collision profile architecture — final corrected model

Collision selection is **two-dimensional and two-stage**.

## 8.1 Source profile row

`$240BC` selects a seven-byte collision profile row from `$24EF6` using the low byte of source actor `+$4C/+4D`:

```text
LEA   $24EF6,A6
MOVE.W $4C(A0),D0
ANDI  #$00FF,D0
MULU  #7,D0
ADDA  D0,A6
```

Therefore source profile ID is low byte `+$4D(A0)`.

The source row is retained as `A2` for the dispatch.

## 8.2 Target profile row

The collision loop swaps A0/A1, invokes the same profile-row selector for the target actor, saves that row as `A3`, then restores A0/A1.

Thus:

```text
A2 = source actor profile row
A3 = target/receiver actor profile row
```

This is crucial: **FIRST and SECOND selectors do not generally come from the same profile row.**

## 8.3 FIRST dispatcher

```text
$2435E  LEA     $24398,A4
$24364  MOVE.B  $05(A2),D0
...
$24372  JSR     FIRST
$24374  BCS     stop
```

FIRST table `$24398`, 15 entries:

```text
00 -> $2484C
01 -> $24408
02 -> $24468
03 -> $244E0
04 -> $2452A
05 -> $245F0
06 -> $24640
07 -> $2468A
08 -> $246EA
09 -> $24732
0A -> $24784
0B -> $247EC
0C -> $245BC
0D -> $245D6
0E -> $24784
```

## 8.4 SECOND dispatcher

Only when FIRST returns carry clear:

```text
$2437A  LEA     $243D4,A4
$24380  MOVE.B  $06(A3),D0
...
$2438E  JSR     SECOND
```

SECOND table `$243D4`, 13 entries:

```text
00 -> $24852
01 -> $24868
02 -> $24924
03 -> $24984
04 -> $24A38
05 -> $24A90
06 -> $24AA8
07 -> $24B20
08 -> $24BA4
09 -> $24BC2
0A -> $24C42
0B -> $24CCA
0C -> $24D5E
```

---

# 9. Collision profile selector table `$00..$21`

Each row supplies a FIRST selector at row+5 when that profile is the **source**, and a SECOND selector at row+6 when that profile is the **target**.

| Profile `+$4D` | FIRST | SECOND |
|---:|---:|---:|
| `$00` | `$00` | `$00` |
| `$01` | `$00` | `$00` |
| `$02` | `$00` | `$00` |
| `$03` | `$02` | `$00` |
| `$04` | `$03` | `$0C` |
| `$05` | `$0A` | `$0C` |
| `$06` | `$04` | `$00` |
| `$07` | `$01` | `$00` |
| `$08` | `$0D` | `$00` |
| `$09` | `$00` | `$01` |
| `$0A` | `$00` | `$02` |
| `$0B` | `$05` | `$00` |
| `$0C` | `$00` | `$03` |
| `$0D` | `$00` | `$04` |
| `$0E` | `$00` | `$05` |
| `$0F` | `$06` | `$00` |
| `$10` | `$07` | `$0C` |
| `$11` | `$08` | `$00` |
| `$12` | `$09` | `$0C` |
| `$13` | `$0C` | `$00` |
| `$14` | `$00` | `$06` |
| `$15` | `$00` | `$07` |
| `$16` | `$05` | `$00` |
| `$17` | `$0B` | `$00` |
| `$18` | `$05` | `$00` |
| `$19` | `$01` | `$00` |
| `$1A` | `$00` | `$08` |
| `$1B` | `$0E` | `$0C` |
| `$1C` | `$05` | `$00` |
| `$1D` | `$00` | `$09` |
| `$1E` | `$00` | `$0A` |
| `$1F` | `$00` | `$0B` |
| `$20` | `$00` | `$00` |
| `$21` | `$00` | `$00` |

The complete profile matrix is therefore the Cartesian composition:

```text
Psrc = low +$4D(A0)
F = FIRST_SELECTOR[Psrc]
execute FIRST(F)

if FIRST carry == 1:
    stop; SECOND does not run

if FIRST carry == 0:
    Ptgt = low +$4D(A1)
    S = SECOND_SELECTOR[Ptgt]
    execute SECOND(S)
```

This is the complete static `profile -> FIRST -> carry -> SECOND` matrix without the false assumption `Psrc==Ptgt`.

---

# 10. Exact FIRST carry behavior

The ROM uses:

```text
ANDI #$EE,CCR  -> carry clear (success)
ORI   #$01,CCR -> carry set   (failure)
```

Exact FIRST carry predicates:

### F0 `$2484C`

```text
C=0 always
```

### F1 `$24408`

```text
C=1 iff +$4C(A1).bit6 == 1
otherwise C=0
```

### F2 `$24468`

```text
C=1 iff BYTE[$4C(A1)] != 0
otherwise C=0
```

### F3 `$244E0`

Success only when:

```text
BYTE[$4C(A1)] == 0
+$33(A1).bit4 == 1
BYTE[$2E(A0)] != BYTE[$2E(A1)]
```

Otherwise carry is set.

### F4 `$2452A`

```text
C=1 iff BYTE[$4C(A1)] != 0
otherwise C=0
```

### F5 `$245F0`, FC `$245BC`, FD `$245D6`

All reach the same success/failure body after wrapper-specific side effects:

```text
C=1 iff BYTE[$4C(A1)] != 0
otherwise C=0
```

### F6 `$24640`

```text
C=0 always
```

### F7 `$2468A`

```text
C=1 iff BYTE[$4C(A1)] != 0
otherwise C=0
```

### F8 `$246EA`

```text
C=0 always
```

### F9 `$24732`

```text
C=1 iff BYTE[$4C(A1)] != 0
otherwise C=0
```

### FA/FE `$24784`

Success only when:

```text
BYTE[$4C(A1)] == 0
+$33(A1).bit4 == 1
D1 != WORD[$2E(A1)]
```

otherwise carry is set.

The provenance of `D1` is closed: it is the inner target-actor iterator cursor after selecting the target actor, i.e. selected active target slot index + 1. The surrounding geometry helpers preserve the iterator register set.

No gameplay-semantic interpretation is assigned to comparing that cursor value against `WORD[$2E(A1)]`.

### FB `$247EC`

```text
C=1 iff +$4C(A1).bit7 == 1
otherwise C=0
```

Thus the FIRST-stage carry behavior is structurally closed.

---

# 11. Exact FIRST subtype/retype behavior

Let `N(x)` denote the normalization helper `$24D7A` described in section 13.

## F0 `$2484C`

No explicit subtype write. Carry clear.

## F1 `$24408`

If bit6 of `+$4C(A1)` is set, carry failure.

On success, initial A1 word is `$0000`.

If bit7 of `+$4C(A1)` is set, it succeeds immediately, keeping `$00` and skipping normalization.

With bit7 clear:

```text
if +$33.bit4 == 1 OR +$52 >= 3:
    overwrite A1 subtype with $02
else:
    keep $00
```

Then call `N` and `$24DDC`.

This is the corrected predicate; earlier incremental reports that inverted the `+$33.bit4` branch are superseded.

## F2 `$24468`

When `+$4C(A1)==0`:

```text
A1 state4 + word02
call N(A1)
```

The handler also writes a subtype on A0 itself (`$02` or `$18` depending on A0 `+$21`), which is a separate actor-role effect.

## F3 `$244E0`

On its exact success gate:

```text
A1 state4 + word0E
```

No normalization call.

## F4 `$2452A`

On success:

```text
initial A1 word10
if ((BYTE[A0+$2D] & $80) != BYTE[A1+$2E]):
    overwrite with word0B
else:
    keep word10
```

No normalization call.

## F5/FC/FD `$245F0` family

On success:

```text
if +$33(A1).bit4 == 0:
    A1 word02
else:
    A1 word03
```

Then normalization and `$24DDC`.

## F6 `$24640`

Always carry clear:

```text
if +$33(A1).bit4 == 0:
    A1 word03
else:
    A1 word04
```

Then normalization and `$24DDC`.

## F7 `$2468A`

On success:

```text
if BYTE[A0+$61] == $41:
    A1 word1D
else:
    A1 word14
```

Then normalization and `$24DDC`.

## F8 `$246EA`

Always carry clear. Initial A1 word is `$000A`.

If `+$4C(A1).bit7` is set, it succeeds immediately, keeping `$0A` and skipping normalization.

With bit7 clear:

```text
if +$33.bit4 == 1 OR +$52 >= 3:
    overwrite A1 with word02
else:
    keep word0A
```

Then normalization and `$24DDC`.

This is likewise the corrected predicate.

## F9 `$24732`

On success:

```text
A1 word11
call N
```

## FA/FE `$24784`

On the exact carry-success gate:

```text
if source A0 profile ID +$4D == $1B:
    A1 word28
else:
    A1 word1F
```

No normalization call.

## FB `$247EC`

When `+$4C(A1).bit7==0`:

```text
A1 state4 + word12
```

If target A1 profile ID `+$4D==$1A`, the handler skips normalization and `$24DDC`; otherwise it calls both.

This is the FIRST producer used when the source profile is `$17`.

---

# 12. Doomsday profile `$17` — corrected pairwise interpretation

Profile `$17` row contains FIRST selector `$0B`. Therefore when the **source actor** has profile `$17`:

```text
Psrc=$17
-> FIRST FB
-> $247EC
-> if carry gate succeeds
-> $2482A writes A1 word0012
-> receiver low +$65=$12
```

This exact producer remains valid.

However, the target actor's SECOND selector is determined by **the target actor's own profile row**, not by source profile `$17`. Therefore a route analysis must not automatically assume SECOND `$00` merely because the source profile row `$17` has row+6 equal `$00`.

The correct pairwise rule is:

```text
source profile17 supplies FIRST=0B
receiver profile supplies SECOND independently
```

---

# 13. Common normalization helper `$24D7A`

`$24D7A` is a consumer/retype helper, not an initial producer.

## Range gate

```text
if +$65(A1) >= $24:
    unchanged
```

## Orientation gate

For values `<$24`, it compares bit7 of `+$2E` on A0 and A1:

```text
if bit7(A0+$2E) != bit7(A1+$2E):
    unchanged
```

If those bits are equal, it performs:

```text
+$65(A1) = TABLE[+$65(A1)]
```

using a low-byte write, preserving high flags.

Exact table:

```text
00 -> 0B
01 -> 0C
02 -> 0B
03 -> 0C
04 -> 0D
05 -> 05
06 -> 06
07 -> 07
08 -> 08
09 -> 09
0A -> 0A
0B -> 0B
0C -> 0C
0D -> 0D
0E -> 0E
0F -> 0F
10 -> 0B
11 -> 0B
12 -> 0B
13 -> 0B
14 -> 0B
15 -> 0B
16 -> 0B
17 -> 0B
18 -> 0B
19 -> 0B
1A -> 0B
1B -> 0B
1C -> 0B
1D -> 0B
1E -> 0B
1F -> 0B
20 -> 0B
21 -> 0B
22 -> 0C
23 -> 0D
```

Fixed points are:

```text
05 06 07 08 09 0A 0B 0C 0D 0E 0F
```

Therefore this helper **does not** cause `$05->$08/$09`.

Typical collapse classes:

```text
00/02/10..21 -> 0B
01/03/22      -> 0C
04/23         -> 0D
```

The helper is invoked from both FIRST and SECOND collision handlers.

`$24DDC`, often called next, only tests `+$33(A1).bit4` and conditionally increments pending damage `+$68(A1)`; it does not modify `+$65`.

---

# 14. Exact SECOND behavior and precedence

SECOND executes after all FIRST explicit writes and FIRST-side normalization, and only if FIRST returned carry clear. Consequently any explicit SECOND A1 write supersedes the surviving FIRST A1 subtype.

## S0 `$24852`

No explicit subtype producer. Sets relation flags, clears `+$44(A1)`, and calls normalization on the existing A1 subtype.

## S1 `$24868`

If full byte `+$2E(A1)==+$2E(A0)`, no state4 subtype is written; A1 is sent to a pending state-5 route.

If orientation bytes differ:

```text
if incoming WORD +$64 is 0000 or 000A and +$52<3:
    paired state5 choreography; no A1 state4 subtype write
else:
    A1 state4 + word02
```

No normalization call follows this explicit `$02` in S1.

## S2 `$24924`

Requires full-byte equality `+$2E(A1)==+$2E(A0)` for the producer path. Otherwise A1 goes to the pending state-5 route.

On equality:

```text
linked A2 -> state0
A1 -> state4 + word0B
```

If incoming A1 word was zero, normalization can be called before the explicit `$0B`, but the later word write supersedes that earlier result. Final low is `$0B`.

## S3 `$24984`

Requires full-byte inequality `+$2E(A1)!=$2E(A0)` for the main body; equality routes A1 to pending state5.

With differing bytes:

```text
if incoming WORD +$64 is 0000 or 000A and +$52<3:
    paired state5 choreography; no A1 state4 subtype write
else:
    A1 state4 + word02
```

No normalization call follows that explicit write.

## S4 `$24A38`

Requires identical full `+$2E` bytes.

On equality:

```text
linked A2 -> state4 + word01
A1        -> state4 + word02
call N(A1)
```

Because full-byte equality guarantees orientation-bit equality, normalization maps A1 `$02->$0B`.

Thus the stable end of this handler is:

```text
A2 low01
A1 low0B
```

## S5 `$24A90`

No subtype write. Clears/manipulates `+$74/+$76/+$44` and a linked actor state.

## S6 `$24AA8`

Main producer path only when incoming A1 full word is `$0000` or `$000A`:

```text
A1 state4 + word02
call N(A1)
linked A2 -> state7
```

A1 ends as `$02` or `$0B` according to the normalization orientation gate.

## S7 `$24B20`

Same incoming full-word gate `{0000,000A}`:

```text
A1 state4 + word02
call N(A1)
linked A2 -> state0
```

A1 ends as `$02` or `$0B`.

## S8 `$24BA4`

This is not an initial `$12` producer. It first requires:

```text
BYTE +$65(A1) == $12
```

then reasserts:

```text
A1 state4 + WORD $0012
```

Thus:

```text
12 -> 12
```

with the side effect that the word write clears the previous high flag byte.

## S9 `$24BC2`

Main producer path requires incoming A1 full word `$0000` or `$000A`:

```text
A1 -> state4 + word02 -> N(A1)
linked A2 -> state4 + word09
```

This is a direct collision-side producer of low `$09` on the linked actor.

## SA `$24C42`

With incoming A1 full word `$0000` or `$000A`:

```text
A1 -> state4 + word02 -> N(A1)
linked A2 -> state4 + word0B
```

It also copies orientation-related state and writes `+$9A(A2)=8`.

## SB `$24CCA`

With incoming A1 full word `$0000` or `$000A`:

```text
A1 -> state4 + word02 -> N(A1)
linked A2 -> state7
```

No linked-A2 subtype write.

## SC `$24D5E`

Gate:

```text
BYTE +$65(A1) == $0A
```

Then:

```text
A1 state4 + WORD $0002
call N(A1)
```

Thus SC is a consumer/retype:

```text
0A -> 02 or 0B
```

not an unconditional initial producer.

---

# 15. Exact collision-write precedence

For A1, the proven ordering is:

```text
FIRST explicit subtype write
-> optional FIRST normalization N
-> SECOND explicit subtype write/reassert/retype, if its branch does so
-> optional SECOND normalization N
-> generic successful collision tail
```

The generic successful tail then calls `$24DEC` and `$24E02`.

`$24E02` does not change the low byte. It can set high-byte bit7 when the receiver's tier is critical:

```text
if WORD +$70(A1) == $0002:
    BSET #7,$64(A1)
```

Therefore:

1. FIRST carry set stops all SECOND effects.
2. SECOND explicit A1 writes override FIRST A1 subtype and any FIRST normalization.
3. SECOND normalization operates on the subtype currently present after SECOND's write.
4. SECOND may independently create linked-A2 subtypes `$01/$09/$0B`.
5. The generic collision tail may alter high flag bit7 but does not rewrite low subtype.

This closes the write precedence requested for `+$65`.

---

# 16. `$09` provenance is multi-class

At least three structurally independent routes to low `$09` are ROM-exact:

```text
A. raw-action direct initial producer
   raw $23 route -> A2 low09

B. state-4 retype
   05 -> 09 when high bit7 latched
   21 -> 09 when +$71==02
   07 -> 09 on held/prone return
   1A -> 09 on contact37

C. SECOND collision linked-actor producer
   S9 $24BC2 -> linked A2 word0009
```

Thus low subtype alone never establishes provenance.

---

# 17. High-byte bit7 is a critical-stamina latch

The executable ROM audit finds two direct setters of bit7 in the high byte `+$64`:

```text
$1B210  BSET #7,$64(A0)
$24E4C  BSET #7,$64(A1)
```

and one explicit clear:

```text
$1B3CA  BCLR #7,$64(A0)
```

No other demonstrated direct `BSET/BCLR #7,+$64(A2/A3)` exists.

Word writes to `+$64` may of course replace the entire high byte and thereby clear or set flags independently.

## Tier classifier `$24EC2`

For non-zero maximum stamina `+$72`:

```text
if live stamina +$66 <= 24:
    +$70 = $0002
else if +$66 <= 2 * floor(+$72 / 3):
    +$70 = $0001
else:
    +$70 = $0000
```

If `+$72==0`, the routine returns without synthesizing a new tier.

Because the CPU is big-endian:

```text
+$70=$0002 -> +$71=$02
```

Thus every audited test `+$71==$02` means **critical stamina tier**.

The critical threshold is exactly:

```text
live stamina <= 24
```

## `$05` latch rule

The five direct word05 producers start with high-byte bit7 clear.

On first entry to `$1B1D0`:

```text
if subtype05 and +$71==$02:
    bit7 = 1
```

On later active contact:

```text
if +$37.bit4==1:
    if bit7==0 -> 08
    if bit7==1 -> 09
```

The generic collision tail supplies another route to set the same latch whenever receiver `+$70==$0002`.

This makes `$09` in the `$05` family a **critical-tier-latched branch**, not a fixed move-specific choice.

---

# 18. Damage ordering and temporal conditions

Central pending-damage processing `$24E58` does:

```text
live = +$66
live -= pending +$68
clamp live at zero
write live +$66
BSR $24EC2
```

Therefore the tier is recomputed from **post-damage** live stamina.

Main per-frame ordering is:

```text
1. actor callbacks / state dispatch  ($F4C2)
2. collision processing             ($24062)
3. pending damage + tier recompute  ($24E58)
```

Inside a successful collision pair:

```text
FIRST
-> SECOND
-> $24DEC
-> $24E02 critical-latch test
```

Only later does `$24E58` apply that frame's new pending damage.

Consequently `$24E4C` tests the tier stored **before** that frame's new pending damage is applied.

---

# 19. Actor-slot ordering and same-frame/next-frame prone entry

The actor iterator `$250E` processes active slots in ascending order from the pointer table at `$5122`:

```text
slot0 -> $1C05B0
slot1 -> $1C06BC
slot2 -> $1C07C8
slot3 -> $1C08D4
slot4 -> $1C09E0
slot5 -> $1C0AEC
slot6 -> $1C0BF8
slot7 -> $1C0D04
slot8 -> $1C0E10
```

When an attacker callback writes linked victim `+$20=4`:

```text
victim slot > attacker slot
-> victim has not yet had its actor turn
-> first state4 callback can occur later in SAME frame

victim slot < attacker slot
-> victim already had its turn
-> first state4 callback occurs NEXT frame
```

The first `$1B1D0` invocation is initialization-only; it can latch critical tier but cannot perform the `$05->$08/$09` consumption. The active transition happens on a later invocation after the state-initialization bit is set.

Therefore a hit that newly crosses the critical threshold can expose pre-hit or post-hit tier to the first subtype05 initialization depending on slot order. This is not nondeterminism: it is exact scheduler behavior.

A concrete `$08/$09` outcome for one hit instance therefore depends on runtime state:

```text
stamina/tier before hit
pending damage
actor slot order
whether a later eligible collision tail sets bit7
current-turn +$37.bit4 contact classification
```

There is no valid static “raw move X always produces 08/09” rule.

---

# 20. `+$37.bit4` timing

Before each actor callback pass, `$F518` executes:

```text
CLR.W $36(A0)
...
JSR $280DC
...
JSR $1C03E
```

Thus `+$36/+$37` is rebuilt each actor turn before state dispatch.

The direct setters of `+$37.bit4` reside in the collision/geometry classification code (`$281DE/$28292/$2836C/$28576`). The bit is therefore a **current-turn predicate**, not a persistent flag inherited from the producer frame.

Many subtype transitions correctly wait for this per-turn condition before retyping to `$05`, `$08/$09`, etc.

A human-readable semantic label more specific than “current contact/geometry classifier bit” is not required to reproduce the ROM rule.

---

# 21. Reset and clear policy

The field is **not generically zeroed when state 4 ends**.

## Bulk actor-memory reset

Three byte-clear loops cover the actor-memory ranges containing all actor structures:

```text
$1F22  clear $1C0060 .. $1C3AD3
$1F3C  clear $1C05AC .. $1C1CD3
$1F56  clear $1C0160 .. $1C1CD3
```

These bulk resets necessarily zero actor `+$64/+65`.

## Explicit live clears

No direct executable `CLR.B $65(Ax)` and no generic `CLR.L $64(Ax)` was found.

Four direct `CLR.W $64(A0)` live-code sites were found:

```text
$17B4C
$182EC
$18F9E
$1A1AE
```

Each belongs to an action-specific terminal route around a `+$25==$FE` gate and state7 transition. These clear both high flags and low subtype.

## No generic state-exit clear

Several state4 callbacks prove deliberate stale-low-byte behavior:

```text
subtype00 -> state0, no +$64/+65 clear
subtype01 -> state0, no clear
subtype08/09 -> state7, only high bit7 cleared; low remains 08/09
subtype10/11/12 -> state0, no low clear
subtype18 -> state0, no low clear
```

Therefore the lifecycle terminal condition is often **leaving main state 4**, not setting `+$65` to zero.

A faithful port must not invent a blanket “clear subtype on every state exit”.

The field is reset by one of these mechanisms:

```text
1. bulk actor-memory reset
2. explicit action-specific CLR.W $64
3. a later WORD producer that replaces $64/$65
```

Low-byte retypes alone do not clear high flags.

---

# 22. Hidden/non-adjacent producer audit

The direct executable instruction audit was broadened beyond adjacent `state4 + MOVE.W #imm,$64` patterns.

Results:

- direct low-byte producers `$12B92/$12FDA` were found and added;
- pre-common-state writes `$1381A/$1383E` were found and added;
- third-actor producer `$18290` was found and added;
- no demonstrated live register-dynamic `MOVE.W Dn,$64(Ax)` or `MOVE.B Dn,$65(Ax)` producer was found;
- apparent `MOVE.L D1,$64(A1)` byte patterns around `$1F2F6/$23F2A/$23F72` are data/table material, not executable actor writes;
- the only demonstrated live dynamic low-byte write is the lookup in `$24D7A`;
- the only direct low-byte arithmetic writers are `$1B44C/$1B452` and the three linked-target `$08/09 -> $06/07` pairs;
- no direct bit operations target low byte `+$65`;
- no direct `CLR.B/NEG.B/NOT.B/NEGX.B` to `+$65` and no direct `ADDQ/SUBQ.B` to `+$65` were found.

This closes the search for direct executable producer forms to the extent demonstrable from the static ROM image. No additional indirect producer is demonstrated by current evidence.

---

# 23. Complete lifecycle model

The field lifecycle is not one linear chain. It is a state-dependent graph with multiple producer classes.

## Producer class A — raw/action callback

```text
raw callback
-> linked A2 (or occasional A3)
-> state4 assignment
-> initial low subtype
```

## Producer class B — collision FIRST

```text
source profile
-> FIRST selector
-> carry predicate
-> receiver A1 state4/subtype write
-> optional normalization
```

## Producer class C — collision SECOND

```text
FIRST carry clear
-> target profile SECOND selector
-> optional A1 overwrite/reassert/retype
-> optional normalization
-> optional linked-A2 producer
```

## Producer/retype class D — state4 callback

```text
existing low subtype
-> current callback predicates
-> byte or word transition
```

## Producer/retype class E — external linked-target prone callback

```text
08 -> 06
09 -> 07
```

followed later by the inverse state4 return:

```text
06 -> 08
07 -> 09
```

## High-byte lifecycle

```text
word producer may initialize/reset flags
critical tier may BSET bit7
byte subtype retypes preserve that flag
08/09 terminal callback explicitly BCLR bit7
later word producer may replace all flags
```

## Terminal lifecycle

Low subtype may remain stale after main state exits. It is only meaningful as a state4 subtype when main state proves state 4. Bulk resets, explicit CLR.W paths, or future word producers eventually replace/clear it.

---

# 24. Two exact named chains already closed

## Perfect Plex

WIP-backed Perfect Plex callback family contains:

```text
$15154  MOVE.W #$0021,$64(A2)
```

Thus linked victim:

```text
initial subtype21
-> state4 callback $1BD6C
-> critical tier +$71==02 ? 09 : 08
```

This chain is ROM-EXACT.

## Boss Man Slam

WIP-backed Boss Man Slam impact contains:

```text
$17100  MOVE.W #$002A,$64(A2)
```

Then:

```text
2A
-> shared callback $1B1D0
-> special 2A branch
-> 08
```

This chain is ROM-EXACT.

---

# 25. Final closed issues

The following requested issues are now **ROM-EXACT / CLOSED** structurally:

1. State4 dispatcher and exact role of low `+$65` under main state 4.
2. Complete state4 subtype table `$00..$2A`.
3. Expanded direct action-side state4 producer inventory, including non-adjacent/byte/third-actor producers.
4. Exact five direct initial `$05` producers.
5. Complete direct state4 consumer/retype graph for every subtype callback where a low transition exists.
6. Exact external `$08->$06` and `$09->$07` linked-target retypes and inverse `$06->$08/$07->$09` return.
7. Byte-write versus word-write flag-preservation semantics.
8. Correct two-stage collision dispatcher: FIRST 15 entries + SECOND 13 entries.
9. Exact source-row and target-row provenance: source profile supplies FIRST; target profile supplies SECOND.
10. Complete profile selector table `$00..$21`.
11. Exact FIRST carry behavior for every FIRST index.
12. Exact FIRST explicit subtype behavior and its corrected branch predicates.
13. D1 provenance for FA/FE `$24784` gate.
14. Exact normalization helper `$24D7A`, range gate, orientation gate, and full table.
15. Exact SECOND handler behavior relevant to subtype writes/reasserts/retypes.
16. Exact FIRST→SECOND write precedence.
17. Direct SECOND linked-A2 producers `$01/$09/$0B`.
18. Correct multi-class provenance of subtype `$09`.
19. Critical-stamina tier classifier `$24EC2` and exact threshold `<=24`.
20. Bit7 critical-tier latch producers and explicit clear.
21. Frame ordering callback → collision → damage/tier recomputation.
22. Actor-slot iteration order and same-frame/next-frame first state4 entry.
23. Current-turn nature of `+$37.bit4`.
24. Bulk reset, explicit CLR.W paths, and deliberate absence of generic state-exit subtype clear.
25. Static direct-writer audit showing no additional demonstrated live direct producer form.
26. End-to-end parametric lifecycle of state4 subtype and high-byte latch.

At this point there is no remaining structural `victim +$65` question that blocks a ROM-faithful implementation model.

---

# 26. Minimal remaining items not provable from current evidence

These are not missing mechanics; they are semantic/reachability questions that the current static evidence cannot honestly resolve further.

## NOT PROVABLE FROM CURRENT EVIDENCE — human-readable move identity for raw `$33/$56`

Their producer logic is exact, but assigning a wrestler/move name without an external trace or stronger WIP mapping would be inference.

## NOT PROVABLE FROM CURRENT EVIDENCE — human-readable semantic names for every numeric subtype/field

The machine behavior of the numeric subtypes is closed. Labels such as “stun”, “fall”, “bounce”, etc. must not be invented solely from control flow.

## NOT PROVABLE FROM CURRENT EVIDENCE — gameplay meaning of the FA/FE cursor comparison

The machine predicate is exact:

```text
D1 = inner selected target-slot cursor (slot+1)
D1 != WORD[$2E(A1)]
```

Its higher-level gameplay meaning is not established by the present evidence.

## RUNTIME-DEPENDENT, NOT AN OPEN STATIC RULE — exact `$08/$09` result for one particular hit instance

The rule is closed, but the instance outcome requires runtime values:

```text
stamina tier
slot ordering
after/before-damage timing
collision-tail latch opportunity
current-turn +$37.bit4
```

It is therefore incorrect to demand a single per-move static result.

## NOT PROVABLE STATICALLY WITHOUT BROADER DYNAMIC CHOREOGRAPHY — which of all mathematically possible source-profile/target-profile pairs occur in every gameplay situation

The dispatcher matrix itself is closed exactly. Enumerating only *reachable* source-target profile pairs for every move would require a broader action/choreography reachability project or runtime traces and is outside what `+$65` needs structurally.

No other unresolved point found in this block changes the proven lifecycle or write precedence.

---

# 27. Implementation implication

This research is sufficient to replace any provisional “one-shot prone subtype” approximation with a ROM-faithful state model later, provided implementation preserves:

```text
main-state gating
low-byte vs word writes
high-byte bit7 latch
FIRST carry
FIRST/SECOND profile independence
normalization order
actor-slot scheduling
post-collision damage/tier timing
current-turn contact37 rebuild
stale low subtype after state exit
```

That implementation step is **not part of this research task** and has not been performed.

---

# FINAL VERDICT

**`victim +$65` lifecycle research: ROM-EXACT structurally CLOSED.**

What remains unlabelled is semantic naming or concrete runtime-instance reachability, not an unknown producer/consumer/precedence/reset mechanism.
