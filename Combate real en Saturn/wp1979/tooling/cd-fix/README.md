# Native Saturn CD transfer correction

## Integration

Compile `wp1979_cd_read.c` alongside the application and make its header visible. In the project's CDFS implementation, include `wp1979_cd_read.h` and replace only these calls:

| Existing call | Replacement |
|---|---|
| `cd_block_sector_read(fad, buffer)` | `wp1979_cd_sector_read(fad, buffer)` |
| `cd_block_sectors_read(fad, buffer, length)` | `wp1979_cd_sectors_read(fad, buffer, length)` |

The existing `cd_block_init()` remains the initialization path. It returned zero in both the native full-disc diagnostic and the independent CD smoke. The SDK files and emulator core are unchanged. There is no link wrapping or symbol interposition.

## Observed cause

The supplied `libyaul.a` implements `cd_block_transfer_data` with 16-bit loads from `0x25890000`. Its matching source is in `libyaul-develop/libyaul/scu/bus/a/cs2/cd-block/cd-block_init.c`; the generated assembly was independently checked. Yabause's unmodified `Cs2ReadWord` has no corresponding sector FIFO handler at that address, so it returns zero. Commands still complete normally. This explains an apparently successful sector read followed by a failed ISO9660 PVD signature check.

Yabause's `Cs2ReadLong` implements the sector FIFO at `0x25818000`. Its `Cs2ReadWord` handler at `0x25898000` is for information transfers. The helper performs normal Saturn command-register transactions and consumes sector data from the 32-bit FIFO. It does not call any emulator interface or supply data from the host.

## Hardware basis

Sega Technical Bulletin #8 corrects the CD communication interface manual's page 102: `CDC_GetDataPtr` returns a `Uint32 *`, and CPU DMA/software access to the data register must use long-word units. Data ordering is upper word then lower word. The original correction appears on PDF pages 17–18 in the archived [Sega technical bulletin collection](https://antime.kapsi.fi/sega/files/Sattechs.pdf), retained locally as `Sattechs.pdf`.

The exact sector FIFO alias is independently documented by a firsthand [Saturn A-bus hardware capture](https://segaxtreme.net/threads/a-bus-cs2.24940/): srg320's July 9, 2021 post reports the retail BIOS sector-transfer path using long reads at `0x25818000` and provides captured A-bus transactions. This distinguishes the 32-bit sector alias from the BIOS information-transfer word alias. The local unmodified Yabause implementation corroborates this mapping; it is not the sole basis for the access method.

## Status and failure handling

The helper issues its own small set of standard CD commands rather than calling the SDK command wrappers. Those wrappers contain a separate status bug: they inspect the low byte of CR1 and bitwise-test ordinal drive states. This helper interprets CR1's high byte, masks its low nibble for the drive state, and compares states by equality. It handles rejection and WAIT responses separately.

The implementation bounds command/data polling at `0x240000` register polls and empty-sector waiting at 16,384 completed status commands. It does not reprogram FRT, which the native sound boot path also uses. These are finite poll budgets, not a claim of a calibrated wall-clock timeout on every hardware configuration.

Each request validates its FAD, sector-count encoding, destination pointer arithmetic and nonzero length. It accepts unaligned destinations and partial final sectors; the FIFO still consumes complete sectors while the caller receives exactly the requested byte count. A fast aligned path uses one long read and one long store per four bytes. End-transfer responses are checked against the expected word count. On a transport error, bounded best-effort end/abort cleanup preserves the original failure diagnostic. `wp1979_cd_diagnostics` exposes stage, command, response, requested/copied bytes and error.

## Native verification

The independent smoke is `../cd-smoke`. It runs on the unchanged Yabause v0.9.15 interpreter/software core with HLE BIOS, no overclock and no frame skipping.

The baseline test called the SDK read and then performed the same command sequence with long FIFO reads. The SDK returned success and 2,048 zero bytes. The FIFO returned `01 43 44 30 30 31 01 00` and the entire 2,048-byte PVD matched the ISO's LBA 16. Original probe source is `../cd-smoke/main-baseline.c`, with results in `../cd-smoke/capture`.

The final helper then passed:

| Case | Result |
|---|---|
| One sector at LBA 16 | All 2,048 bytes equal disc data |
| Sixteen sectors at LBA 16 | All 32,768 bytes equal disc data |
| Unaligned, 2,051-byte request | Exact data; surrounding guard bytes preserved |
| FAD 149 | Argument error |
| Null destination | Argument error |
| Overflowing request length | Argument error |

The checked memory dump, screenshot, source hashes and machine-readable assertions are in `../cd-smoke/fixed-capture/verification.json`, `report.json` and `final.png`. This is native emulator evidence. Physical Saturn validation has not been performed.
