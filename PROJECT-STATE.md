# WWF WrestleFest Saturn — private development repository

Private working state for the native Sega Saturn port/reimplementation.

## Current validated checkpoint

- **Test254 / V0.269** is the canonical validated source checkpoint.
- Canonical source snapshot: `/WWF-Saturn/WWF-Saturn-Test254-V0269-source.zip`
- Source SHA256: `cb324b8c6493642854d1799597b8f4ee6b531041400b0d909a34608c17a78cb0`
- Test253/V0.268 remains the direct gameplay predecessor.
- Test254 integrates the canonical audio subsystem on top of Test253 gameplay; it does **not** replace the gameplay baseline with the older gameplay copy bundled in the audio package.
- Test255 / V0.270 is currently **WORK IN PROGRESS** and must not yet be treated as a canonical checkpoint.
- The user has requested a BIN/CUE after Test255 attract/demo work is finished and validated.

## Canonical lineage since Test200

Test200 → subsequent validated checkpoints → Test240 hardware image → Test251 → Test252 → Test253 → **Test254 canonical** → Test255 WIP.

Important known image state:

- Test240 is the last BIN/CUE known to boot correctly in the recent line.
- No Test255 BIN/CUE has been generated yet.

## Test251 / V0.266

- Eight original gfx ROMs verified and full 8 MiB `gfx2` reconstructed.
- All recoverable RISE poses resolved.
- PICK UP pose 395 resolved.
- 90 `Gxy.PAK` partner-dependent packs generated.
- 873 exact partner-dependent ground/PICK UP compositions recovered.
- Fxx finisher overlays and Gxy ground overlays separated.
- LWRAM worst measured combination remained below 1 MiB.

## Test252 / V0.267

### ROM-EXACT

- Program palette loader proved 16 banks at base `$2F22`, stride 32 bytes.
- Bank 15 source offset proved as `$3102`.
- Exact bank-15 palette and 100 object compositions for poses 114–123 recovered.
- `OBJ15.PAK` preserves original per-piece palette bank, including bank 0/15 mixtures.
- Legacy Test212 fallback registry reduced to empty because all registered families became exact-covered.

### PROVISIONAL Saturn infrastructure

- `OBJ15.PAK` kept as a persistent HWRAM pack rather than reusing the shared LWRAM overlay slot.
- CDFS file capacity increased to avoid directory truncation.

## Test253 / V0.268

### ROM-EXACT

- Ring exit action `0x69`, descriptor `$199FC`, poses 32–38.
- Front ring entry action `0x6C`, descriptor `$19C28`, poses 39–43.
- Ring-transition compositions preserve original bank-14 rope pieces and exact bank-14 palette.
- Victim ring-out chain recovered through state 4 / selector 14, poses 177→177→178→179, then actions `0x68→0x6A`.
- Attacker ring-out action recovered as raw `0x06`, descriptor `$13020`, poses 175→176→0.
- Ground roll/recovery sequences recovered from action `0x4B` using 110/111 or 112/113 then 104.
- Grapple struggle recovered as raw `0x29`, descriptor `$1656A`, partner-dependent poses 245/246/247/466 and two-cycle ownership flow.
- `RING14.PAK` generated for exact mixed wrestler/rope compositions.

### PROVISIONAL / pending

- Some Saturn spatial interpolation for crossing ropes and expelled-body trajectory remains native/provisional.
- CPU arbitration/cadence for simultaneous grapple inputs is not fully proved exact.

## Test254 / V0.269 — canonical audio integration

### RECOVERED_ROM / ROM-EXACT associations preserved

- Canonical sound command/event layer integrated from the supplied audio package.
- `SDRV.BIN`, complete `AUDADP.PAK`, recovered OKI decoder and music masters retained.
- Stage music table preserved.
- Low-stamina `<0x18` warning command `0x0B` and music restoration path preserved.
- Ring-out count 17/18/19/20 → `0x65/0x66/0x67/0x68` preserved.
- Proven long-impact/finisher command `0x32` associations retained only where supported by ROM evidence.

### PROVISIONAL Saturn transport

- The complete `AUDADP.PAK` remains the canonical source, but only proven gameplay-linked samples are preloaded/decoded into SCSP RAM.
- This avoids approximately 480 KiB of persistent HWRAM use from the original integration backend.
- Unproven gameplay→audio command associations remain registered but inactive.

