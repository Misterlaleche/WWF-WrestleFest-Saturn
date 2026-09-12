# Gameplay RUNTIME-16 — Integration Master acceptance

Date: 2026-09-12

External checkpoint:
`BLOCK-GAMEPLAY-MASTER-01-RUNTIME-16(1).zip`

ZIP SHA256:
`0d8ddac0cd849b3c65a7c354aa6bab7606316b883e412380fc5b8fa3bce57ea9`

Gameplay branch:
`gameplay-runtime-r01`

Final branch HEAD:
`1853f5564b8a00ddd5883a68169993305a40cfae`

Source integration commit:
`8589e9ef796c2ed6ed00fb9a9f07c8eda3c96f24`

## Independent Integration Master audit

- package SHA256SUMS: **278/278 PASS**;
- canonical main 68000 reconstructed directly from `wwfwfest.zip`:
  SHA1 `a726c46f350723ea73158bc78055d17d418b14e2`;
- `tools/audit_runtime16_rom.py`: **PASS** against the reconstructed canonical program;
- HOST full suite rerun independently with
  `-std=c11 -Wall -Wextra -Werror -pedantic`: **PASS**;
- supplied SH-2 cross compile: **PASS**, 36 non-Saturn-specific translation units;
- supplied full Saturn ELF link: **PASS**, 41 objects;
- unresolved project symbols: **0**;
- ELF SHA256:
  `bf33e0eac805fa345dd55f46a1cb2a976fad73bb85ca2a41a57f04998e176d02`;
- no disc BIN/CUE generated. Binary files in `assets/` are data payloads, not disc images.

The linker log contains RWX LOAD-segment warnings but no link failure; this is not promoted to hardware validation.

## WP28 advance

RUNTIME-16 advances the real normal-frame chain to:

`$00514E -> $008B3A -> $02088E -> $020958 -> $026936 -> $02983C -> $00F560 -> $00F18A -> $01C150 -> $01F914 BLOCKED`

New real ROM-derived owners integrated:
- `$00F560`: pair/relation aggregate;
- `$00F18A`: actor raw-state prepass.

Next mandatory unbound WP28 owner:
**`$01F914` fixed referee scheduler**.

The full normal `wf_runtime_frame()` is therefore still **NOT COMPLETE**.

## Fail-closed dependencies preserved

RUNTIME-16 correctly does not fabricate:

- `$00F560 -> $0021B4` shared RNG path; exact inherited 68000 register context remains unproven;
- `$00F560 -> $00F98C` active scene/transition route;
- `$00F560 -> $00A0E0` active audio-producer state machine; WP24 remains sole backend owner;
- `$00F18A -> $00DCDE` human controller path;
- `$00F18A -> $010E86` movement/follow helper;
- `$00F18A -> $01129E` state-2 random/helper path.

Direct-match and attract gameplay bootstrap remain BLOCKED pending exact ROM initialization.

## Ownership

Gameplay continues to own:
- WP01/State5 actions;
- WP28 gameplay scheduling;
- CPU/AI;
- physics/collision/damage;
- ground/release;
- pin/referee gameplay;
- ring-out;
- TAG/Royal gameplay;
- WP31 objects/effects/props;
- shared `$21B4` RNG once exact context/order is proven.

Integration Master does **not** merge or rewrite Gameplay source here. It records and consumes the named audited checkpoint/contract.

`$01F914` crosses referee state dispatch with `$247C/$27B8` render-coordinate/object-queue work. Renderer ownership must remain explicit; no no-op/synthetic referee representation is allowed.

## Verdict

**RUNTIME-16 ACCEPTED as the current Gameplay checkpoint.**

This is a real audited advance, not FINAL Gameplay closure.

Hardware: **NOT TESTED**.
BIN/CUE: **NO**.
