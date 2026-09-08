# WIP170 CHECKPOINT — port services + exact announcer terminal runtime

## Scope
Advance WIP169 additive integration from isolated modules to reusable port services and a live-ready exact announcer exit runtime, while keeping the newer WIP160+ gameplay core untouched. No BIN/CUE generated.

## Port services
Added `wf_port_services.c/.h`:
- boot loads Records from Saturn internal Backup RAM;
- missing/corrupt/unavailable BUP falls back to exact arcade defaults without blocking gameplay;
- qualifying TAG/Royal records are inserted with the exact ranking rule and immediately attempt persistence;
- in-session ranking remains valid even if save fails;
- clear-records restores arcade defaults and attempts save;
- Credits lifecycle is owned by the service and exposed as VBlank/event API;
- frontend return event deactivates the Credits service cleanly.

## Announcer exit runtime
Added `wf_announcer_runtime.c/.h` for the audited terminal TAG announcer sequence:

```text
state15: Y $0178 -> $01A7, +0.5/update, 94 updates, selectors 8/9/10/11, dwell12
state16: no movement, 48 updates, selectors 4/5/6/7, dwell12
state17: no movement, 48 updates, selectors 13/14/15/16, dwell12
state18: terminal
```

The runtime deliberately does not mutate a generic facing flag. The research proves states15/16/17 express the visible turn/exit through frame selectors; this prevents the known port error where the announcer walks out still facing the camera.

## Validation
Unified host suite with `-std=c11 -Wall -Wextra -Werror` PASS:
- Records/WFRK
- Credits state/timing
- attract biography compile
- A44 router/runtime
- A49 metadata
- A49->A44 exact transport adapter
- referee/announcer tables
- announcer exact exit runtime
- port services boot/records/credits
- Presentation contracts compile

## Package
`WWF-Saturn-Test255-WIP170-integration-overlay.zip`
SHA256 `1cc8cb00b5e33871cd743432efce119b68047e9d6bd14d6f679996af95aee306`
Archive integrity PASS. No arcade ROM, `program.bin`, BIN/CUE or ISO included.

## Safety
- no WIP160+ gameplay/AI/animation file replaced;
- raw23 WIP168 release remains contract-only until canonical source is materialized;
- `tick==12` and held fallback remain unchanged;
- actor-dependent announcer phases remain actor-dependent rather than assigned fake fixed timings.

## Next
Implement the referee count action `$20022` as a parameterized ROM-exact phase runtime that preserves inherited `+$25` instead of inventing an absolute pin-start timeline. Then continue frontend wiring contracts while waiting for materializable WIP160+ source.