## Test255 / V0.270 — WORK IN PROGRESS

Do **not** treat this section as validated/canonical until a Test255 checkpoint ZIP is produced and hashed.

### ROM-EXACT evidence already recovered in WIP

- Common arcade animation phase engine around `$1C0C8–$1C14E`:
  - `+$24` = frame/phase index word;
  - `+$25` = low byte of that index;
  - `+$22` = phase timer;
  - `$00FE` = finite-sequence completion marker;
  - `FF00` = frame held for callback-controlled release.
- Elbow Drop raw `0x24` impact/audio timing recovered at the exact ROM phase boundary rather than the old provisional tick.
- Additional finisher impact timing work for Jake/Slaughter/Perfect is being reconciled against callback phases.
- C07/C09 roster packs were found to have 221 frames while runtime requires 223; they were regenerated with two empty terminal frames only, preserving all existing graphics and preventing future `CHECK309`.

### Attract/demo reverse engineering — mandatory Test255 target

The current Saturn attract mode is explicitly provisional and must be replaced by the original arcade state machine.

Recovered so far:

- The arcade attract flow has **14 states**.
- State durations recovered from ROM:
  `848, 736, 1040, 928, 656, 928, 1200, 944, 1616, 1152, 800, 816, 832, 864` ticks.
- States 2–13 feature the wrestlers in this recovered order:
  **Hogan → Warrior → Jake → Boss Man → Slaughter → Perfect → Earthquake → DiBiase → Hawk → Animal → Smash → Crush**.
- Each featured wrestler must actually be present in that state's demo roster; fixed portrait/roster mismatches are forbidden.
- Wrestler portrait routine located around `$8CF8`; portrait animation timing and removal thresholds are being recovered.
- ROM biography/text renderer located around `$2503C`; wrestler data such as weight/from/birthday/trademark hold are recoverable directly rather than transcribed from screenshots.
- TAG demo overlay uses recovered text ID `$29`, graphics block `$15`, palette bank 7.
- Royal demo overlay uses recovered text ID `$2A`, graphics block `$16`, palette bank 6.
- Demo overlay/logos must be built from original ROM graphics/tile data, not redrawn from screenshots.
- Demolition/presenter segments use Smash/Crush IDs 4/5 and contain recovered audio/event commands, but exact per-speaker animation/event mapping is still being resolved.
- Records routine around `$1B4E` has been located; TAG vs Royal records variant details still need full closure.
- Hawk/Animal attract/demo artwork must be recovered rather than skipped.

### REGISTRADO PERO NO IMPLEMENTADO in Test255 WIP

- Exact mapping of the two distinct Demolition speaker sequences to their frame/event/audio commands.
- Fully separated TAG-records and Royal-records presentation data.
- Hawk/Animal attract/demo graphical packs.
- Complete integration of the 14-state attract machine into the Saturn runtime.
- Any presentation audio command whose producer/meaning is not yet independently demonstrated.

## Existing important gameplay issues to preserve/retest

- Rope-side fix: the actual contacted rope side is now retained; needs hardware retest.
- Presenter exit choreography is partly implemented, but the exact back-facing presenter sprite remains pending unless recovered during attract/presentation work.
- CHECK338 previously occurred during playable attract/demo; later overlay serialization and separate object/ring packs were designed to avoid reintroducing that collision.

## Fidelity labels — mandatory

- **ROM-EXACT**: directly demonstrated from original ROM/program/gfx/audio evidence.
- **PROVISIONAL**: native Saturn implementation or semantics not yet proved exact.
- **REGISTERED BUT NOT IMPLEMENTED**: recovered/identified but not yet active in runtime.

Never promote move identity, AI behavior, timing, geometry, presentation or audio meaning from plausibility alone.

## Persistence policy

After each canonical checkpoint:

1. Save the source ZIP to `/WWF-Saturn/` in ChatGPT Library.
2. Update `PROJECT-STATE.md` and `HANDOFF.md` in this private repository immediately.
3. Add/update focused documentation when a major subsystem changes.
4. Generate BIN/CUE only when explicitly requested or when the current task explicitly requires it.

Historical Test200 details remain recoverable from Git history and the Test200 checkpoint; this file now tracks the current canonical line rather than duplicating all historical evidence verbatim.
