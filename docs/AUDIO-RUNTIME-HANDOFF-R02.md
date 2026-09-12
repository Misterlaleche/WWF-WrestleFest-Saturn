# Runtime Audio Handoff R02 — Re-audited

Date: 2026-09-12

## Authority decision

**R02 supersedes R01 as the canonical external runtime-audio handoff.**

Package:
`WWF-RUNTIME-AUDIO-HANDOFF-R02-REAUDITED.zip`

Package SHA256:
`49335637b8ba6d26b361c88e3886f620e9f52c076df3d4bfb44accf3701ccb5c`

R01 remains historical only and must not be used as the current packaging authority.

## Why R02 replaces R01

The R01 re-audit found two packaging-quality defects:

1. its direct WP24 source snapshot was incomplete;
2. six bridge files were semantically equivalent normalized copies but were described as an exact GitHub snapshot even though their Git blob SHAs were not byte-identical.

R02 corrects both:
- complete current WP24 source set is included;
- both current WP24 host tests are included;
- all 12 included source/test blobs byte-match the audited Git snapshot;
- owner-map markdown cleanup does not change ownership semantics;
- exact `0x1F` behavior remains fail-closed/open.

These were handoff/provenance defects, not a semantic change to the producer matrix.

## Package audit

Top-level SHA256 manifest:
**PASS — 31/31 entries byte-exact.**

Nested ZIP integrity:
**PASS** for all four nested ZIPs.

Authority package copies match the project originals byte-for-byte:

- `WWF-Saturn-Audio-Complete-Integration(1).zip`
  SHA256 `9c262da297117a38ee321df3429c0b51de039d1664dbaa5638cf7450bfe0bee8`

- `WWF-Saturn-Audio-FINAL-DEFINITIVE-ROM-EXACT-HANDOFF(1).zip`
  SHA256 `00d3edf2757aad38bd208af19639705afa0b285172550ea07c89e3d675107fdf`

## Producer matrix

Canonical current counts remain:

- total ROM-audited producers: **173**
- `INTEGRATION_READY`: **14**
- `OWNER_SCOPE_ONLY`: **4**
- `UNRESOLVED`: **155**

No `REGISTERED_NOT_IDENTIFIED` producer is promoted to ready.

Critical correction remains:
- low-stamina producer = `0x009088 -> command 0x0B`, JSR at `0x00908C`;
- `0x0263CC -> 0x0B` is **REGISTERED_NOT_IDENTIFIED**, not low stamina.

## Ownership / architecture

WP24 remains the sole backend owner.

Audio dispatch is a side effect at the proven equivalent owner callback/state/branch.

Forbidden:
- global audio semantic observer;
- new WP28 audio scheduler phase;
- binding unresolved producer semantics;
- using WIP160/WIP175/WIP176 as canonical runtime authority.

The 14 ready producers may be integrated at their proven owners.

## 0x1F

Exact `0x1F` lifecycle is still OPEN.

Because `wf24_command_bus_init()` requires a non-null `special_fade_1f` callback, the consumer must wire a deliberate fail-closed callback that:
- reports failure;
- performs no fade;
- performs no state mutation.

A successful no-op is forbidden.

## Music

WP24 transport remains **HARDWARE-PASS** on real Saturn.

The 18 music-command renderer/research is authoritative.

Still OPEN:
- exact final cue lifetime/termination;
- exact `0x1F` fade lifecycle;
- simultaneous full-gameplay music + SFX + voice validation;
- integrated final Saturn audio validation.

TEST-05 long captures are validation material, not production looping authority.

## Source reconciliation with Integration Master

R02 records Git snapshot:
`3287ac70c2010b3706aaf3a76003e56f7e1fa25b`

The 12 WP24 source/test Git blob SHAs recorded by R02 were independently compared against current `integration-master-r01`.

Result:
**12/12 byte-identical Git blobs already present in Integration Master.**

Therefore R02 requires no source overwrite in the master. The canonical handoff/provenance reference is updated while preserving the already-identical current WP24 source.

## Validation claims from R02

- package audit: PASS
- authority ZIP copies: PASS
- producer matrix 173/173: PASS
- owner map counts 14/4/155: PASS
- complete WP24 source snapshot: PASS
- Git blob SHA matching: PASS
- WP24 host compile: PASS
- command-bus host test: PASS
- runtime-hook host test: PASS

Not claimed by R02:
- SH-2 cross-compile of the consuming runtime;
- full Saturn ELF link of the consuming runtime;
- integrated hardware validation;
- closure of exact `0x1F`;
- closure of final cue lifetimes.

## Integration decision

From this checkpoint forward:

**Use R02 instead of R01.**

R01 package SHA `cc1c9ed44edcde0237770521123072c3bbec6927bf37184c157e7b092858db65` is historical/superseded and must not be cited as the canonical current runtime-audio handoff.
