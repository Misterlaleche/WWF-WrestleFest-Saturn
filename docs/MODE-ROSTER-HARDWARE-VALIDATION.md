# MODE + ROSTER hardware validation

Date: 2026-09-11  
Target: Sega Saturn NTSC-U + Terraonion MODE  
Canonical combined checkpoint: **WF-SATURN-MODE-ROSTER-INTEGRATION-TEST03**  
Reference BIN SHA-256: `c9e81bb6e7a775079ec3c2783ad06d73ad48aea560b4a0e89c78dcf51144fc05`

## Verdict

**HARDWARE-PASS for the combined isolated MODE + ROSTER frontend block.**

Validated route:

```text
MODE
-> confirm TAG or ROYAL
-> ROSTER
-> navigate
-> confirm wrestler
```

This status means the combined rendering/input handoff is suitable for source-level master integration. It does **not** make the isolated harness the canonical game main loop, and it does not overrule the original arcade ROM or CLEAN-R01 ownership.

## Inputs

- `WF-SATURN-BLOCK-MODE-01-ROMEXACT-CANDIDATE`
- `WF-SATURN-BLOCK-ROSTER-01-TRANSPARENCY-FIX-CANDIDATE`

The blocks were integrated from source/assets. BIN/CUE images were never concatenated.

## Hardware-validated closures

### MODE -> ROSTER transition flash

Observed failure: a yellow/orange flash when confirming TAG/ROYAL.

Cause: ROSTER palette/VRAM reconfiguration could occur while the previous MODE bitmap was still visible, causing old pixels to be scanned with the new palette.

Validated correction:

```text
blank/synchronize display
-> configure/load ROSTER CRAM/VRAM
-> restore display
```

Result: no observed yellow/orange handoff flash.

### ROSTER transparent background composition

Validated composition:

- NBG0 = transparent moving ROSTER foreground;
- NBG2 = fixed background;
- existing scroll geometry/cadence preserved.

This closes the prior black-background regression caused by disabling NBG0 transparent-code processing.

### Green wrestler-confirmation state

The arcade-style bright-green confirmation is preserved while the cursor remains visible.

Correct composition order:

```text
resident portrait/background
-> green confirmation mask
-> cursor
-> selected 96x96 VRAM patch
```

### TEST02 confirmation flash

Observed failure: during wrestler confirmation, the full NBG0 foreground disappeared for roughly one captured video frame, leaving only the fixed background.

Cause: `confirmed_rebuild()` called a helper that cleared and repopulated the full 1024x256 NBG0 bitmap.

TEST03 correction:

```text
keep NBG0 resident
-> compose selected 96x96 patch
-> wait VBLANK-IN
-> upload only selected 96x96 patch
```

No full NBG0 clear/rebuild, CRAM rewrite, background rewrite, or scroll reset occurs on confirmation.

Hardware result: **PASS**. Green selection appears, cursor remains, and the flash is gone.

## Proven implementation invariants for master/runtime

The master import must preserve:

1. TAG/ROYAL result propagation from MODE into the corresponding ROSTER navigation table.
2. Display blank/synchronization before MODE -> ROSTER CRAM/VRAM changes.
3. Transparent NBG0 moving foreground over fixed NBG2 background.
4. Existing scroll constants/cadence from the proven block.
5. Resident ROSTER foreground during selection/confirmation.
6. Confirmation update limited to the selected 96x96 rectangle.
7. Confirmation patch begins at VBLANK-IN.
8. Composition order: portrait/background -> green mask -> cursor.
9. No isolated-harness freeze/timeout behavior in the master runtime.
10. No replacement of CLEAN-R01 main-loop/state ownership with the test harness.

## Runtime data boundary

At minimum the runtime must carry:

```text
selected_match_mode  = TAG or ROYAL
selected_roster_slot = validated slot index
```

Bind `selected_wrestler_id` only through the ROM-backed roster/move mapping authority. Do not infer or invent an ID mapping from presentation slot order if the canonical mapping has not yet been bound.

## Master validation gate

After source-level import into the master, re-run on physical Saturn:

```text
TAG -> ROSTER -> scroll/navigation -> confirm
ROYAL -> ROSTER -> scroll/navigation -> confirm
```

PASS requires:

- no yellow/orange transition flash;
- no foreground-disappearance confirmation frame;
- correct cyan -> bright-green confirmation;
- cursor remains visible;
- correct TAG/ROYAL navigation;
- background and scroll composition unchanged;
- only a proven next-state owner handles the post-confirm transition.

## Source package

The full hardware-reference source, binary assets, reference BIN/CUE, provenance hashes and integration contract are kept in the external handoff package:

`WF-SATURN-MODE-ROSTER-HARDWARE-PASS-HANDOFF-R01.zip`

Protected/reference binary assets are intentionally not duplicated into this GitHub repository.
