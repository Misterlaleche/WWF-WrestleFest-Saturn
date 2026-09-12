# BLOCK-GAMEPLAY-MASTER-01 — RUNTIME-17 OWNER CLOSURE AUDIT ACCEPTANCE

External audit:
`RUNTIME17-OWNER-CLOSURE-AUDIT.zip`

ZIP SHA256:
`996dd714ff3c7307ea6ed2960e36a98b0ffbecc03121c333aeb0da934cf0c447`

Base implementation remains:
`BLOCK-GAMEPLAY-MASTER-01-RUNTIME-16`

Gameplay branch remains:
`gameplay-runtime-r01@1853f5564b8a00ddd5883a68169993305a40cfae`

This package is **research/ownership closure only**. It contains no deployable source, no branch/HEAD and no replacement runtime. It must not be promoted as a source-level RUNTIME-17 implementation.

## Independent master audit

Canonical main 68000:
- SHA1 `a726c46f350723ea73158bc78055d17d418b14e2`
- SHA256 `265ac89deeb7e708ac6fda724356bfd6af1c9012928a4e551e6e2faa424c2306`

Re-run directly against `wwfwfest.zip`:
- WP28 FINAL main-loop verifier: PASS
- WP09 FINAL referee verifier: PASS
- WP02 FINAL HOLD/descriptor verifier: PASS
- WP22 FINAL roster/move verifier: PASS
- independent R17 anchor audit: PASS

The ZIP contains three duplicated upper/lower-case audit logs; each duplicate pair is byte-identical. No SHA256 manifest is shipped, so integrity is anchored by the independently computed top-level ZIP SHA256 above.

## $01F914 fixed referee owner — CLOSED RESEARCH

ROM proves:
- fixed actor: `$1C11F4`
- primary state dispatch: 11-entry table `$1F952`
- continuation: `$1FAF0`
- render-coordinate preparation: `$247C`
- object queue insertion: `$27B8`
- Royal-specific conditional route: `$F8E4` when `$1C0161.bit1` is set

Exact callsites:
- `$1F92E -> $1FAF0`
- `$1F934 -> $247C`
- `$1F93A -> $27B8`
- `$1F94A -> $F8E4`

The referee is not one of the nine normal wrestler slots.

## $00F4C2 wrestler callback/animation/queue pass — CLOSED RESEARCH

Accepted actor order is ROM-exact:

1. reject when `+$32.bit7` set;
2. latch `+$1E=+$1C`, `+$1C=+$20`, set `+$20.bit7` when the latch is not already active;
3. `$F518`: clear `+$36`, optional `$280DC`, apply `+$38/+$3A/+$3C`;
4. `$1C03E` action/state callback dispatcher;
5. inside `$1C03E`, `$1C0C8` advances the animation descriptor/timer;
6. `$247C` render-coordinate preparation;
7. `$27B8` object queue insertion;
8. `$2208` post-queue actor helper;
9. after the wrestler scan, `$24090` invalidates stale profile/cache state.

Important atomicity rule:
`$1C0C8` is called **inside** `$1C03E` at ROM callsite `$1C0AC`. Do not split/reorder it as a separate top-level WP28 phase.

## $01C03E raw action dispatcher — CLOSED RESEARCH

For low latched state `+$1C == 5`:
- action table = `$12614`
- index = low `+$60`

For state 4:
- table = `$1AFD4`
- index = low `+$64`

Otherwise:
- table = `$11478`
- index = low `+$1C`

This is distinct from the WP01 CPU State5 executor at `$1DFE2/$1E018`. They are separate scheduler consumers and must not be collapsed.

## Raw $23 / Earthquake classification

WP22 confirms:
- actor/internal Earthquake = `$08`
- category `$07`, button-1/P path -> raw `$23`
- raw `$23` = **STOMP**
- Earthquake Vertical Splash raw selector remains unproven
- the old Earthquake raw `$47` semantic attribution is rejected by WP22

Raw `$23` action mapping:
- descriptor `$15CE0`
- callback `$15D08`

ROM anchors independently confirmed:
- shake writes `$15E14/$15EF0 -> $1C1800=$000E`
- `$14710` also writes the same shake value but is outside raw `$23`
- victim producer `$15F70 -> state4 word $0007`
- pending value `$15F7C -> +$68=$0014`
- victim producer `$1601A -> state4 word $0009`

Therefore State5 support alone does **not** make raw `$23` execute end-to-end; the real `$F4C2/$1C03E/$1C0C8` action pass is mandatory.

## Renderer/queue boundary

`$247C` computes actor display coordinates from actor world X/Y/Z, camera globals, per-actor corrections and facing.

`$27B8` performs validity/screen gating, selects one of six queue pairs from table `$2806` by actor `+$12`, appends the actor pointer and increments that queue count.

This is an explicit renderer integration contract. A no-op binding is forbidden.

## Why implementation remains blocked

Current RUNTIME-16 source has:
- exactly nine wrestler actor records in `wf_runtime_t`
- no explicit fixed referee actor owner
- no binding for `$01F914`
- no binding for `$00F4C2`
- no complete `$1C03E` raw-action dispatch layer
- no explicit `$247C/$27B8` presentation-queue contract

Remaining raw `$23` helper dependencies include:
- `$1110E`
- `$11412`
- `$11152`
- `$26AE`
- shared phases `$175FC/$176C4/$1770C`

Unknown reachability must remain fail-closed. `+$44=$09..$0F` raw23 reachability remains unproven by current FINAL evidence.

## Verdict

- `$01F914` ownership research: **ROM_EXACT_CLOSED**
- `$F4C2/$1C03E/$1C0C8` ownership/order research: **ROM_EXACT_CLOSED**
- raw `$23` mechanics: **CLOSED_CLASSIFIED**
- Saturn source integration: **PARTIAL / BLOCKED_BY_EXPLICIT_CONTRACTS**
- current deployable Gameplay checkpoint: **RUNTIME-16**
- full normal frame: **NOT COMPLETE**
- hardware gameplay validation: **NOT TESTED**

No Gameplay source was modified by this master acceptance.
No BIN/CUE generated.
