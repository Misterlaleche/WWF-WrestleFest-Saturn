# AUDIO GAMEPLAY DISPATCH AUDIT — 2026-09-11

## Objective

Make the hardware-proven Saturn audio backend callable from the game using the
original arcade command namespace, while refusing to invent event-to-command
bindings that are not demonstrated by ROM evidence.

## Decision 1 — preserve the arcade command byte

The gameplay side emits the original command byte. It does not call a semantic
Saturn-specific sound name such as `SFX_PUNCH` or `MUSIC_MATCH`.

Routing is therefore:

- `0x00` -> stop music;
- `0x01..0x12` -> demonstrated production music commands;
- `0x13..0x1E` -> registered, but no demonstrated main-CPU producer: reject/fail closed;
- `0x1F` -> special fade lifecycle callback;
- `0x20..0x81` -> SFX/voice command backend;
- everything else -> invalid/reject.

Implemented by `wf24_command_bus.*`.

## Decision 2 — first gameplay bindings admitted

Only mappings already established by the audited audio integration package were
promoted into reusable code.

### Stage music

ROM table `0x000DEE..0x000E00`:

`04,0A,0C,0F,06,04,0A,0C,0F,06`

Invalid stage indices return `0x00`; they are not wrapped modulo 10.

### Low stamina

Recovered binding:

active actor stamina below `0x18` -> command `0x0B`.

The binding helper is stateless. The gameplay owner must preserve the recovered
edge/latch behavior rather than emitting `0x0B` every frame.

### Final countout numbers

Only displayed counts 17..20 are admitted:

- 17 -> `0x65`
- 18 -> `0x66`
- 19 -> `0x67`
- 20 -> `0x68`

Other counts produce no command.

### Heavy / finisher layer

`0x32` is exposed as the recovered long heavy/finisher reaction layer.

`0x2B` is also exposed because ROM evidence demonstrates an exact
`0x2B -> 0x32` pair on specific callbacks including Hogan Legdrop and Smash
Suplex. It is **not** automatically paired globally: many `0x32` call sites do
not have a preceding `0x2B`.

## Decision 3 — deliberately NOT bound yet

The following are not globally mapped yet:

- generic punch/kick impact;
- arbitrary falls/rope contact;
- referee phrases;
- presenter phrases selected by semantic English label;
- victory/game-over/frontend transitions beyond proven command producers;
- `0x1F` timing/fade curve.

Reason: the command table proves playback identity, priority, bank and sample
sequence, but that alone does not prove a unique gameplay producer for each
semantic event. These must be attached at their recovered ROM call sites or at a
CLEAN-R01 equivalent whose state/edge is proven to correspond to that call site.

## Host audit

The command-bus test covers:

- STOP routing;
- MUSIC routing;
- `0x1F` routing;
- SFX routing;
- rejection of `0x13`;
- rejection of `0x82`;
- all ten stage-music entries at the boundaries;
- low-stamina threshold boundary `0x17/0x18`;
- countout 17..20 plus rejection of 16/21;
- constants `0x32` and `0x2B`.

Local host compile/test result: PASS with `-Wall -Wextra -Werror`.

## Integration boundary

This repository still does not contain the current canonical gameplay runtime.
Therefore this commit establishes the callable audio command bus and audited
ROM-backed binding layer, but does **not** pretend to patch absent gameplay call
sites.

When the current gameplay source is imported/reconciled, the correct next step
is to replace each proven arcade sound-call site with:

`proven gameplay edge/state -> original command byte -> wf24_command_dispatch()`

The WP24 backend remains the single Saturn transport owner.
