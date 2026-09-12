# WP12 R02 — Technical report

## Scope

Exclusive owner: paid `ENTRANCES/PREMATCH` from the post-roster live route through
the proven gameplay-active boundary `$00000F8C`.

No gameplay rules, TAG gameplay, Royal in-match spawning, pin logic, CPU, collision,
damage, shared audio backend, or master `main.c` is implemented here.

## R01 corrections implemented

R02 implements every structural blocker from audit R3 that belongs inside this
module or its explicit contract:

- one physical identity byte: low byte of actor word `+$56`, i.e. physical `+$57`;
- activation command: `+$57 < 5 -> $3032`, `+$57 >= 5 -> $3031`;
- `$A814` ownership: wrestler receives only local `+$1D=1`; controller owns `+$22`
  and `+$04` there;
- special ID `$0A` activates the next `$10C` wrestler record;
- `$1C0168` voice-$73 latch and `+4` controller-state skip;
- exact `$AD6C/$AD9C` local wrestler programs, using raw threshold `N` as dwell
  `N+1` callbacks and terminal `CLR.W +$1C`;
- state `$15` presenter selector cycle `8,9,10,11,8...` rather than clamping;
- common selector-word post-processing (`|=$8000`, or `$FFFF` for nonzero
  `$1C0163`);
- nine wrestler records plus a separate referee actor;
- Royal setup validation for `+$2E/+$04/X/Y/Z/+$32/+$34/+$20/+$B6`;
- Royal actor traversal continues after one actor reaches its terminal threshold;
- Royal state-0 `+$04` rewrite from `+$33.bit7`;
- skip input is evaluated after callback/render/sync;
- TAG-only `$98BA` is preserved as an exact external producer;
- pre-`$A654` `$BD2` chain and optional `$AE20(D0=0) -> $264E2` interstitial;
- `$C08..$C60` TAG/cabinet block including `$1C0160.bit6` and four `+$104=$8000`
  writes;
- continuous `$A778` teardown without artificial VBlank-separated audio events;
- post-`$A654` `$2A06`, `$DEE` audio, `$8AE6`, then `$F8C` handoff;
- renderer contract covers presenter/referee/nine actor records without generating
  camera/raster coordinates itself.

## Additional direct-ROM correction found during R02 self-audit

At the one-time entry to `$A654`:

```text
CMPI.B #0,$1C0163
BNE     skip
ADDI.W  #$0080,$1C11FA
```

Therefore, when `$1C0163==0`, referee X (`$1C11F4 + $06`) receives `+$0080`
**once before** TAG `$98BA` / `$3132`.  R02 models this exactly and has a regression
test for both zero and nonzero `$1C0163`.

## TAG flow

`$BD2` entry -> `$8EB6` (`$3100,$3120`) -> optional `$AE20/$264E2` -> `$7B70`
-> exact `$C08..$C60` inline gate -> external `$C98` -> validated setup -> `$A654`.

At `$A654` entry TAG executes `$98BA` then `$3132`.  The TAG controller uses
states `$00..$17`; state `$18` is terminal.  Identity/activation participants use
primary/fallback records exactly as `$ABD8` demonstrates.  `AAEA` consumes shared
RNG `$21B4`; R02 requires that RNG externally and never substitutes its own PRNG.

The presenter type is numeric set/type `$001F`.  WP25 closes numeric selector to
composition for selectors `4..7`, `8..11`, `13..16`; human pose names remain OPEN.

## Royal flow

Royal skips `$98BA` and the TAG controller.  All nine active wrestler records are
still traversed by `$A654`.  State-0 actors increment `+$22`; nonterminal calls
rewrite `+$04` from `+$33.bit7` and do not move X/Y.  At threshold `$40`, an actor
writes controller terminal state `$18` and emits `$312F`, but the nine-record loop
continues for remaining records in that same iteration.

The six canonical opening records are validated against the `$10ACC/$10AE4` tables.
R02 does not fabricate a separate Royal timer; initial counter state remains an
upstream common-setup responsibility.

## Referee prematch

R02 requires the `$10718` setup contract:

- active;
- type `$000C`;
- X `$0280`, Y `$0198`, Z `$0140`;
- primary `+$20=$8000`;
- `+$34=0`;
- global `$1C1697=1`.

The referee is rendered every `$A654` iteration.  Gameplay referee state machines
remain outside WP12.

## Exit and handoff

On action-button skip or terminal state, the same ordered batch contains the
`$A778` teardown:

`$2B58(D0=$1F) -> $9A42 -> $7506 -> CLR.W $1C14CE -> $1C15FC=1 -> $2A06 ->
$3100 -> $3120 -> $312F`.

After `$A654` returns, the live path executes another `$1C15FC=1/$2A06`, sends the
raw command from `$DEE[low $1C0162]`, then `$8AE6`, then reaches `$F8C`.

Gameplay execution after `$F8C` remains `BLOCKED_BY_GAMEPLAY_OWNER`.

## Renderer boundary

Each `WP12_OP_RENDER_*` entry represents the exact arcade render-call pair **`$247C -> $27B8`** as one integration operation. Its `rom_address` is `$247C`; `$27B8` remains the second call of the same pair and is not promoted to a separate frame or VBlank.

R02 exports exact machine-space actor/presenter/referee coordinates and selector
words.  WP26 explicitly classifies final camera/actor/raster values as
runtime-dependent producer data, so R02 does not invent camera offsets or screen
coordinates.
