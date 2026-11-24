# Physical timing acceptance

Matching a native wait count does not establish that a scene appears for the
same time. CPU work can consume Saturn fields while the native cursor stays
at the same tick. The acceptance reference is the field counter at
59.82612341538 fields/s, with 37609/39168 native ticks per field; host process
elapsed time is not a Saturn timing source.

## What current captures establish

`tools/analyze_capture.py` reports combat averages, actual sampled 120/600-tick
windows, pending/max debt, handoff debt, bank waits, audio issue counters and
terminal actor comparisons. Direct-combat comparisons require saved stage 21
and their exact fixture endpoints. Full-frontend builds use the separate WFT1
snapshot described below. Partial captures never establish an unobserved
terminal state.

For stage 20→21, `finish_field` marks combat completion. The last stage 20 sample
and first stage 21 sample bound result completion even while the main diagnostic
field value is frozen. No interpolated completion timestamp is used.

Frontend phase/PC diagnostics also expose pauses that a wait-count test cannot
see. In frontend 05 the observed PC 298C / tick 1705 remains unchanged for at least
48 physical fields; PC 298C / tick 1304 remains unchanged for at least 42. These are
unchanged diagnostic observations, not a cycle-level attribution to an
individual helper. The corresponding evidence records phase-entry and exit
bounds, rather than assigning exact times to transitions missed by the sampler.
See `evidence/frontend-05-timing-memory.json`.

## Full-frontend terminal capture

Profiled full-frontend builds reserve the first 4 KiB of LWRAM for WFT1. At the
actual post-ACK terminal boundary, before the next original owner can change
the runtime, the program copies the 2,412 actor bytes and publishes the header
magic last. The header records mode, owner, RNG, native tick, pending debt,
clock remainder and physical fields at combat start and completion.

The current deterministic automatic TAG route reaches tick 10,397 and owner
11B6; the automatic Royal route reaches tick 14,409 and owner 1AFC. Older TAG
captures at tick 9,666 belong to their earlier fixture and must retain its
matching reference. These are distinct from the direct
fixtures at ticks 8,537/8,474. Their references are reproduced by
`python tests/run_jo_main_oracle.py --limit 30000 --output evidence/oracle-local`,
which compiles the actual application `port/main.c` with host hardware shims.
The host field cadence is not a Saturn speed measurement. Manual inputs do
not acquire an automatic fixture's reference merely by selecting the same mode.

WFT1 separately records the field immediately before the post-combat owner,
the field at its return, and the frontend tick counts on either side. Its
completion flag is published only after that return. Thus the analyzer can
report an exact owner interval when complete, while retaining a valid terminal
actor snapshot if the owner is still running. The small copy/transport interval
between combat completion and owner entry is reported separately; it is not
silently removed from the overall route.

The capture runner observes the completion counter and saves LWRAM without
writing guest state. `tools/analyze_capture.py` validates the WFT1 version,
length, mode, endpoint, owner, RNG and reference hash. A valid actor comparison
does not prove that all intermediate images were displayed or that every scene
transition met its physical deadline. The first-visible/publication measurements
below remain necessary for full timing acceptance.

The later WFT2 observer records every actual SDK displayed-tag publication and
binds native ticks only after successful combat commits. The complete C13
Royal diagnostic captures all ticks 0 through 14,408 in order from event one,
with a final display field before terminal, no ring loss and no guest/reader
issues. Its original TIME UP wait and final clock stamps also reconcile with
the native fixture. This is stronger publication-continuity evidence, not an
independent framebuffer comparison or uninstrumented speed measurement. See
`evidence/royal-recovery/c13-continuity` for the bounded claim and independent
audit; its nonzero pending clock debt remains visible.

## Measurements needed for each transition

| Boundary or measurement | Observable value | What it resolves |
| --- | --- | --- |
| Original owner entry/exit | Native tick, virtual level, clock remainder, physical field, owner PC and generation | Separates the original wait budget from elapsed transport work; preserves HIGH/LOW entry differences |
| Completed scene publication | Physical field plus the unique submitted tag | Locates when complete maps, palette, scroll and sprite records become eligible for display |
| First visible scene | Displayed tag/field and captured pixels | Includes queue, VDP1 and VBlank latency; an owner return alone does not prove visibility |
| Longest unchanged image | Consecutive physical fields with the same displayed state, correlated with native tick progress | Detects pauses even when total wait counts agree |
| Boundary debt | Pending ticks before entry, at exit and after reanchoring | Prevents a clock reset from hiding an accumulated delay |

A small bounded diagnostic event buffer can record owner and publication
boundaries without changing game RAM, input, waits or execution order. Each
event needs its native tick, physical field, PC, generation, tag and event kind;
the clock remainder/virtual level must also be available at the boundary.
The asynchronous capture can then read the recorded event values instead of
guessing the moment from a later status sample. Buffer overflow must be
reported, not treated as continuous coverage.

With a known starting remainder, the rational clock gives the exact field
budget for the recorded native tick interval. Without it, report the possible
field range. Compare first-visible boundaries to the matching arcade route
under the same input sequence; also report owner-to-publication and
publication-to-display latency separately. A screenshot first observed at
field N establishes an upper bound unless the publication tag proves the
precise first displayed field.

The existing host fixtures prove the original logical waits and publication
ordering. They cannot prove hardware speed, CD streaming behavior or physical
scene durations. Current capture averages and actor endpoint equality remain
separate pieces of evidence; none grants whole-game acceptance.
