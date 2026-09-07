# WIP58 — raw $10 actor32 candidate context classification

Status: source/research checkpoint only. No BIN/CUE. No ROM files.

## ROM-EXACT / evidence-preserving addition
- `$1F354` is no longer an unclassified state-2 actor32 producer.
- Existing Test130/Test131 ROM pins place `$1F354` inside the AI state-0 distance/approach path; it sets `+$32 bit2` while entering main state 2 as part of AI approach selection.
- The raw-$10 actor32 candidate profile now exposes this site as `ARCADE_PRONE_RAW10_ACTOR32_CONTEXT_AI_STATE0_APPROACH`.
- `$1C4CA`, `$1C52E`, and `$1CED0` remain context-unknown exact state-2 producers.

## Important correction to provenance model
- It is not yet valid to assume exactly one of the four state-2 producer sites is the universal raw-$10 source.
- `$1F354` is a generic AI approach producer and may be inherited by a later AI raw-$10 action; therefore provenance is state-history dependent.
- Context classification does not authorize live release by itself.

## Safety
- raw `$10` remains blocked by `ARCADE_PRONE_RELEASE_BLOCK_ACTOR32`.
- No new state-2 SET is wired into `game.c`.
- No `release=true` activation.
- No 32-tick fallback removal.
- No `tick==12` replacement.

## Tests
- New Test342 verifies `$1F354` is classified as AI state-0 approach while remaining unpromoted as a raw-$10-specific source.
- Directed Tests329..342 relevant PASS, including Test340 safety wiring.

## Persistence
- Library: `/WWF-Saturn/WWF-Saturn-Test255-WIP58-source.zip`
- SHA256: `80edc177f367ce1b0db7ea7936e4c401d4fee763c8980d0b7510f34a4799dce2`

## Next
Classify caller/control-flow context for `$1C4CA`, `$1C52E`, and `$1CED0`, then model actor32 provenance as inherited state history by control path rather than forcing a single universal raw-$10 producer.
