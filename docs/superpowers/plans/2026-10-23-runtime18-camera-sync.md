# RUNTIME-18 camera synchronization

> Execute inline with superpowers:executing-plans; obtain one fresh whole-diff review at the end.

**Spec:** Continue 426ed17. Fix camera/ring/sprite desynchronization with direct ROM evidence, verify in Mednafen, deliver a reproducible BIN/CUE. No provisional code or frontend changes.
**Architecture:** The ROM's four scroll WORDs at $17E6/$17EA/$17EE/$17F2 are authoritative. Camera phases and all video consumers must share them; preserve original phase order and register effects.
**Constraints:** sources/ is read-only; use existing runtime18/direct-combat checkout. No forced gameplay outcomes, guessed offsets or physical-hardware/ROM-EXACT claims.

### Task 1: Prove and correct camera ownership
- Add a differential against untouched ROM $26936/$2983C, including stale adapter state, every camera class, scene bounds, paired scrolls, unsigned edges and D0–D7. Observe RED before changing production code.
- Import canonical state into camera phases, publish successful updates, and make render consumers read canonical scrolls. Correct clamp semantics only as demonstrated by the oracle.
- Add a complete-match invariant for shared scroll state; run differential and integration to GREEN.

### Task 2: Verify integration and visual behavior
- Run the complete HOST suite, transport tests, sanitizers, original R17 and R18 ROM matrices. Resolve any actual regression with evidence.
- Build normal and deterministic no-input SH-2 discs. Inspect moving ring/sprites in Mednafen and retain screenshots, scroll samples, endpoint and executable hashes.

### Task 3: Consolidate and deliver
- Obtain independent whole-diff review and resolve material findings.
- Update current-state/reproduction documents, commit checkpoint, package source/assets/discs/evidence, rebuild exported source and verify delivered hashes.
- Deliver normal BIN/CUE and source checkpoint with the exact validation scope.
