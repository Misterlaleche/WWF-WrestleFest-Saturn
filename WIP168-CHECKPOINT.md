# WIP168 CHECKPOINT — first context-complete held release contract: chained raw `$23`

## Scope
Combine WIP123/WIP132/WIP153/WIP160/WIP161/WIP165/WIP166/WIP167 into the first held-Ground context whose physical callback operation is fully evaluable from ROM-exact provenance. Contract/research checkpoint only: no runtime release enabled because the latest source package is not materialized in this session; no fallback removed; no `tick==12` change; no BIN/CUE.

## Exact provenance class
This contract applies only when all of the following are demonstrated for the current attacker:

```text
1. chained Ground entry through standard successful held contact -> state7 -> state0 (WIP132)
2. full 32-bit Z shadow preserved; starting high word = floor T=$0140, arbitrary fraction retained
3. state0 init executed: probe42=0 and actor32.bit2=0
4. actor32.bit0 provenance is exact CLEAR and no intervening bit0 event occurred (WIP166)
5. global $1C007E provenance is exact 0 from the applicable clear class, with no intervening direct 3/4 writer (WIP167)
6. raw23 initial internal callback substate is the `$44=1..4` held family from WIP161
7. callback first-entry semantics from WIP160 are respected
```

This is intentionally narrower than “raw23 selected from state0”.

## Exact raw23 launch
Initial raw23 held family owns:

```text
$15DE2  +$5C=$0500
$15DE8  +$5E=$0048
$15DF4  +$01=2
```

No `+$42=+$20` writer belongs to this initial setup/gate; in this chained state0 provenance probe remains exact zero.

First callback entry initializes the motion tuple and does not consume the normal held gate. The common movement pass then performs the first 16.16 vertical integration.

## Exact current-turn contact
With global classifier provenance 0, actor32.bit2 clear and bit0 clear, WIP167/WIP106 select `$2818E` with threshold `$0140`.

For arbitrary inherited fractional low word F:

```text
Delta23(n) = [n*$0500 - $0048*n*(n-1)/2] / 256 px
```

WIP165/Test432 proves:

```text
n=36 -> Z high >= $0140 for every F -> contact37=0
n=37 -> Z high <  $0140 for every F -> next pre-callback pass sets contact37.bit4
```

The contact turn is therefore fraction-invariant in this provenance class.

## Exact callback operation
At initial raw23 gate `$15DFE`, current-turn contact37 is sufficient for the held-counter operation. The contact path reaches:

```text
$15E10 CLR.W +$22(A0)
```

By WIP145/WIP147 common-updater semantics, this is exactly:

```text
CLEAR_AND_ADVANCE
```

followed by the normal common descriptor-clock update in the same actor pass.

No `$11152/$11192/$11412` helper is required for this preparatory held-counter clear.

## Readiness verdict
For the exact provenance class above:

```text
raw $23 initial held gate = CONTEXT-COMPLETE / ROM-EXACT EVALUABLE
```

Every required input for the physical held-counter operation is closed:

- callback-first-entry semantics: known
- 32-bit Z/fraction: known/preserved
- probe42: exact 0
- actor32 bit2: exact clear
- actor32 bit0: exact clear by provenance condition
- global classifier mode: exact 0 by provenance condition
- motion tuple: exact
- internal +$44 family: exact 1..4
- current-turn contact37: exact after integration37
- held operation: contact-only CLEAR_AND_ADVANCE
- helper carries: not required for this initial gate

## Why release is still not switched on here
The latest canonical gameplay source ZIP (`WIP160` lineage) could not be materialized through the available Library retrieval interface in this session. Enabling a runtime branch against an older or reconstructed source would risk regressing WIP160 callback ownership corrections.

Therefore WIP168 is an implementation-ready contract, not a source mutation.

When the canonical source is materialized, the first safe live experiment is **not** “enable raw23 globally”; it is:

```text
enable raw23 initial held physical operation only when every WIP168 provenance-valid flag is true;
otherwise retain the existing conservative fallback.
```

## Validation
Local Test432 ROM-pins raw23 launch/gate/counter clear, state0 actor32/probe clears, classifier index0/bit0 gate/threshold, and exhaustively verifies all 65536 fractional low-word values for integrations 36/37.

## Safety
- No global raw23 release authorization.
- No release outside the exact chained provenance class.
- No 32-tick fallback removed.
- No `tick==12` damage migration.
- No source code changed in this checkpoint.
- No ROM bytes persisted.
- No BIN/CUE generated.

Main program SHA1: `a726c46f350723ea73158bc78055d17d418b14e2`.
