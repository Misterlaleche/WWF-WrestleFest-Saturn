# Selection ownership audit 04

Read-only production audit. Research files only under `work/selection04`. No production edits.

## Exact evidence

Original main program: even `31e14-0.ic18`, odd `31e13-0.ic19`; interleaved SHA1 `a726c46f350723ea73158bc78055d17d418b14e2`. `selection_oracle.py` reads the external ZIP, verifies the SHA1 and executes original M68000 instructions using Unicorn 2.1.4. No production ROM copy is retained here.

Reproducible production oracle (from production/full_game, using a host with Unicorn2.1.4 and C compiler):

```
python tools/run_selection_oracle.py --rom /ruta/wwfwfest.zip --out /ruta/nueva/oracle
```

`evidence/platform04/oracle-results.json` and `oracle.log` record the production replay; the native function is also compared for all ten mappings:

- All 10 slot mappings executed from original `$5DA0` to `$5E82`.
- Nine confirmation fixtures executed from `$59D2`, including the real `$64E0` normalizer and `$5F28`/`$2AEA` when called. Stop before geometry/render at `$5CD0`, or the loop tail `$5D06`.
- 1,500 original palette writes: 75 confirmation counts for each of ten slots in each mode.
- Four complete original `$1086C` setup calls after `$5DA0` conversion and original `$D1C` preservation/clear. These use explicitly declared test seeds/register contexts. They prove behavior at those boundaries, **not** cold-boot provenance, a full selector playthrough, or production CPU selections.

## Slot mapping

These are words read at `$64CC + 2*slot`, independently verified by executing `$5DA0`. Names are the supplied native runtime's `wf_r18_fault_pages.h` identity labels, not inferred from portrait order.

| Selection slot | Raw identity | Native name | TAG palette bank | Royal palette bank |
|---|---:|---|---:|---:|
| 0 | 0 | Hogan | 0 | 0 |
| 1 | 7 | Perfect | 6 | 6 |
| 2 | 2 | Snake | 2 | 2 |
| 3 | 8 | Earthquake | 7 | 7 |
| 4 | 6 | Slaughter | 4 | 4 |
| 5 | 9 | DiBiase | 8 | 8 |
| 6 | 1 | Warrior | 11 | 11 |
| 7 | 3 | Bossman | 1 | 1 |
| 8 | 10 | Smash | 14 | 5 |
| 9 | 11 | Crush | 14 | 3 |

Raw 4/5 are Hawk/Animal, not the selection's slot8/9 pair. They are absent from this ten-slot conversion table.

Selection slot is actor WORD `+$22` (normally zero high byte, slot BYTE `+$23`). At exit `$5DA0`, each active actor gets mapped identity in WORD `+$56` and WORD `+$02`, and it is appended to history `$1C0598`. TAG then swaps actor0/1 identities/history if actor0 is active but lacks credited-player bit `+$A1.bit0`. Do not bypass this canonicalization when player 1 starts in the second actor.

`$64A4` cursor coordinates are five columns x two rows: X 384/480/576/672/768; Y 616 for slots0..4, 520 for5..9. Royal adds `$200` to actor Y in `$5CD0`. Slot9 has its own `(768,520)` position; TEST03's `(slot==9)?8:slot` is presentation-specific, not an identity or Royal geometry rule.

## Confirmation, partner, highlight

- The ROM confirms a fresh A/B input edge, not START. `$64E0` normalizes active-low physical port input. `+$A6` stores held buttons, `+$A2`/`+$A3` the new edge, `+$91` last direction and `+$A8`/`+$A9` the normalized direction. The actor's raw port pointer is `+$8A`.
- Reservation words begin `$1C11F4 + 2*slot`; confirmed-choice words begin `$1C1214 + 2*slot`. Confirmation rejects an already confirmed choice. These are different arrays.
- TAG ordinary choice confirms the current actor and, if its partner is inactive and uncredited, activates that partner through `$5F28`. It copies the same physical port and input-edge latches, then starts its selection at the same slot. A held confirmation therefore does not immediately accept the partner; release/repress is required. Another slot must be chosen because the first slot is already confirmed.
- TAG slot8 is special: it confirms both linked actors immediately, inherits the first actor's palette restoration word and port for an inactive partner, and assigns lower-address actor slot8 and higher-address actor slot9. No second partner choice is needed. Raw identities become10/11 only in the exit conversion. Royal slot8 and slot9 each confirm only the one selected actor. The direct TAG slot9 fixture is boundary coverage, not a claim it is reachable in normal TAG navigation: normal right-page TAG navigation does not enter9.
- Original paired pointers are actor0 `$1C05B0` <-> actor1 `$1C06BC`, actor2 `$1C07C8` <-> actor3 `$1C08D4`, stored at `+$86`.
- Palette owner `$5EAE` looks up bank at `$5F14` (TAG) or `$5F1E` (Royal), then writes WORD `bank*$80 + $18801E` (pen15). On confirmation counts1..74 it cycles ROM `$5F0A`: `0FF0,0F0F,00EF,0FFF,0F0F`. On count75 it restores actor WORD `+$AE` via `$5EEA`; count76 is complete and writes no palette. The flash phase counter is WORD `+$5C`, modulo5.
- Actor palette restore values come from `$6484` / cabinet-dependent `$6494`, not one universal green. Default four actors: `00F0,00FF,0E6F,0F00`; two-player TAG table: `00F0,00F0,00FF,00FF`.
- TEST03's permanent bright-green mask and freeze after the first confirmation do not implement this lifecycle. Its fixed bitmap palette needs an explicit original bank/pen-to-Saturn-index mapping to transport these writes correctly. Sharing a converted RGB value is insufficient to identify which source pixels change.

