# Records BIOS safety on Saturn

The R01 adapter invokes the real Saturn backup BIOS after Jo has enabled its debug null-access detector. MPR-17933 BUP_Init at instruction $7D616 legitimately loads the decompressor vector at ROM $000005F8. Jo channel A covers data accesses in $000-$FFF; the BIOS read therefore records a false game fault. Jo checks that record only after the game callback returns, which explains why a fault recorded during initial records loading can surface at a later attract/title or match transition. This is a concrete defect; it does not establish that every reported hardware failure, or the long CD loading time, has the same cause.

The adapter saves the exact BBRA value, disables only channel A around BUP_Init, and restores it immediately on return, before any subsequent BIOS operation. It never clears Jo's previously recorded fault or disables JO_DEBUG globally. Both writes have volatile dummy reads: SH7604 manual §6.3.7 note 4 requires a read of the last written UBC register to ensure that its changed setting has taken effect. [Primary SH7604 manual](https://antime.kapsi.fi/sega/files/sh7604.pdf).

## Reproduce the focused tests

From the repository root, with a C compiler supporting AddressSanitizer and UndefinedBehaviorSanitizer:

```sh
python3 tests/test_save_ubc.py --sanitize
```

The test needs no BIOS file. It compiles the real SH-2 adapter branch with only the BIOS vectors and BBRA address redirected to a host MMIO fixture. Fifty-six cases cover exact BBRA restoration, init/open failures, corrupt/missing records, write/verify failures, preserved prior faults and subsequent game null detection. The unmodified R01 adapter fails because the legitimate BIOS read changes Jo's fault record. Existing persistence tests remain separate.

To independently decode the relevant instructions from a privately supplied original MPR-17933:

```sh
python3 tests/test_save_ubc.py --sanitize --bios /path/to/mpr-17933.bin
```

No BIOS data is included in this repository. The optional check reports the input hash and read address. `--source-dir` can select a historical adapter directory, and `--build-dir` retains the test log and report. Original and candidate hashes/results are in `evidence/records-hardware-20261113/ubc.json`.

## Evidence and limits

An isolated read-only Mednafen observer of the exact R01 image saw one post-SDK low data read through records initialization and attract: address $5F8, width 4, vector $1F04, with shadow BBRA=$6C/BARA=0/BAMRA=$FFF. The emulator's pipeline PC was $7D61A; the actual SH-2 load is at $7D616. This confirms the runtime path. It does not prove physical UBC behavior: stock Mednafen does not implement this trap, and its BBRA readback is zero. Host MMIO tests and target SH-2 assembly inspection establish the save/disable/readback/call/restore/readback sequence; physical Saturn acceptance remains pending.

## Complete backup workspace

Sega STN-52 corrects the BIOS work area to **8312 bytes**: an 8192-byte block table plus 120 bytes of variables. The separate library area remains 16384 bytes, for a minimum total of 24696. R01's 24576-byte lease was 120 bytes short. [Primary Sega STN-52](https://www.infochunk.com/saturn/segahtml_en/info/hon/stn52.htm).

The adapter now requests 32768 bytes, covering whole cache slots for both supported 4096-byte and 8192-byte sprite packs. The existing caller drains render/DMA work and borrows this cache tail only outside combat; returning it restores the full cache capacity. It is a temporary loan, with no permanent memory allocation. Static assertions check the BIOS minimum and both pack alignments. The standalone BIOS test disc derives its address from the same fixed cache end and new size.

```sh
python3 tests/test_save_workspace.py --sanitize
python3 tests/test_save_adapter.py
python3 tests/test_save.py
```

The workspace test requires the existing canonical `source_assets/ROM68K.BIN`, `source_assets/SPRITES.PAK` and stock `cd/SPRITES.WFP`; optional path arguments select another asset location. It creates the 8192-byte pack using the real builder's zlib path. The real adapter and cache loader are linked with a fake BIOS edge that writes the full documented footprint. Both pack formats check adjacent guards, every too-short lease size, alignment rejection before BIOS access, unchanged factory rankings, unchanged retained cache bytes, full-capacity restoration and exactly the borrowed slots being invalidated. The original 24576-byte source fails at adjacent guard byte zero for both formats; the new source passes under ASan/UBSan. Existing reboot/persistence/error and serialization regressions also pass.

The intermediate 28672-byte solution passed the 4096-byte test but was superseded because it cannot borrow whole 8192-byte slots. Its hashes/results remain recorded in the compact workspace evidence. The final 32768-byte candidate adds one temporary 4096-byte slot relative to that iteration.

This bounds test models the documented maximum workspace usage. It does not claim that R01's small internal-memory records file necessarily reached that maximum, nor that this overflow explains the reported hardware symptoms. Physical acceptance and the separate slow-CD investigation remain pending. See `evidence/records-hardware-20261113/workspace.json` for the exact source/pack hashes, baseline failures and review result.
