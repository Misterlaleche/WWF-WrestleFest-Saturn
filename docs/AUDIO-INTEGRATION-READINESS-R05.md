# AUDIO INTEGRATION READINESS R05

Canonical parent: `WWF-SATURN-AUDIO-ROM-EXACT-SOFTWARE-CLOSED-R04-MASTER-REAUDITED.zip`
SHA256 `c4898b654eb105a88cc24500de96c00b68d188582aa7d7f7dbb13f1b3315ae36`.

R05 audited package:
`WWF-SATURN-AUDIO-INTEGRATION-READINESS-R05-AUDITED.zip`
SHA256 `2f241bc267bcb7fa5c58a53e343982e648041835d5c3e746fe20d2fc4437675b`.

## Classification

- AUDIO SOFTWARE CLOSED: inherited from R04 MASTER-REAUDITED.
- **AUDIO INTEGRATION PARTIAL**: current R05 project-level state.
- AUDIO HARDWARE READY: BLOCKED.
- AUDIO HARDWARE CLOSED: BLOCKED.

## R05 closure

- 173/173 audio producer rows reverified against ROM.
- Producer counts: 14 INTEGRATION_READY, 6 ROM-EXACT CLOSED, 4 OWNER_SCOPE_ONLY, 149 UNRESOLVED.
- New closed owner contracts: $020098 (WP09), $00A67A/$00A7AC/$00A872/$00ACE0 (WP12), $00BFBC (WP13).
- $00A872 corrected to conditional 0x31/0x32 by actor low +$56 <= 5.
- $0263CC -> 0x0B remains UNRESOLVED and is not low stamina.
- R04 0x1F original-Z80-driver behavior remains unchanged.
- Added measurement-only performance probe; no semantic observer and no WP28 phase.
- Historical A16 SCSP resident/dynamic overlap is guarded out: resident ends 0x3C5E4, dynamic begins 0x3C600, three 0xA000 slots end 0x5A600, music ring remains 0x60000..0x6FFFF.
- HOST strict + 5 tests PASS.
- SH-2 project-owned strict PASS; exact dependencies compile; full relocatable audio link PASS.

## Static evidence

R04 core state: 38,236 B.
Optional perf probe: 268 B.
Instrumented state: 38,504 B.
A16 SFX staging: 40,960 B.
Audit relocatable text: 35,958 B.
External AUDPCM source is 980,196 B and its final residency is not assumed.

Music half deadline from the hardware-proven format is 371.519274 ms, cadence 2.691650390625 refills/s, payload 88,200 B/s. These are transport calculations, not CPU benchmarks.

## Exact blockers

The current Integration Master does not yet consume R04 Live VM and the gameplay Saturn entry remains fail-closed. Therefore Live VM cycles/frame, worst-case CPU, final stack watermark, final HWRAM/LWRAM fit, final source residency, end-to-end audio runtime, underrun stress and final hardware closure remain BLOCKED BY INTEGRATED BUILD / owner runtime / real Saturn.

WP24 must not close those gaps by inventing external producer semantics.
