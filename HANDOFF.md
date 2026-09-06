# New-chat handoff — WWF WrestleFest Saturn

Use this private repository plus `/WWF-Saturn/` in the persistent ChatGPT Library as the source of truth.

## Branch to resume from

Resume only from **`test255-lod-handoff`**.
Do not resume from `main` or from any temporary LOD branch. Temporary branches are archaeology only.

## Canonical baseline vs current WIP

- Canonical validated baseline remains **Test254 / V0.269**.
- Canonical source: `/WWF-Saturn/WWF-Saturn-Test254-V0269-source.zip`
- Canonical SHA256: `cb324b8c6493642854d1799597b8f4ee6b531041400b0d909a34608c17a78cb0`
- **Test255 / V0.270 remains WIP until validated on real Saturn hardware.**

Latest full source checkpoint:
- **Test255 WIP18**
- Library: `/WWF-Saturn/WWF-Saturn-Test255-WIP18-source.zip`
- SHA256: `fc9191006caa757d056a1511c8a8bc486ed2c440abf7a1381273dc45f958caf8`
- WIP18 checkpoint commit: `f3a9f8709f5e063ca9461914c707390060a051d0`

Intermediate source checkpoint:
- WIP17 host-only Doomsday state machine
- Library: `/WWF-Saturn/WWF-Saturn-Test255-WIP17-source.zip`
- SHA256: `36b25b10cf7cadcf10e5ef7df7019d22ff28a74339974be24a8e9a8a06e5e306`

## Build policy

Do **not** generate BIN/CUE unless the user explicitly requests it. When requested, the assistant must prepare source/commits, run and fix GitHub Actions, download the artifact and deliver BIN/CUE directly. Do not ask the user to operate workflows manually.

The last generated Test255/V0.270 BIN/CUE remains the older GitHub Actions run `34048120742`; it predates WIP17/WIP18 and does not make Test255 canonical.

## Roster graphics mapping — preserve exactly

Logical Saturn wrestler -> arcade graphics bank:

- 0 Hogan -> 0
- 1 Warrior -> 1
- 2 Jake -> 2
- 3 Boss Man -> 3
- 4 Smash -> 10
- 5 Crush -> 11
- 6 Slaughter -> 6
- 7 Perfect -> 7
- 8 Earthquake -> 8
- 9 DiBiase -> 9
- 10 Hawk -> 4
- 11 Animal -> 5

Do not revert to the old incorrect assumption that logical 10/11 use graphics banks 10/11.

## Roster/assets already closed

- 12-wrestler resident runtime contract: 223 frames per `Cxx.PAK`.
- Hawk and Animal resident banks use corrected arcade banks 4/5.
- Smash/Crush use corrected arcade banks 10/11.
- `ROSTER.PAL` covers all 12 logical wrestlers.
- Hawk/Animal do not inherit Hogan's moveset fallback.
- Hawk/Animal recovered player special stamina path is 500.
- `Gxy` ground/PICK UP partner overlays cover all 12 logical wrestlers.
- `Sxy` grapple-struggle partner overlays cover all 12 logical wrestlers.
- logical IDs 10/11 use `A/B` in generated CD filenames where the hexadecimal filename scheme is used.

## Doomsday Device — ROM-EXACT trigger/state/link chain is CLOSED

Do not investigate the missing activation bridge again. It is already demonstrated.

### Animal hold

- logical Animal 11 -> internal actor 5.
- linked classifier at `$E1D4/$E232`.
- fresh K selects raw action `$89` in the recovered linked categories.
- descriptor `$1ABA4`, callback `$1ABC0`.
- poses `269 -> 452 -> 453 -> 454 -> 455`.
- finite visual duration 85 ticks.
- victim raw state `$FF`.
- Animal/victim reciprocal `+$26` survives the hold.
- attacker `+$34 bit3` is set; completion writes `+$24=2`, `+$22=0`, `+$60 bit7`; event/effect `$32` is produced.

### Hawk selection / launch

- wrestler state 8 sets `+$32 bit1`; state 8 completes to state 9 without clearing it.
- Hawk waits exactly 8 eligible ticks.
- Hawk reads partner through `+$86`.
- only partner state 5 / raw `$89` dispatches Hawk to state5/internal4 -> `$12BDA/$12C22`.
- `$12C22` consumes `+$32 bit1`, sets the recovered special launch path through `+$34 bit3`, and uses the recovered launch vector.

### Collision / three actors

- Hawk arms collision profile `$17` at the exact recovered threshold gate.
- first-handler index 11 -> `$247EC`.
- victim becomes type `$12`.
- `$24DEC/$24E02` establish reciprocal Hawk/victim `+$92`.
- descriptor `$12C0A` resolves the third actor through **victim `+$26`**, which yields Animal.
- victim receives state 4, selector `$19`, raw damage `$20`.
- Animal receives state 5 / raw `$8006`.
- exact composition clock uses poses 459/460 then 166/378; `LODTEAM.PAK` contains the three-role compositions for the ten ordinary opponents.

## WIP18 — live Saturn integration status

The above state/link chain is now active in the native TAG runtime and covered by focused host tests before gameplay activation.

**ROM-EXACT active pieces:** raw `$89` producer mapping for Animal K, reciprocal `+$26`, state8->state9 carried flag, exact 8-tick `+$86` predicate, `$12C22` flag lifecycle/vector, profile `$17` gate, type `$12`, reciprocal `+$92`, third actor strictly from victim `+$26`, victim/Animal raw writes, raw damage `$20`, `$12C0A` composition clock and `LODTEAM.PAK` role ownership.

**PROVISIONAL native boundaries:** native `PAIR_HOLD` -> exact linked-category adapter; Saturn world-coordinate/raw `+$0E` launch progression; Hawk composition anchor transform; minimum one-tick release bridge for callback-held pose 166 (`$FF00`); committed-sequence rescue/interruption policy; native apron restore after completion.

**REGISTERED BUT NOT IMPLEMENTED:** exact `$FF00` release producer; exact launch-to-collision world integration; exact Doomsday rescue/interruption producer.

Focused validation passed: Test203, 205, 206, 207, 211, 253 struggle assets, 259, 260, 267, 270, 271, 272, 273.

Historical Test210 (`tag_crossing_exchange`) also fails on a pristine extraction of authoritative WIP16 with the same assertion. Treat it as pre-existing baseline debt, not a WIP18 regression.

## Exact next tasks

1. Resume from the **WIP18 Library ZIP**, not WIP16 and not a temporary branch.
2. Continue the **individual Hawk moveset** to closure.
3. Continue the **individual Animal moveset** to closure.
4. Close Hawk/Animal interactions against each of the other ten wrestlers and then close the complete 12-wrestler roster.
5. Preserve the labels **ROM-EXACT**, **PROVISIONAL**, **REGISTERED BUT NOT IMPLEMENTED** for every recovered/implemented element.
6. Persist each important source checkpoint in both Library `/WWF-Saturn/` and this handoff branch.
7. No BIN/CUE unless explicitly requested.

If further byte-level ROM proof is required, the source checkpoint intentionally contains no arcade ROM. Ask the user to re-upload the original `wwfwfest.zip`; never infer missing ROM facts and never store/redistribute the ROM set in GitHub.
