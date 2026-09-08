# WIP151 CHECKPOINT — PUNTO2 actor32/raw10 authority integration

## Scope
Integrates the final PUNTO2 `actor +$32.bit2 / raw $10` authority into the latest canonical WIP149 Ground Actions source, following the pre-existing WIP150 research checkpoint without enabling physical held release.

Fidelity labels used below are deliberate:
- **ROM-EXACT**: directly demonstrated by ROM/event provenance.
- **PROVISIONAL**: existing native bridges retained unchanged.
- **REGISTERED BUT NOT IMPLEMENTED**: exact ROM event catalogued but not wired to a native event without a one-to-one proof.

## Source authority and corroboration
WIP150 already exists in GitHub as the corrected byte-level actor32/raw10 research checkpoint; this WIP151 is the runtime/source integration checkpoint.

The project-source archive named `WWF-WrestleFest-Saturn-PUNTO2-actor32-raw10-ROM-EXACT-COMPLETE.zip` did not surface through the available project-file retrieval interface in this session. The integration therefore applies the final PUNTO2 rules supplied by the project owner and independently rechecks the machine-level event set against the known-good WrestleFest main program.

Main program SHA1 authority: `a726c46f350723ea73158bc78055d17d418b14e2`.

No ROM bytes are included in this checkpoint/source package.

## ROM-EXACT actor32 direct-event catalog
The legacy 9-entry immediate `BSET/BCLR #2,+$32` API is preserved for compatibility.

A new complete direct-event catalog contains **28 exact sites** affecting actor byte `+$32.bit2`:
- 7 explicit `BSET` -> SET;
- 2 explicit `BCLR` -> CLEAR;
- 9 `MOVE.W #imm,+$32` -> CLEAR at all demonstrated sites;
- 5 `CLR.W +$32` -> CLEAR;
- 5 `ANDI.W #mask,+$32` -> CLEAR at all demonstrated sites.

Big-endian rule: actor byte `+$32` is the high byte of word `+$32/+33`, so `+$32.bit2` corresponds to **word mask `$0400`**.

`$985C ORI.W #$0003,+$32(A0)` is deliberately excluded: it changes only low-byte `+$33` bits and does not mutate `+$32.bit2`.

The new catalog records A0/A1/A2/A3 target role, operation, operand/mask and exact bit2 effect. Events without an exact native one-to-one event source are **REGISTERED BUT NOT IMPLEMENTED**, not guessed into runtime.

## ROM-EXACT raw $10 actor32 provenance
New pure resolver `arcade_prone_raw10_actor32_resolve()` implements only demonstrated provenance:

1. **STATE0_EXACT** -> known CLEAR via `$114CC BCLR #2,+$32(A0)`.
2. **CONTROLLER_STATE1** -> known CLEAR: exact state0 clear is inherited through controller entry `$F228`; its state1 callback family contains no actor32 mutation.
3. **STATE6_FIRST_ENTRY** -> known CLEAR via `$11E04 BCLR #2,+$32(A0)` in callback `$11DE0` first-entry path.
4. **OTHER_STATE1** -> no default. It is known only if the caller supplies the exact last direct actor32 event proven to have executed on that actor; the event catalog determines SET/CLEAR.
5. Missing/invalid provenance -> **UNKNOWN**.

An arbitrary `actor32_shadow` boolean with no exact event provenance is not accepted as proof for OTHER_STATE1.

## Safety / live runtime
- `state_set()` remains actor32-free. No native-state resemblance is used.
- `$1C4CA/$1C52E/$1CED0` remain unwired in `game.c`.
- Existing exact live event wiring remains unchanged (`$114CC`, `$1F354`, `$214A4`).
- Legacy/global `arcade_prone_hold_release_blockers(0x10,0)` still contains `ARCADE_PRONE_RELEASE_BLOCK_ACTOR32`.
- `arcade_prone_hold_live_release_ready(0x10,0)` remains false.
- New context-aware blocker API can remove only the actor32 blocker when provenance is exact; it is not wired to physical release yet.
- Unknown state1 provenance remains blocked.
- No `+$42`, `+$0E`, `contact37`, 32-tick fallback or `tick==12` behavior changed.
- No BIN/CUE generated.

## Tests
Added:
- Test410 — complete 28-event catalog, 7 SET / 21 CLEAR, 19 word-form events, big-endian `$0400` semantics, `$985C` exclusion, legacy 9-entry compatibility.
- Test411 — raw10 provenance policy: state0/controller-state1/state6-first-entry CLEAR; OTHER_STATE1 uses exact last event; absent/invalid event stays UNKNOWN/blocked.
- Test412 — safety guard: no `state_set()->actor32`, no CPU-dispatch SET runtime wiring, legacy raw10 blocker remains, UNKNOWN state1 blocked.
- Test413 — ROM-byte pinning of all 28 direct events and `$985C` non-event.

Harness fixes while integrating tests:
- Tests408/409 are registered before the runner loop so they actually execute.
- Test346 declares its existing `arcade_stamina.c` link dependency.

Directed actor32/raw10 regressions pass through Test413. A full host-suite run progressed through the existing suite without a detected failure before the command execution window expired; this is not claimed as a completed full-suite pass.

## Ground Actions status after PUNTO2 integration
Actor32 is no longer an undifferentiated boolean blocker for raw `$10`: exact provenance classes are now representable. However raw `$10` is **not globally unblocked**. Other state1 histories with no exact last event remain UNKNOWN by design.

Next work remains selective Ground materialization from WIP149: carry exact actor32 provenance into the actor runtime only at one-to-one demonstrated events, then combine it with the existing 32-bit-Z/typed-held-operation readiness contract before enabling any isolated physical release.

Source ZIP: `WWF-Saturn-Test255-WIP151-source.zip`
SHA256: `80aa6ab76c73577184426aa383c90ba35de3d1be740ea132db45001d473905ff`
