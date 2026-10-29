# WP02 — independent paid TAG introduction choreography audit

## Verified result and deliverables

`wp02_scene.c`, `wp02_scene.h`, and `wp02_rom_tables.h` implement a **native C99/SH-2 compatible scene controller**, with no 68000 emulator in the Saturn runtime. All work here is isolated under `wp02/research_rom`; source archives and existing projects were not edited.

The reference test executes the supplied original MC68000 program from `$A654`, including `$98BA`, the original presenter/wrestler callbacks, original sprite composer, original RNG, and actual `$A78E` presenter clear. Only the external display acknowledgement is supplied by the harness. The MC68000's 24-bit physical ROM bus is mirrored when Unicorn preserves nonzero upper address bits. No choreography callback is mocked or skipped.

Independent MAME validation: capture `verification/mame/full/frames.bin`, SHA-256 `2d8cf4e2b09de5a9b740847044ea9445071cf6fae0ee615f4a2682f51e1e6b39`, arcade frames **3249–4818**, raw identities **[0,6,9,1]**, initial RNG **0xCEB7C58E**. Native state matches all **1,570 records**: **1,569 visible display updates plus terminal clear**. Every exposed presenter/referee field, all wrestler selectors, XYZ, local states, phases, counters, RNG state, and opening name visibility match. Player/CPU ownership bits are outside this isolated four-primary-slot scene initialization and are explicitly excluded from wrestler comparisons.

Reproduce:

```sh
python test_scene_against_rom.py /path/to/01-wwfwfest.zip --wide
python test_scene_against_mame.py /path/to/verification/mame/full/frames.bin
```

The ROM oracle uses research-only Unicorn 2.1.4. It is not linked into the Saturn code. Detailed results are in `native_differential_results.json` and `mame_differential_results.json`. The suite includes all twelve raw identities, valid Demolition paired teams, deliberately unusual grouped combinations, multiple seeds including zero and `0xFFFFFFFF`, and exact audio events.

## ROM identity and namespaces

The program is the even/odd interleave of `31e14-0.ic18` and `31e13-0.ic19`: 524,288 bytes; SHA-1 `a726c46f350723ea73158bc78055d17d418b14e2`; SHA-256 `265ac89deeb7e708ac6fda724356bfd6af1c9012928a4e551e6e2faa424c2306`.

**The scene API uses raw actor/internal IDs, which are also raw graphics-set IDs.** Do not apply the old Saturn UI permutation to these values.

| Raw ID / graphics set | Identity | TAG selection |
|---:|---|---|
| 0 | Hulk Hogan | Selectable |
| 1 | Ultimate Warrior | Selectable |
| 2 | Jake Roberts | Selectable |
| 3 | Big Boss Man | Selectable |
| 4 | Hawk | Hidden Legion of Doom opponent |
| 5 | Animal | Hidden Legion of Doom opponent |
| 6 | Sgt. Slaughter | Selectable |
| 7 | Mr. Perfect | Selectable |
| 8 | Earthquake | Selectable |
| 9 | Ted DiBiase | Selectable |
| 10 | Demolition Smash | Paired with 11 in TAG |
| 11 | Demolition Crush | Paired with 10 in TAG |

ROM `$64CC` is exactly ten BE WORDs: `[0,7,2,8,6,9,1,3,10,11]`. WP22 `tables/roster_namespaces.csv` independently closes all identity namespaces. `raw_roster_04_05_0a_0b.png` directly decodes the relevant original artwork and confirms the LOD/Demolition distinction.

WP20's TAG navigation at `$6314` isolates selector slot 9; confirming shared Demolition slot 8 constructs adjacent selector identities 8/9, which `$64CC` maps to raw 10/11. For random normal TAG teams, draw distinct singles from `[0,1,2,3,6,7,8,9]` and optionally include intact ordered Demolition `[10,11]` in logical slots `[0,1]` or `[2,3]`. Random selection of this demonstration's participants is a port policy; the ROM's original introduction choreography is preserved after selection.

## Starting state, fixed anchors, and camera

The four wrestlers are already inside the ring. No wrestler position changes in this pre-match loop. `$1057E/$10584` use the ROM table `$10708`:

| Logical participant | Primary record | Fallback if inactive | X | Y | Z/base |
|---:|---:|---:|---:|---:|---:|
| 0 | 0x1C05B0 | 0x1C09E0 | 0x240 | 0x150 | 0x140 |
| 1 | 0x1C06BC | 0x1C0AEC | 0x220 | 0x160 | 0x140 |
| 2 | 0x1C07C8 | 0x1C09E0 | 0x2C0 | 0x150 | 0x140 |
| 3 | 0x1C08D4 | 0x1C0AEC | 0x2E0 | 0x160 | 0x140 |

Presenter `$1C14CE`: type `0x1F`, status `0x8001`, X `0x280`, Y `0x178`, Z `0x140`, initial selector 2. Referee `$1C11F4`: type `0x0C`, selector 0, status `0x8000`, X `0x300`, Y `0x198`, Z `0x140`. Referee setup initially writes X `0x280`; `$A65E` adds `0x80` for stage byte `$1C0163==0`. The referee remains static; scene occlusion must not be interpreted as a new referee movement or removal state.

