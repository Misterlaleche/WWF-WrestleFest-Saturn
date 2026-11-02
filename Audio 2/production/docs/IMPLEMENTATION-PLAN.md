# Audio 2 — implementation and verification plan

> Execution: autonomous, as requested. Continue existing source; preserve the complete historical tree. Use systematic debugging and failing regression cases before behavioral changes.

**Goal:** a reproducible native Audio 2 subsystem with honest acceptance gates, original ROM commands, no prerecorded music, and a bounded integration contract.

**Architecture:** retain the existing ROM-executing sequencer and native 32-slot SCSP FM bridge, plus the 68000 ADPCM service and four DSP PCM voices. Consolidate the strongest independently verified sources into this isolated directory. First fix real scheduling and reporting defects; native synthesis accuracy remains a separate gate. No R04, scene, gameplay or graphics source is changed.

**Authority:** user master specification (2026-11-01), original `wwfwfest.zip`, checkpoint `db18be40a1d79de200408207573e9f45bf73bf2c`, R04/A53 reference. Public GitHub publication excludes ROMs, generated protected resources, captures, BIOS and executable objects containing original data.

## Global constraints

- Saturn emulator: standalone Mednafen 1.32.1 exclusively. Historical Beetle measurements are historical and cannot satisfy this session's emulator gate.
- Native driver must not depend on expansion RAM. No full music PCM or CD-DA; original sample data may be generated locally.
- Zero FM drops, underruns, lost required commands and hangs. Do not enlarge buffers, lead time or lateness tolerances to conceal a failure.
- All 18 music commands, 98 SFX commands, original 0x1F fade and transitions must be tested. A short prefix or a host model is not full validation.
- Production readiness requires all software gates and owner comparative listening; hardware status remains HARDWARE PENDING until owner tests physical Saturn.

## Review focus

1. Clock wrap, late generation versus delayed service, and backlog during interpreter work.
2. Transactional resource rejection before any executable memory write; ABI and interval mismatches.
3. Multi-voice prefill/refill under concurrent FM and command traffic, preserving priority suppression.
4. True original-driver stop/change/fade behavior, not fabricated sequencing.
5. Missing or failed evidence must never become a passing gate; counters must measure actual discarded events as well as batches.

## Tasks

- [x] Recover all Audio 2 source and requested context; verify every Git blob and the historical 380-file manifest. Check branch and main tips.
- [ ] Reproduce historical queue/service faults using cost-aware tests. Add cooperative service and explicit failure state, preserving musical event traces.
- [ ] Rebuild native 68000/SCSP firmware with prepared PCM service; validate pack parser, decoder, priorities, underflow and recovery.
- [ ] Audit and fix only proven native mapping defects; preserve quantified architectural discrepancies.
- [ ] Provide portable resource generator and cross build, stable public API, memory map and ABI checks. New build profiles receive new hashes; old pins are not weakened.
- [ ] Execute isolated Saturn harness with Mednafen 1.32.1; capture reference/native WAVs with exact executable and capture identities.
- [ ] Run complete music/effects/transitions/stability and fault-injection coverage; run identical TAG/Royal scenarios with and without audio if integration prerequisites permit. Mark unexecuted cases BLOCKED.
- [ ] Produce CODEX-INTEGRATION.md and AUDIO-FINAL-AUDIT.md, rerun verification, independent review, publish identifiable commits and an unmerged PR.

## Decisions and progress

- 2026-11-01: `audio-2` still points to checkpoint; `main` is parent `aedca6827c7801a88dbb1eae280830cd4776b95f`. Recovered 415 selected files, 304 unique blobs, zero hash mismatches. `verificar_paquete.py`: 380/380 PASS.
- Local Git is an explicitly identified verified partial source export because CLI private authentication is unavailable. Remote publication will use the authenticated GitHub connector with the real checkpoint as parent and a base tree, preserving every unrelated path.
- Latest SH-2 sources selected from `z80-inline-tabla`; prepared PCM source selected from `pcm-servicio`. This combination has no inherited integrated pass.
- Historical dropped_fm counts batches, not individual writes; late_count can increment twice per event. Current state RUNNING/error0 after discarded FM is a reporting defect.