## State boundary needed for a real single-player match

A `(mode, slot)` or even four raw IDs does not define the original match. The existing native owner chain already expresses the appropriate boundary:

1. `wf_r18_frontend_driver` carries reset/start/mode/roster to original `$AC0`.
2. `wf_r18_match_driver_init` consumes that **same** frontend machine (shared raw RAM, live D0..D7 and A0..A6), and `wf_r18_match_flow` runs `$AC0 -> $D1C -> $BD2 -> $7B70 -> $C98 -> $A654 -> $C6C -> $8AE6 -> $F8C` for a paid route.
3. `$C98` includes `$1086C`, which owns CPU selection, actor flags, anchors, health and referee setup. `$A654` owns the original TAG/Royal prematch branch. `wf_r18_runtime_handoff_f8c` is the later typed-combat import boundary.

Preserve the full 16 KiB original RAM image and the live register/control context. The following dependencies explain why reducing the payload to IDs is unsafe:

- `$005C` RNG state, `$0080` frame/global clock, and full-width live D1..D7 are inputs to original `$21B4`; its RNG is not a standalone seeded PRNG.
- `$0066` cabinet/DIP configuration, paid route WORD `$007C`, replay WORD `$008C`, mode BYTE `$0161`, progress WORD `$0162` / BYTE `$0163`, and multiplayer flags BYTE `$0160` affect branches and initialization.
- Actor active, credited and partner-control flags, raw identities, physical port pointers and partner pointers distinguish the selected human from a selectable CPU teammate.
- History `$0598` and candidate list `$0380` belong to selection/progression. Royal's occupancy/group state includes `$16A8` onward. `$D1C` deliberately preserves selected actor fields while clearing `$05AC..$1CD3`; this order matters.
- Graphics/palette allocation owners affect RAM and live registers. Rendering callbacks cannot be successful no-ops merely because TEST03 has a bitmap already drawn; changing live registers can change later RNG output.
- Keep the existing IRQ/control context (SR/SSP/USP, generation, pending ACK) and one shared clock/input stream. Selector steps run within a frame until the actual wait boundary, not one native step per display field.

Do not invent a seed or set a paid/active actor bit as a replacement for the start owner. Original `$0978` consumes credit, clears the original actor range, sets active actor and physical input pointer, and marks paid state. `$57E0` expects the state resulting from GAME SELECT; it does not create a player from an empty RAM image. The supplied reset clear `$1F1E` starts at RAM `$0060`, so it does not itself establish the initial `$005C` RNG bytes. The cold RAM/reset provenance still needs an explicit original/platform owner; the fixture seed in this audit is not that owner.

Original `$1086C` execution examples (fixture only):

| Mode and selected slots | Active raw identity words after setup |
|---|---|
| TAG 0,1 | actor0 `0000`, actor1 `4007`, CPU4 `800A`, CPU5 `C00B` |
| TAG 8,9 | actor0 `000A`, actor1 `400B`, CPU4 `8008`, CPU5 `C009` |
| Royal 0 | actor0 `0000`, CPU4..8 `8004,8005,A009,A006,A00A` |
| Royal 9 | actor0 `200B`, CPU4..8 `8004,8001,8002,A008,A009` |

These demonstrate the high-byte role/team flags and that Royal supplies five CPUs for one human. Copying only low-byte IDs drops essential state. TAG also pairs CPU raw10/11 if either is selected, excludes raw4/5 from its ordinary pool, and uses raw4/5 at progress4/9, as implemented in `wf_r18_match_setup.c`.

## Achievable adapter and remaining owners

The next reviewable adapter should execute the existing **raw roster owner** while TEST03 remains a display transport: project the current actor slot/page/scroll, service input/VBlank, transport original palette changes, expose the selectable TAG partner, and return only the original `$AC0` state. Before entering it, obtain a proven `$57E0` snapshot from the real start/mode owner or execute that owner chain. No independent selection policy is required.

A complete prematch can then use the existing match driver. Its three extension hooks are not missing algorithm research:

- `$98BA` TAG name/card layout and `$8EB6` sound sequence exist in `wf_r18_match_support.c`; they need shared FG/palette buffers, ROM reads and correctly routed nested effects.
- `$285DA` card/world object update exists in `wf_r18_frontend_world.c`; it needs the same RAM/register/ROM context and submission of resulting world updates.
- Frontend driver already routes `$2AEA,$26E66,$1F9E,$2A06,$21E6,$26A42,$247C,$27B8,$2836,$51D2,$527E` and input/IRQ owners, but Saturn platform services must implement them faithfully. TEST03 alone does not bind that driver.
- TEST03 `roster_init()` selects one shared artwork set regardless of Royal and its cursor/confirmation helper aliases9->8. Royal's separate source world region and palette-bank identity need verified rendering support before Royal roster display can be claimed complete.
- The current `wf_prematch_begin(ids[4], seed, hook)` adapter expects already finalized identities and supplied RNG; it is not a substitute for `$1086C` and has no Royal six-participant state interface.

No new RNG, enemy selection, partner policy or synthetic bootstrap is justified. Missing integration is the persistent machine/control state plus Saturn render/CD/input/IRQ transport, original asset bank/pen identity, and connection of already present owners. This audit does not establish all assets, callbacks or native implementations as end-to-end verified.