Actor projection `$247C`: `screenX = X − RAM16[0x1C17E6] + signed actor correction`; `screenY = Y + Z − RAM16[0x1C17EE] + signed actor correction`. For this scene those camera words are **0x1E0, 0x230**. Background hardware scroll Y **0x5D0** is the transformed value `2048−560`, not the actor-camera Y. Preserve the original graphics compositor/visible-area transform after these coordinates; do not substitute provisional pixel anchors.

Opening names are produced by `$98BA`. `$A9E2` clears their composition with `$9A42` on **update 129**, the same update that advances state 0 to state 1. The native API exposes `names_visible`. The separate graphics extraction owns the original font tiles and exact name-cell placements.

## Controller and exact exit

Callback table `$A7B4` has 24 entries, states `0x00–0x17`; `0x18` is terminal. The no-grouped-skip route is:

| State(s) | Display updates | Behavior |
|---|---:|---|
| 00 | 129 | Selector 2, presenter setup/opening names |
| 01 | 50 | Selector 2 for 17, then 3 for 33 |
| 02 | 161 | Speaking selectors 0/1; command 317B |
| 03 | 33 | Selector 2 |
| 04,08,0D,11 | Identity-dependent | Identity voice and speaking |
| 05,09,0E,12 | 64 | Wrestler activation; presenter selector 0x0C |
| 06,0F | 21 | Speaking; command 317C |
| 07,0A,0C,10 | 1 each | State handoff, prior selector retained |
| 0B | 41 | Speaking; command 317D |
| 13 | 33 | Selector 2 |
| 14 | 50 | Selector 2 for 17, then 3 for 33 |
| 15 | 94 | Presenter Y += 0.5/update, selectors 8–11, 12 each looping |
| 16 | 48 | No motion, selectors 4–7, 12 each |
| 17 | 48 | No motion, selectors 13–16, 12 each; then state 18 |

Identity-window thresholds at `$AC9E` are `[64,128,128,128,128,128,128,128,64,256,64,64]`; the displayed duration is **threshold+1**. Without grouped skips the total is `989 + W0 + W1 + W2 + W3`. This formula is **not universal**: identity command `3173` sets `$1C0168=0x8000`; its activation adds four before the ordinary state increment, skipping the partner's separate introduction. The native controller executes that original branch.

At `$A6A2`, the **resulting** presenter state is compared with 0x0B. From that point the raw selector gets bit `0x8000`, including the exact handoff frame and all exit selectors. State 15 reaches integer Y `0x1A7` on its 94th update. States 16/17 do not change XYZ.

`wp02_tick()` returns 1 for each original visible update. On the last one, state is 0x18 and selector 0x8010 is still active: retain that visible frame. The next call clears presenter status, sets `done`, returns 0, and does **not** advance `ticks`. This matches the independent MAME capture's final clear record. Gameplay, combat, input skip, post-intro HUD initialization, and common subsequent audio commands do not run in this independent scene.

## Wrestler poses and original RNG

Activation `$A85A` changes the selected wrestler's local state to 1. If raw ID is 10, `$A878–A886` also activates the next physical record, its Demolition partner. Each active wrestler then executes `$ACB6`: pointer table `$AD6C`, counts `$AD9C`, selector words and parallel threshold words. Phase transitions use `counter > threshold`, so a table value 6 yields **seven** visible updates, not six. The selector programs are extracted by `extract_wp02_tables.py`. Last selector persists on the update that clears local state; idle returns on the following update.

Idle raw selectors are 0x8000 on the left team and 0 on the right team. Active programs preserve their raw selectors, including their authored facing. No added taunt, walk, rope-entry, or translation is present.

Speaking writes selector 0/1 before decrementing the countdown. On negative countdown, `$21B4` supplies `6+(RNG&15)`, then the phase toggles. The original RNG wraps 32-bit additions of D1–D7 and `$1C0080`, then rotates right by the low three bits. Its register inputs matter. For this four-primary scene the original composer leaves D4=6, D5=D7=0; D6 comes from the final drawn slot-2 actor: Jake gesture selectors 89–91 leave 0, Slaughter idle leaves 0xFFFF, other covered states leave 0x1C1610. The native port preserves these outputs; `composer_rng_context.json` records the independently executed original-composer probe. No fabricated arcade cold-boot seed is claimed.

## Sound and corrections to older summaries

Entry emits raw 3132. Fixed commands are 317B/317C/317D/317C at states 02/06/0B/0F. Identity low bytes from `$AC4A` are `[6D,6E,6F,74,73,73,71,70,72,75,73,73]`. Activation emits **3032 for raw IDs below 5; 3031 otherwise**. `$2052` ORs the command with 3100 before the hardware write: activation therefore reaches the sound bus as 3132/3131. The API exposes both raw and effective commands, in execution order, with no invented text transcription.

The audit corrects several older FINAL/WIP prose claims: selector 0x0C is written to the **presenter A0**, not the activated wrestler A2; the activation sound comparison is `<5`, not the stated opposite branch; local selector durations are threshold+1; grouped announcements invalidate a universal four-window duration; old UI IDs 4/5 are not raw arcade 4/5; and fixed 900-tick or invented walk-in scenes are superseded. Raw selector names such as “turn” or “rope descent” remain descriptive labels, not extra animation instructions.
