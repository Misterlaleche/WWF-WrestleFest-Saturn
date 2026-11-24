# Records and backup on Jo

The original ranking owner remains responsible for qualification, sorting,
three-character initials, timeout, and the TAG/Royal records graphics. The
transport must not insert records or manufacture a qualifying result.

`tests/test_ranking.py` executes the intact 68000 ROM as an independent oracle
and compares the native ranking code, native glyphs and native records
graphics. It checks all D/A registers and the complete cabinet/video/palette
memory regions for both modes, qualification thresholds, tied scores,
one/two/four participants, directional wrap, held confirmation and timeout.
Other scene/fade dependencies execute from ROM on both sides of this isolated
test. This proves neither Saturn pixel output nor physical input latency.

## Persistence transport

`port/wf_jo_save.c` uses the existing 160-byte WFS1 image: both adjacent
66-byte ranking tables at cabinet RAM `0xC2..0x145`, sixteen reserved option
bytes, and CRC32. It reads/writes only the exact internal-memory file
`WFESTSAT001`. A rankings-only update preserves loaded option bytes.

Initialize one `wf_jo_save_t` with `wf_jo_save_init(&save, NULL, NULL)`. Load
after original RESET installs the factory tables. Store after the original
ranking owner has completed. Calls need a four-byte-aligned, temporary
24,576-byte lease: 16 KiB of BIOS library plus 8 KiB of BIOS work memory.
The context is 164 bytes with the SH-2 ABI; no heap allocation or workspace
pointer survives an operation. Jo's 2 KiB texture stage is too small. The
caller drains video/DMA, completes outstanding audio service and borrows the
sprite-cache tail outside combat, without nesting a scene-preparation loan.
The caller returns and invalidates the loan after every result.

Missing storage leaves factory/live records intact. An absent file is written
only after the records actually change; factory-only sessions do not write.
`wf_jo_save_needs_store` performs the exact comparison without BIOS access, so
the caller can skip draining video or borrowing workspace for unchanged data.
A bad size, bad
CRC, unreadable file or ambiguous directory result prevents overwriting that
file for the session. A failed write/verify is reported and can be retried;
an unchanged successfully loaded/saved table does not open the BIOS again.
There is no formatting or deletion operation. Jo retains SMPC/reset ownership
through its reset-disable/enable calls; the adapter does not replace Jo's
audio driver. BIOS calls are synchronous, so actual device audio/timing and
power-loss behavior still require target validation.

## Reproduce host checks

```sh
python3 tests/test_save.py
python3 tests/test_save_adapter.py
python3 tests/test_ranking.py --report build/ranking-rom.json
python3 tests/test_main_save.py
```

The first test independently computes WFS1 bytes/CRC and rejects every single
bit corruption. The second runs the production save transaction against a
deterministic storage transport under ASan/UBSan, covering restart, unchanged
updates, options preservation, corrupt/full/unavailable storage and failures.
Only the BIOS I/O boundary is replaced; the hardware BIOS branch requires
an SH-2 build plus emulator/physical-console checks.

`port/main.c` initializes the save context once before frontend initialization.
Its RESET callback restores records after the original factory reset; its
ranking-complete callback first checks `wf_jo_save_needs_store`. Changed data
drains video while servicing audio, ends asset demand, cancels scene preparation,
borrows the cache tail, performs the synchronous operation, returns the loan
and services audio again. Storage failures remain nonfatal to gameplay;
renderer/audio ownership failures are still explicit errors.

The main integration test runs the actual cold RESET, selections, 14,409-tick
Royal combat loss and original ranking owner, with only the backup device
replaced. It verifies restoration after factory defaults, exactly one changed
record write, full cache recovery, unchanged callbacks preserving an active
scene-preparation loan, and continued play for failed writes/unavailable
storage. Its low-score save fixture affects records eligibility only; it does
not alter actors, outcomes, timers, RNG or gameplay progression. Host fields
are not evidence of Saturn speed or BIOS latency.

## Stock Saturn emulator and BIOS validation (2026-11-13)

The production SH-2 transport was then tested with stock Mednafen1.32.1 and
the supplied console BIOS, in dedicated disposable profiles. No memory writes
from the host, saved machine states or replacement backend were used. The
standalone test uses the actual0x002f6000 cache-tail workspace address:

- Missing file → BIOS write → separate cold boot → identical132 record bytes.
- A deliberately malformed existing file remains byte-identical and is not overwritten.
-127 ordinary BIOS test files fill the private device; the game adapter reports
  a write failure and returns normally.
- The actual full game loads a BIOS-created low-score fixture, completes its
  normal14,409-tick Royal loss and initials/ranking owner, and saves status5.
  A separate cold boot restores exactly the updated132 bytes from that game.
  Restored ranking SHA256:
  `c463fb208c24e531e033ea0810ed2b0ccb4ac91fb9ac4c8a5b4c1e5c591fc8b3`.
- The complete-game capture matches the native terminal and reports no audio
  faults, PCM/music underruns or music I/O errors. The BIOS probe alone does
  not validate audio timing; the complete-game capture supplies that check.

The integrated target uses `-Os` for the cold postmatch translation unit only.
It links at `_end=0x060db8c4`, below the enforced0x060dc000 boundary, retaining
the contiguous64KiB music allocation and all138 combat sprite cache slots.
The original SH-2 peephole workaround remains enabled at compile and link.

Reproduction helpers: `tests/build_save_bios_probe.py` creates an independent
test disc; `tests/run_save_bios_probe.py` cold-boots it and checks read-only RAM
observations. Scenarios1/2 create files and must only use their own private
emulator profiles. `tools/run_mednafen.py --backup-file FILE` seeds a fresh
full-game profile from a BIOS battery file and records its identity; no
emulator state is loaded. A final loaded probe can use `--rankings-file FILE`
to compare against the completed game's saved record payload.

Evidence: task workspace `bios-probe-{normal,corrupt,full}` and
`save-integration/build/capture-records-seeded`. Physical hardware and
power-loss behavior have not been tested.
