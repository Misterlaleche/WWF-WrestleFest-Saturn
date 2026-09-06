# Test255 / V0.270 WIP — ROM-exact attract records recovery

## Correct records producer

The state-entry path at `$0B40` proves the observed arcade order directly:

`$AE20` Demolition/presenter interstitial -> `$264E2` transition -> `$69C8` records renderer -> `$21E6` hold `$0100` ticks -> continue new attract state.

`$69C8/$6A26` selects two different 11-row RAM tables using the Royal flag:

- TAG: `$1C00C2`;
- Royal: `$1C0104`.

The immutable default tables are stored at `$6DB0` (TAG) and `$6DF2` (Royal). `$6D82` copies `$42` bytes to each RAM table, proving 11 entries x 6 bytes. Each entry is recovered losslessly as:

`[metric_raw, initial0, initial1, initial2, wrestler_a, wrestler_b]`.

Default initials are `TAK`, `HAG`, `HAN`, `SIN`, `TAT`, `KIN`, `NAR`, `MAE`, `SAK`, `ISI`, `TOM`. The first byte remains named `metric_raw` until its exact user-facing semantic label is independently demonstrated.

## Exact renderer layout data

`$69C8` uses fixed 11-entry tilemap-offset tables:

- names at `$6B42`: `043C,083C,0C3C,0F3C,113C,133C,153C,173C,193C,1B3C,1D3C`;
- metric column at `$6B58`: `0570,0970,0D70,0F5C,115C,135C,155C,175C,195C,1B5C,1D5C`.

Royal shifts the metric-column base by eight bytes inside `$6A26`.

The recovered data are represented by `wip/test255/arcade_attract_records.c/.h` and byte-audited by `wip/test255/tools/test255_attract_records_rom.c`.

## False candidates explicitly closed

- `$B608` / gfx set `$4E` decodes as a TAG-championship presenter sequence (`The WWF Tag Championship: after a number of tough matches, we have a new...`), not records.
- `$BDA6` / gfx set `$4F` decodes as a Hogan/countdown-card presentation (4->3->2->1), not records.
- `$1B4E` is not the state-entry records renderer; it is a fixed wrestler/profile carousel followed by production credits.

## Fidelity status

**ROM-EXACT:** call order, TAG/Royal table selection, `$0100` hold, both immutable 11x6 default tables, RAM destinations, initials/IDs/raw metrics, and row tilemap offsets.

**REGISTERED BUT NOT IMPLEMENTED:** exact Saturn rendering of the `$69C8` tilemap/font surface and persistence/update semantics for user-created record entries.
