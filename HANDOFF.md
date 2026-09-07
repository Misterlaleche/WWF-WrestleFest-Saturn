# New-chat handoff — WWF WrestleFest Saturn

Use this private repository plus `/WWF-Saturn/` in the persistent ChatGPT Library as the source of truth.

## Branch to resume from

Resume only from **`test255-lod-handoff`**.
Do not resume from `main` or from any temporary LOD branch. Temporary branches are archaeology only.

## Canonical baseline vs current WIP

- Canonical validated baseline remains **Test254 / V0.269**.
- Canonical source: `/WWF-Saturn/WWF-Saturn-Test254-V0269-source.zip`
- Canonical SHA256: `cb324b8c6493642854d1799597b8f4ee6b531041400b0d909a34608c17a78cb0`
- **Test255 / V0.270 remains WIP.**

Latest full source checkpoint:

- **Test255 WIP20**
- Library: `/WWF-Saturn/WWF-Saturn-Test255-WIP20-source.zip`
- SHA256: `5b09082b5d58707931ca8c7ca0a31ef4400485a2f6f75b547dc9626373e4bb81`
- Read `WIP20-CHECKPOINT.md` before editing runtime code.
- WIP20 is derived from WIP19, not from WIP9 or any HWFIX diagnostic payload.

Historical source checkpoints still useful for archaeology:

- WIP19: Hawk/Animal individual action registry work.
- WIP18: live Doomsday integration.
- WIP17: host-only Doomsday state machine.
- WIP16: byte-level trigger closure.

## Build policy

Do **not** generate BIN/CUE unless the user explicitly requests it. When requested, the assistant must prepare source/commits, run and fix GitHub Actions, download the artifact and deliver BIN/CUE directly. Do not ask the user to operate workflows manually.

The HWFIX1..7 BIN/CUE series was diagnostic hardware work. **HWFIX7 is the hardware-validated stability behaviour**, but it is not a replacement source lineage; WIP20 folds that behaviour back into the WIP19 development source.

## Hardware-stability result — CLOSED DIAGNOSIS

Real NTSC-U Saturn testing established the following sequence:

- CHECK309/339/342: failures around roster/object/ring full-pack loading and heap pressure.
- CHECK347/348: large contiguous arena allocations were not viable.
- HWFIX5 proved combat could load when resident `RING14.PAK` / `OBJ15.PAK` were removed.
- CHECK324 then exposed exact unmapped rendering states rather than memory failures.
- HWFIX6 added `CHAR / STATE / RAW` diagnostics.
- `CHAR 7 / STATE 27 / RAW 206` identified `ST_GROUND_PICKUP`, raw `$CE`, descriptor `$15B6C`.
- HWFIX7 added only bounded **PROVISIONAL** visual fallbacks for the affected state families.
- The user then validated HWFIX7 on real Saturn as behaving normally again in TAG, Royal and demo.

WIP20 therefore keeps full resident `RING14/OBJ15` disabled until the streaming replacement is hardware-validated.

## WFC2 streaming groundwork — WIP20

Test275 adds a no-allocation WFC2 metadata+block streaming decoder.

Measured WIP19 assets:

- `RING14.PAK`: 268319 bytes total; 53012 bytes metadata+directory; max stored block 3013 bytes.
- `OBJ15.PAK`: 105279 bytes total; 32320 bytes metadata+directory; max stored block 2741 bytes.

Host validation decodes every tile from both packs and compares it with the existing full-pack `roster_pack` decoder: tile-for-tile identical.

**REGISTERED BUT NOT IMPLEMENTED:** Saturn CD-sector callback and gameplay activation of this streaming reader.

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

Do not investigate the activation bridge again.

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

- Hawk arms collision profile `$17` at the recovered threshold gate.
- first-handler index 11 -> `$247EC`.
- victim becomes type `$12`.
- `$24DEC/$24E02` establish reciprocal Hawk/victim `+$92`.
- descriptor `$12C0A` resolves the third actor through **victim `+$26`**, yielding Animal.
- victim receives state 4, selector `$19`, raw damage `$20`.
- Animal receives state 5 / raw `$8006`.
- exact composition clock uses poses 459/460 then 166/378.
- `LODTEAM.PAK` contains the three-role compositions for the ten ordinary opponents.

## Doomsday live runtime status

**ROM-EXACT:** raw `$89` producer mapping for Animal K, reciprocal `+$26`, state8->state9 carried flag, exact 8-tick `+$86` predicate, `$12C22` flag lifecycle/vector, profile `$17` gate, type `$12`, reciprocal `+$92`, third actor strictly from victim `+$26`, victim/Animal raw writes, raw damage `$20`, `$12C0A` composition clock and `LODTEAM.PAK` role ownership.

**PROVISIONAL:** native `PAIR_HOLD` -> exact linked-category adapter; Saturn world-coordinate/raw `+$0E` launch progression; Hawk composition anchor transform; minimum one-tick release bridge for callback-held pose 166 (`$FF00`); committed-sequence rescue/interruption policy; native apron restore after completion; HWFIX7 visual fallback while RING14/OBJ15 streaming is not active.

**REGISTERED BUT NOT IMPLEMENTED:** exact `$FF00` release producer; exact launch-to-collision world integration; exact Doomsday rescue/interruption producer; Saturn runtime streaming for RING14/OBJ15.

## Hawk / Animal individual work

WIP19/Test274 fixed 126 exact action decisions (2 wrestlers × 21 categories × P/K/P+K) and preserves logical/internal mapping Hawk `10 -> 4`, Animal `11 -> 5`.

Exclusive move callbacks still needing ROM-backed semantic closure remain **REGISTERED BUT NOT IMPLEMENTED** where identity/lifecycle/damage has not yet been proven. Never name a move only from visual resemblance.

## Validation

WIP20 directed regression passed:

Test203, 205, 206, 207, 211, 253 struggle assets, 259, 260, 267, 270, 271, 272, 273, 274, 275, 276.

The complete host suite stops at ROM-dependent Test164 if `program.bin` is absent. Historical Test210 also fails on pristine WIP16 and is baseline debt, not a WIP20 regression.

## Exact next tasks

1. Resume from **WIP20**, not WIP18/WIP19 and never from WIP9/HWFIX payloads.
2. Implement and host-test Saturn-style sector-aligned/unaligned block reads for `stream_pack`.
3. Activate `OBJ15.PAK` streaming first while retaining HWFIX7 fallback as rollback.
4. Activate `RING14.PAK` streaming second.
5. Request hardware validation before removing any corresponding PROVISIONAL fallback.
6. Then continue Hawk individual moveset, Animal individual moveset, interactions against the other ten wrestlers and full roster closure.
7. Preserve **ROM-EXACT**, **PROVISIONAL**, **REGISTERED BUT NOT IMPLEMENTED** labels.
8. Persist every important checkpoint in GitHub and `/WWF-Saturn/` Library.
9. No BIN/CUE unless explicitly requested.

If further byte-level ROM proof is required and the ROM set is not available, ask the user to re-upload `wwfwfest.zip`; never store or redistribute the ROM set in GitHub or Library.