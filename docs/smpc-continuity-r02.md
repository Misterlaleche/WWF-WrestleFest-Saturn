# R02 controller continuity and VBlank publication

The first SMPC deadline change (`f09b5ad`) prevented early command writes but
could stop polling controllers indefinitely. In the normal-key red capture,
the emulated physical pad held C (`CDFF`) while eight SDK observations over two
seconds remained `FFFF`. Issued-command timing alone did not establish liveness.
That preliminary R02 image was withheld.

## Cause and bounded correction

The existing ISR decoded music before user callbacks and the SMPC query. A
read-only FRT probe measured title work before SMPC at median 1,226 µs, maximum
2,173 µs; music alone was median 913 µs. These are emulated timings including
probe overhead, not measurements from a physical console. The former cutoff
could reject every request. It could also leave CRAM publication after OUT.

With `WF_JO_ENGINE` only:

- Periodic INTBACK requests peripherals only (`IREG0=0`, `IREG1=8`). This removes
  the system-status interrupt/CONT round trip that had to finish before OUT.
- The existing FRT ≥300 µs minimum remains. A fresh beam sample must be at least
  six lines into blank and no later than raw VCNT `1FC`. The latter leaves two
  complete lines before OUT: at least 127.112 µs NTSC or 128 µs PAL after the
  sampled line ends. Final sample→COMREG has no wait, call, division, or CPU
  preemption in the accepted linked path.
- After gun latching, VDP1 erase, time and VDP2 work, the order is user callbacks
  (including synchronous CRAM/register publication), peripheral query, music.
  Video therefore precedes any remaining SMPC wait. Audio decoding and its
  budgets are unchanged.
- `jo_getdate()` refreshes system status on demand. It reserves the existing
  foreground command slot, copies OREG while masked, and holds a separate
  pending-status-IRQ flag until that IRQ is consumed. VBlank cannot mistake an
  old status interrupt for a new pad transaction. Published pads are retained.

Generic SDK builds retain their old protocol and ISR order. The actual game
uses `jo_getdate` in foreground with interrupts enabled. This change does not
promise progress for a caller masking interrupts while an earlier peripheral
transaction is still busy. Existing inherited-mode, FRT, external-latch and
diagnostic contracts remain as described in the original deadline document.

The protocol/window change alone, candidate A, was rejected: normal keys entered
Royal, but queries still had gaps up to 28 fields. Candidate B adds the bounded
music reordering above. Neither candidate changes gameplay or audio assets.

## Reproducible host gates

```
python3 tests/test_sdk_smpc_continuity.py --output build/smpc-continuity --mutations
python3 tests/test_sdk_smpc_timing.py --output build/smpc-timing --mutations
python3 tests/test_sdk_smpc.py --output build/smpc-generic
```

The continuity fixture uses the actual core ISR, complete SMPC source, input
publication and date function. MMIO, time and unrelated callback work are
substituted. It checks 216 consecutive field transactions with press/hold/release,
RTC changes, busy pads, caller-mask preservation, and status IRQ deferred behind
VBlank. Measured callback costs make old publication order fail and B pass.
The timing fixture passes 11,274 cases across PAL/NTSC, clocks/divisors, wrap,
display on/off, startup/inherited modes, stale transfers and invalid modes.
Eight semantic mutants are rejected, including missing status-IRQ exclusion,
status-plus-pad polling, absent RTC refresh, old music order and unsafe `1FE`.
The generic non-Wrestlefest suite also passes.

## Exact target evidence

Candidate B is frozen in `/work/r02-smpc-continuity-order`:

- ELF SHA-256: `47e596cbefbd4dc25434d7442364ddbbdd7da31cf159af9a449bdf4145dc24e9`
- RAW SHA-256: `8466af77d81a99a5575bef42ff7c085af16005b7f24ebe70835360941fd2a8dc`
- BIN SHA-256: `850524ebd8d11467dfcf53c4e734c1dde5af37a299fa66bd694b95f2e077568d`
- `_end=060DB464`; `__jo_work_end=060E9578`. No reservation or compiler-flag changes.
- All 34 original B12 assets verified; only five SDK sources differ from the
  frozen manual base. Full source hashes are retained in the evidence manifest.

Independent linked-assembly review verified callbacks finish before COMREG
`06011D90`, with music called afterward at `06011B50`. CRAM DMA is synchronous
and VDP2 present flushes immediately. Thus every observed successful runtime
request establishes publication finished before that COMREG and before OUT.

| Cold boot | Total requests | Runtime fields / requests | Min after IN | Min before OUT |
|---|---:|---:|---:|---:|
| NTSC, normal keys → Royal | 4,760 | 3,250 / 3,250 | 331.471 µs | 478.416 µs |
| PAL, natural attract → title | 17,026 | 15,763 / 15,763 | 392.993 µs | 3,254.820 µs |
| PAL, normal keys → TAG | 5,463 | 4,200 / 4,200 | 392.993 µs | 3,561.930 µs |

Runtime starts at field 2203 NTSC and 1856 PAL, after startup records/BIOS reset
commands. Full traces retain initial gaps of two/four fields; these are not
silently removed from overall results. There are no gaps in the listed runtime
intervals. Every transaction with a captured following IN ended SF=0, SR=C0,
Executing=-1 before that IN. The final request can be truncated by capture exit.
Normal keys published C, Start, A/B, left/right and releases and entered actual
combat (700 Royal ticks and 651 TAG ticks). Timing/null faults and audio fault,
PCM/music underrun, music error and unsupported-fade counters were zero. Captured
WAVs decode completely. Screenshots were inspected for title/arena presentation;
this is not a pixel-oracle comparison or a performance acceptance claim.

The PAL title between demos is phase 6 with title PC `837E..8482`, not phase 2.
The first automation missed this valid return; its failed report is preserved
as a checker false negative. Video and trace show the visual title from
approximately 256.9 to 262.8 seconds. No state restoration or game RAM writes
were used. A separate cold boot exercised TAG keys.

Evidence is in `evidence/smpc-continuity-20261113`. For example:

```
python3 tests/analyze_smpc_continuity.py \
  evidence/smpc-continuity-20261113/pal-tag.csv.gz \
  --capture evidence/smpc-continuity-20261113/pal-tag-capture.json.gz \
  --runtime-start-field 1856 --output build/pal-tag-audit.json
```

The observer uses actual emulated VDP2 edge timestamps and is cold-boot-only;
its clock state is not serialized for savestate restoration. This evidence
does not establish physical Saturn acceptance. Full victory/credits/records
and final stock-emulator release checks remain separate gates owned by the
release workflow.

At integration, a separate stock-emulator test that waited for attract to
return and then pressed C/Start stopped progressing at frontend phase 3,
PC `52BE`, while VBlank continued. Its cause is under investigation in
`capture-stock-tag-attract`; it is not attributed to SMPC without evidence.
The image remains withheld. The SDK timing/input gates above do not override
that independent release failure.

Primary protocol sources: Sega SMPC [§2.2 INTBACK processing](https://www.infochunk.com/saturn/segahtml_en/hard/smpc/hon/p02_20.htm),
[§2.4 registers/status](https://www.infochunk.com/saturn/segahtml_en/hard/smpc/hon/p02_40.htm),
and [§3.1 peripheral acquisition](https://www.infochunk.com/saturn/segahtml_en/hard/smpc/hon/p03_10.htm).
