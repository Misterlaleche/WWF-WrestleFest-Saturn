# New-chat handoff — WWF WrestleFest Saturn

Use this private repository plus `/WWF-Saturn/` in the persistent ChatGPT Library as the source of truth.

## Branch to resume from

Resume from **`test255-lod-handoff`**.
Do not resume from `main` for the current LOD work. `main` still represents the older canonical line.

Current handoff parent commit before this update:
`32bbc23a362b1f567c04337208f581890315cefc`

## Canonical baseline vs current WIP

- Canonical validated baseline remains **Test254 / V0.269**.
- Canonical source snapshot: `/WWF-Saturn/WWF-Saturn-Test254-V0269-source.zip`
- SHA256: `cb324b8c6493642854d1799597b8f4ee6b531041400b0d909a34608c17a78cb0`
- **Test255 / V0.270 is still WIP and must not be promoted to canonical until the user validates it on real Saturn hardware.**

Latest full source checkpoint to resume from:
- **Test255 WIP16**
- Library: `/WWF-Saturn/WWF-Saturn-Test255-WIP16-source.zip`
- SHA256: `dac151299786211086d10b42ba0f2df4156f2efd137792934d457c58ae9be374`

The current chat also has an incomplete local `wip17` working directory, but it was not yet validated/persisted as a new source checkpoint. Do not treat WIP17 as authoritative unless reconstructed from WIP16 and revalidated.

## Latest generated Test255 BIN/CUE

A structurally valid **Test255 / V0.270** BIN/CUE was generated successfully by GitHub Actions run:
`34048120742`

Artifact name:
`WWF-WrestleFest-Test255-V0270-BIN-CUE`

Hashes:
- BIN: `0d39383e2f8ca76a347888fac90b6cf0acb0d505466437fd76bc8d2349f5241d`
- CUE: `b8f5ab9791fde0ee026b63e9e55f1005e8f675a9a15e9141f81d4e6974ad7497`
- ZIP: `cc3f504a562bddc0aa758514d95308b20d96eefeaf76ac08ca6da1e03ff246d9`

The user has not yet reported the real-Saturn result, so Test255 is not canonical.

## Hawk / Animal / LOD work — current exact state

### Correct logical-to-arcade graphics mapping

The logical Saturn roster is 0..11, but the arcade graphics banks differ for Demolition and LOD:

- logical 0 Hogan -> arcade bank 0
- logical 1 Warrior -> arcade bank 1
- logical 2 Jake -> arcade bank 2
- logical 3 Boss Man -> arcade bank 3
- logical 4 Smash -> arcade graphics bank 10
- logical 5 Crush -> arcade graphics bank 11
- logical 6 Slaughter -> arcade bank 6
- logical 7 Perfect -> arcade bank 7
- logical 8 Earthquake -> arcade bank 8
- logical 9 DiBiase -> arcade bank 9
- logical 10 Hawk -> arcade graphics bank 4
- logical 11 Animal -> arcade graphics bank 5

Do not revert to the earlier incorrect WIP10 assumption that C10/C11 graphics were directly arcade banks 10/11 for Hawk/Animal.

### Roster/assets already closed

- 12-luchador runtime contract is 223 resident frames per `Cxx.PAK`.
- Hawk and Animal resident banks exist and are based on the corrected arcade banks.
- Smash/Crush were remapped back to the correct arcade source banks.
- `ROSTER.PAL` was expanded to 12 palettes.
- Hawk/Animal no longer inherit Hogan's moveset fallback.
- Hawk/Animal special stamina path is 500 in the recovered player path.
- Partner-dependent `Gxy` ground/PICK UP overlays were expanded to all 12 logical wrestlers.
- Partner-dependent `Sxy` grapple-struggle overlays were expanded to all 12 logical wrestlers.
- IDs 10/11 are encoded as `A/B` in generated CD filenames.

### Doomsday Device chain — ROM-EXACT evidence already closed

Animal special hold path:
- logical Animal 11 -> internal actor 5
- linked-context classifier at `$E1D4/$E232`
- K selects raw action `$89` in exact linked contexts
- descriptor `$1ABA4`
- callback `$1ABC0`
- pose sequence `269 -> 452 -> 453 -> 454 -> 455`
- finite visual duration 85 ticks
- victim is put into raw state `$FF`
- attacker `+$34 bit3` is set
- completion writes `+$24=2`, `+$22=0`, `+$60 bit7`
- event/effect `$32` is produced
- the reciprocal grapple link `+$26` between Animal and victim survives this hold

