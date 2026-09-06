# New-chat handoff — WWF WrestleFest Saturn

Use this private repository plus `/WWF-Saturn/` in the persistent ChatGPT Library as the source of truth.

## Current canonical baseline

1. Read `PROJECT-STATE.md` first.
2. Current canonical validated source checkpoint:
   - **Test254 / V0.269**
   - Library: `/WWF-Saturn/WWF-Saturn-Test254-V0269-source.zip`
   - SHA256: `cb324b8c6493642854d1799597b8f4ee6b531041400b0d909a34608c17a78cb0`
3. **Do not resume from Test200, Test240, Test251, Test252 or Test253.** Test254 is the canonical baseline.
4. Test253/V0.268 is the direct gameplay predecessor and introduced ROM-exact grapple/ring-transition work; Test254 integrates the canonical audio subsystem without replacing that gameplay baseline.
5. Test255 / V0.270 is currently **WORK IN PROGRESS**, not yet a canonical checkpoint. Its work includes further ROM-exact animation/audio timing and attract-mode reverse engineering.
6. No Test255 BIN/CUE exists yet. The user requested a BIN/CUE only after Test255 attract-mode work is closed and validated.

## Important canonical facts after Test254

- Test252 closed the legacy Test212 fallback registry and added exact bank-15 object compositions.
- Test253 added ROM-exact ring exit/entry compositions, ring-out chains, the raw `0x29` grapple struggle, and exact partner-dependent struggle assets.
- Test254 integrated the canonical audio package while preserving Test253 gameplay. All `RECOVERED_ROM` audio command identities are preserved.
- `AUDADP.PAK` remains the complete canonical audio source; the Saturn transport only preloads proven gameplay-linked samples into SCSP RAM to avoid a large persistent HWRAM cost.
- Current Test255 WIP has additionally recovered the common animation phase engine (`+$24/+$25/+$22`), corrected exact timing for selected impacts, repaired C07/C09 to the uniform 223-frame roster-pack contract, and is reverse-engineering the original 14-state attract/demo sequence.

## Attract/demo target now mandatory

The original arcade attract sequence must be reproduced from ROM evidence, including:

- TAG demo with WWF/WrestleFest logos and explanatory text overlay;
- first Demolition/presenter segment with original animation and voice;
- TAG records screen;
- Royal Rumble demo with Royal overlay/text;
- second Demolition/presenter segment with the opposite Demolition speaker animation;
- Royal records screen;
- wrestler-specific demos with animated portrait + ROM text box, where the featured wrestler is actually part of that demo match;
- protagonist sequence recovered for states 2–13: Hogan, Warrior, Jake, Boss Man, Slaughter, Perfect, Earthquake, DiBiase, Hawk, Animal, Smash, Crush.

Do not use the old fixed Saturn attract cast or a fixed Hogan portrait.

## Fidelity rules

Always distinguish:

- **ROM-EXACT** — directly demonstrated from original ROM/program/gfx/audio evidence;
- **PROVISIONAL** — native Saturn implementation or timing/geometry not yet proven exact;
- **REGISTERED BUT NOT IMPLEMENTED** — recovered or identified but not yet active in runtime.

Never promote a move identity, animation meaning, AI rule, audio association, timing, geometry or presentation detail from plausibility alone.

## Build policy

After each canonical checkpoint, update this repository (`HANDOFF.md`, `PROJECT-STATE.md`, and relevant docs) immediately. The Library holds the canonical source snapshot ZIP. Generate BIN/CUE only when explicitly requested or when the current task explicitly requires it.
