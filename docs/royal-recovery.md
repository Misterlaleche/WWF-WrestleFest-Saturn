# Royal performance recovery — 2026-11-12

The authorized goal remains arcade cadence on NTSC Saturn with 4 MiB expansion,
the exact 37609/39168 game clock, every original tick/frame/actor/pixel and all
original waits. Jo Engine 2026.3, CD-R/MODE and the B12 fixes remain required.

## Executor loss

The execution environment disconnected and returned with this session's folder
empty. The original working repository, local commits, generated images and raw
captures are no longer available in the recovered filesystem. Do not describe
them as saved deliverables or attach their old paths as current files.

The delivered B12 source was recovered from its unchanged version3 archive:
SHA256 `6d052926aeff021bd720f8b6f2afc14f26182273a395373c2316ea5ba61371f2`.
The new branch is `royal-performance-recovery`, starting at `805483a`.
It is a new recovery history, not the former `royal-performance` branch.

## Measurements observed before the loss

These figures were read from completed tools in the conversation. Their raw
capture files are currently missing. They guide reconstruction; they are not
fresh acceptance evidence for the restored source tree.

| Terminal-only capture | Combat ticks | Saturn fields | Seconds |
| --- | ---: | ---: | ---: |
| Exact B12 | 14409 | 17856 | 298.464934 |
| C05 | 14409 | 16629 | 277.955499 |
| Paired decoder | 14409 | 15871 | 265.285449 |
| C06 paired + fast inflate + GPU3520/index8192 | 14409 | 15603 | 260.805800 |

All four terminal actor images and RNG matched the B12 reference; result owners
completed, with no recorded fault or audio underrun. The unchanged native
schedule includes one65-period TIME UP wait and takes15075 fields. C06 was528
fields above that model (96.616035%); the task was not complete.

C06 identities: ELF `d70acb3eb80f2655e66e14594470d69a37ec39e1b8f865ab628fa170f6ed4cab`,
RAW `bc0aeecfa28d555356bdd4ffa4726d6d3ed42aa931c6253617160010fdb68c0c`,
BIN `759125526e134e2dbe4c24f0996c8341b43b09df83fc928424248a2d798a596c`.
Its linked end was0x060da348, below the dual-CPU ceiling0x060dc000.

The isolated profiled overlap experiment saved52 fields over identical ticks308
to3000 and533 decodes. Its full run was not completed. C07 combined native
Royal/TAG and SDK ownership gates passed before the loss, but the target partial
capture was still in progress. Never infer its final cadence from that run.

## Recovery and next acceptance

1. Keep B12 immutable as the reference. Preserve the restored original disc,
   assets, SDK archive and BIOS identities.
2. Recover literal source and fixtures from available conversation text. Label
   historical hash matches separately from unverified transcriptions and newly
   reconstructed implementations. Never invent a missing test result.
3. Restore the previously justified CPU, render, cache and paired-inflater
   optimizations, then rerun their relevant equivalence gates.
4. Rebuild and recapture the composed candidate. Preserve every displayed tag,
   explicit original wait and START phase/debt behavior.
5. The proposed wait-entry closure must drain the prior frozen display epoch
   before starting the original65-period wait, using audio service only. It must
   not publish live HUD changes early. Its final focal gate was not run.
6. Investigate the slave's cache-through polling as a new hypothesis only:
   overlap raised sampled callback work while the slave spent more time polling
   for VDP1. No backoff code or timing evidence existed before the loss.
7. Save reproducible checkpoints outside the transient executor, then finish
   Royal and TAG target validation and create the manual release.

Physical Saturn acceptance remains unverified. B12 release files must not be
replaced by a partial or unverified recovery build.

## Fresh recovery evidence

The restored B12 target was rebuilt and cold-booted again. Its ELF and RAW
are hash-exact matches to the earlier B12 capture. The disc image has a new
filesystem creation timestamp; its immutable assets were individually verified.
The complete Royal capture again measured17856 fields (298.4649343903437s),
with all14409 ticks, the matching terminal actor image/RNG and no recorded
fault or audio issue. Its original-wait clock schedule is15075 fields.
See `evidence/royal-recovery/b12-capture/analysis.json` and the retained raw
RAM/report evidence. This is fresh evidence, unlike the still-missing C06 run.

CPU reconstruction passed fresh full native Royal/TAG traces:36 snapshots,
222 exact files and every command/pixel row. Its portable pinned B12 references
and runners are under `tests/cpu_recovery`, with reports under
`evidence/royal-recovery/cpu`. The sparse ring reconstruction passed1441792
queries plus all seven scene pixel/dirty-map tests and saves12864 B of BSS.

A separate WIP source/history checkpoint has been saved successfully. It is
not a release candidate and does not replace the four B12 deliverables.