Hawk three-actor special path:
- Hawk callback `$12C22`
- descriptor `$12C0A`
- Hawk special three-actor frames include poses 459/460 and then 162/163
- victim's `+$26` is used to resolve the third actor, which is Animal during the hold
- victim receives state 4, selector `$19`, and raw damage `$20`
- the third actor receives state 5 / raw `$8006`

Collision/link producer:
- Hawk arms collision profile `$17`
- profile first-handler index 11
- handler `$247EC`
- victim is converted to type `$12`
- `$24DEC/$24E02` create the reciprocal `+$92` relation between Hawk and victim

Hawk launch flag:
- wrestler state 8 sets `+$32 bit1` in all ten transition variants
- Hawk `$12C22` consumes that exact bit with BCLR
- if it was set, Hawk enters the ROM special launch setup using `+$34 bit3` and the recovered launch vector values

### Latest closure from WIP16 — this is the exact point to continue

The previously missing final selection bridge was found:

**Hawk's AI enters `$12C22` only after an 8-tick wait and when its partner pointer `+$86` resolves a partner currently in state 5 / raw `$89`.**

This closes the Animal-hold -> Hawk-special producer as ROM-EXACT.

In other words, the next chat should treat the trigger chain as demonstrated:

`Animal linked hold raw $89` -> `partner remains linked to victim through +$26` -> `Hawk waits 8 ticks` -> `Hawk checks partner through +$86 and sees state 5/raw $89` -> `Hawk enters $12C22` -> `collision profile $17` -> `victim type $12 / reciprocal +$92` -> `three-actor descriptor $12C0A` -> `victim +$26 resolves Animal` -> `three-actor hit/damage/landing sequence`.

The next engineering task is **not more ROM discovery for the missing trigger**. That bridge is now closed. The next task is to integrate this exact trigger/state sequence into the native Saturn runtime without replacing it with a simplified synthetic condition.

## Exact next tasks for the new chat

1. Materialize/open `/WWF-Saturn/WWF-Saturn-Test255-WIP16-source.zip` from Library.
2. Use the user's original `wwfwfest.zip` ROM set for any further byte-level proof. If it is not available in the new chat, ask the user to upload it again.
3. Implement the live Saturn LOD sequence using the exact recovered states/links/timing:
   - Animal hold raw `$89` state representation;
   - preserve victim/Animal `+$26` equivalent;
   - Hawk 8-tick AI wait;
   - partner `+$86` equivalent check for Animal state 5/raw `$89`;
   - enter Hawk `$12C22` equivalent;
   - establish Hawk/victim `+$92` equivalent;
   - load/use `LODTEAM.PAK` / exact three-actor compositions;
   - reproduce damage, victim state, third-actor state, launch and release timings from ROM evidence.
4. Add focused host regression tests for every transition in that chain before activating it in normal gameplay.
5. Then continue Hawk and Animal individual move-set closure and remaining full-roster interactions.
6. Persist the next validated source checkpoint in Library and update this handoff branch.
7. Do **not** generate a new BIN/CUE unless explicitly requested by the user. When requested, the assistant should handle GitHub Actions and deliver the artifact directly; the user should not need to run the workflow manually.

## Fidelity labels — mandatory

Always distinguish:
- **ROM-EXACT** — directly demonstrated from original ROM/program/gfx/audio evidence.
- **PROVISIONAL** — Saturn-native implementation/timing/semantics not yet proved exact.
- **REGISTERED BUT NOT IMPLEMENTED** — recovered/identified but not yet active in runtime.

Never promote a move identity, animation meaning, AI rule, timing, geometry, audio association or presentation detail from plausibility alone.

## Important caution

Several temporary LOD branches were created during the previous session (`test255-lod-wip16-src`, `test255-lod-runtime-wip17`, `test255-lod-wip16-work`, etc.). Ignore them as continuation targets unless explicitly needed for archaeology. **Use `test255-lod-handoff` + the Library WIP16 ZIP as the authoritative continuation point.**
