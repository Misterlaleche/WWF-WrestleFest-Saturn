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
5. Test255 / V0.270 is currently **WORK IN PROGRESS**, not yet canonical.
6. Latest persisted Test255 work is **WIP checkpoint A**:
   - GitHub: `TEST255-WIP-A.md`
   - Library: `/WWF-Saturn/WWF-Saturn-Test255-V0270-WIP-A-source.zip`
   - SHA256: `0e8b5151a246c3f874342c4406046c25afd882c52cfb5455eed94493086ac127`
7. No Test255 BIN/CUE exists. Generate BIN/CUE only when the user explicitly requests it.

## Important canonical facts after Test254

- Test252 closed the legacy Test212 fallback registry and added exact bank-15 object compositions.
- Test253 added ROM-exact ring exit/entry compositions, ring-out chains, the raw `0x29` grapple struggle, and exact partner-dependent struggle assets.
- Test254 integrated the canonical audio package while preserving Test253 gameplay. All `RECOVERED_ROM` audio command identities are preserved.
- `AUDADP.PAK` remains the complete canonical audio source; the Saturn transport only preloads proven gameplay-linked samples into SCSP RAM to avoid a large persistent HWRAM cost.

## Test255 WIP-A facts

- Exact 14-state duration table, state rosters, Royal six-man cast and state→featured-wrestler mapping are now recovered from ROM and integrated into the native attract clock.
- The old fixed Saturn attract cast is removed from the WIP path.
- States 2–13 use the exact protagonist order: Hogan, Warrior, Jake, Boss Man, Slaughter, Perfect, Earthquake, DiBiase, Hawk, Animal, Smash, Crush.
- Exact `$8CF8` TAG/Royal overlay constants and profile-object timing/anchor thresholds are pinned.
- Hawk/Animal palette banks 10/11 and sparse exact `C10.PAK`/`C11.PAK` attract graphics have been recovered; `ROSTER_COUNT` is now 12 in WIP-A.
- Extended-roster attract AI is deliberately provisional/fenced until Hawk/Animal gameplay registries are expanded beyond the historical 10-entry tables.

## Attract/demo target still mandatory before Test255 can become canonical

The original arcade attract sequence must be reproduced from ROM evidence, including:

- TAG demo with WWF/WrestleFest logos and explanatory text overlay;
- first Demolition/presenter segment with original animation and voice;
- TAG records screen;
- Royal Rumble demo with Royal overlay/text;
- second Demolition/presenter segment with the opposite Demolition speaker animation;
- Royal records screen;
- wrestler-specific demos with animated portrait + ROM text box, where the featured wrestler is actually part of that demo match.

Still pending after WIP-A: exact Demolition speaker/event/audio mapping, fully separated TAG/Royal records presentation, exact portrait pixel/tile renderer, exact TAG/Royal overlay tile renderer, and full 12-character gameplay registries for Hawk/Animal.

## Fidelity rules

Always distinguish:

- **ROM-EXACT** — directly demonstrated from original ROM/program/gfx/audio evidence;
- **PROVISIONAL** — native Saturn implementation or timing/geometry not yet proven exact;
- **REGISTERED BUT NOT IMPLEMENTED** — recovered or identified but not yet active in runtime.

Never promote a move identity, animation meaning, AI rule, audio association, timing, geometry or presentation detail from plausibility alone.

## Build policy

Persist meaningful WIP checkpoints when needed, but do not promote Test255 to canonical until its attract/demo target is closed and validated. After each canonical checkpoint, update this repository (`HANDOFF.md`, `PROJECT-STATE.md`, and relevant docs) immediately. Generate BIN/CUE only when explicitly requested.
