# RUNTIME18 bootstrap authority audit

Status: **native stage-zero one-player TAG direct-bootstrap implemented and ROM-verified through $F8C / $FA4**.

Canonical main68k SHA1: `a726c46f350723ea73158bc78055d17d418b14e2` (checked against `work/recovery/main68k.bin`). Read-only source: WP12 and WP29 inside `sources/zips finales.zip`. Extracted references and disassembly are under `work/runtime18/bootstrap`.

## Proven route

WP12 establishes `$AC0 -> $BD2 -> $C98 -> $A654 -> teardown -> $8AE6 -> $F8C`. Skipping visible prematch does not authorize skipping setup or its teardown.

`$C98` calls `$D1C` (save selected actor fields, clear actor area, restore), `$2988`, `$1FC0`, `$1F6C`, `$26E66`, `$1086C`, `$FFD2`, `$1004A`, `$2A06` and conditionally `$1F9E`. `$1086C` branches to TAG `$1034A` when `$1C0161.bit0` is clear.

TAG setup `$1034A` builds the eligible roster at `$1C0380` using history at `$1C0598`, excluding IDs4/5. `$103CA` selects an RNG descriptor from `$106E0` by progression; `$103E6/$103EE` call `$24CC`. `$1040A` resolves the selected roster entries. IDs10/11 are paired together; progression stages4/9 force IDs4/5. `$10454..$10474` writes CPU identities and activates physical slots4/5. `$104A4..$104FA` installs their partner pointers and flags. `$104FE..$106CA` builds primary/reserve ownership, positions, health and team flags; `$106CE` initializes referee via `$10718`.

Primary anchors at `$10708`: X/Y machine coordinates only. They do not constitute a bootstrap. `$10782` uses human identity health table `$10830`, CPU progression table `$10848`, cabinet difficulty adjustments `$1085C`, and previous-match preservation gates.

The real live entry `$8AE6` writes `$1C0072=0xFFFF`, calls `$1E92`, then `$782C/$7506` for active human primary slots, then `$C1EA`, and finally writes `$1C169E=0x8000` at `$8B30`. The R17 active-frame test fixture's `0x3000` is therefore not launch authority.

## Missing state and implementation obligations

* A genuinely reached player-selection result (actor identity, active flags, partner/input pointers and history), or a separately audited minimal direct-selection adapter.
* `$D1C` restore/clear boundary and all relevant setup owners above, including exact register side effects needed by gameplay RNG.
* CPU roster `$24CC` inputs and cold RNG state. WP29 explicitly records **NO WRITER** for cold `$1C005C`; choosing a seed must be labeled a port policy, never claimed to be arcade reset authority.
* Prematch common teardown `$A778..$A7B2`, then `$8AE6`, before `$F8C`.
* An oracle test deriving state from these real owners, rather than reusing the synthetic R17 active-frame fixture.

## Implemented owner

`wf_r18_tag_setup_1086c` in `wf_r18_bootstrap.c` implements the TAG branch `$1034A..$106DE`, `$24CC`, `$10782` and calls existing exact referee initializer `$10718`. It accepts supplied selected actors, history, replay, cabinet difficulty, progression, and real RNG/register context. Palette allocation/upload `$2AEA` is an explicit callback; it must preserve gameplay registers. This is a setup-owner API, not a complete direct-match launcher.

Independent Musashi execution of canonical ROM versus native C passed 40 cases: four RNG values (including zero and all ones) over all ten progression stages. Compared all nine actor records, referee, D0-D7, RNG result, history and eligible roster. The oracle intercepted only `$2AEA` (palette transport). Native compiled with `-std=c99 -Wall -Wextra -Werror`. Test entry: `tests/r17/bootstrap_setup_driver.c`, oracle `tools/verify_bootstrap_rom.py` (currently workspace paths).

The one-player path writes CPU slots4/5 from the actual ROM weighted selection, not fixed opponents. Its read of `$57(A0)` at `$104E6` intentionally retains A0=$1C0598 (therefore reads actor0+$3F); translating that instruction as a CPU actor read would be incorrect. Success at this API means this owner completed; `$C98`, teardown and `$8AE6` are still separate obligations.


## Direct launch completed

`wf_r18_direct_boot` accepts explicit human/partner identity, cabinet configuration, supplied RNG and initial D-register context. These are direct-launch choices; no claim is made that the ROM sets a cold RNG constant. Fresh zeroed game state replaces the menus; the selected fields are exactly those preserved by `$D1C`. Actor0 is the human, actor1 its reserve, linked to the same input source; CPU4/5 are chosen by the original weighted roster algorithm. The exposed direct launcher deliberately supports stage zero TAG only.

The native route reproduces `$C98` clear/video/register effects, `$1086C` setup, `$FFD2` fixed objects, `$1004A` fixed-four, `$A778` common teardown, `$8AE6` live finalization and `$F8C` through `$FA4`. Visible prematch is omitted as direct-launch policy. Audio commands are silent while retaining their register effects. ROM scene palettes `$2A06` are emitted as exact writes; parent video transport must support `$186000/$188000` layer banks. `$26E66` state is exact; its map-copy work is joined to the independently audited Saturn ring renderer by `map_26e66`. The output priority is `$7C`, from stage0 scene index0 and ROM `$26E9E`.

Full-route Musashi verification (`tools/verify_bootstrap_direct_rom.py`) executes the real `$C98`, including the real `$26E66` engine, then `$A778`, `$8AE6`, and `$F8C` to `$FA4`. Four supplied RNG states (0,1,13579BDF,FFFFFFFF) passed. It compares all wrestler records, referee, both fixed objects, four fixed objects, 20 camera/setup globals, D0-D7, RNG, roster/history, BCD credit fields and every FG0/palette write in order (address/value/width). Palette allocation/release and text transports are intercepted symmetrically; their independent implementation is owned by native video. Background VRAM map contents are verified separately by the ring-renderer oracle.

Additional setup and setup+finalize suites each cover all ten progression stages; the direct launch remains stage0. Credit157/remainder1 exercises decimal conversion and both additional text branches. Native full-route helpers compile with strict warnings. Large participant transaction buffers use static guarded storage, avoiding approximately49KiB stack frames.
