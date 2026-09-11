# INTEGRATION MASTER R01 — PARALLEL OWNERSHIP

## This branch owns

- Saturn memory ownership and budgets (WP32 integration).
- CDFS/disc/loading contract (WP33 integration).
- cross-block no-clobber policy.
- master scene asset manifest format.
- later source-level assembly of frontend + gameplay + audio + renderer.

## This branch does not own

- State5/action callbacks;
- CPU/AI;
- physics/collision/damage;
- pin/referee/ring-out/TAG/Royal gameplay;
- unresolved frontend ROM choreography.

Those remain with their dedicated blocks, especially `gameplay-runtime-r01`.

## Merge rule

Do not copy gameplay RUNTIME checkpoints into this branch by hand while the gameplay owner is active. Consume a named checkpoint/commit after audit. Likewise, WP24 remains the single audio backend owner.

## Current state

Memory/CDFS contract: integration candidate.
Full game runtime: not yet assembled.
BIN/CUE: not requested and not generated.
