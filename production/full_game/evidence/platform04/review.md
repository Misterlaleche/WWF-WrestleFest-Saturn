# Checkpoint04 focused review

Reviewed uncommitted `production/full_game` against `dff29f13b82104808d87e3744a73070fd5841ff1`, with `SPEC.md` and `docs/ASSETS-04.md` as scope constraints. Read-only production review; no Docker execution or production source changes.

## Findings

No prioritized actionable defects found in the checkpoint04 changes inspected. This conclusion covers the shared CD/resource loader and raw confirmed-slot identity binding, not completion of the game or the still-OPEN frontend/partner/Intro/Royal rendering work.

## Evidence

- Verified all 2,023 upstream source-lock entries. Generated HW1 CDFS/transport `.c` and `.h` copies are byte-identical to the locked upstream sources. Only the shared platform owns CD startup; the generated renderer retains no separate main, input owner, or clock. The linker map retains the intended one platform/transport implementation.
- Verified the seven current CD resources against approved TEST03 lengths and SHA-256. Their order with 32-byte alignment uses 130,848 bytes of the first 128 KiB LWRAM reservation, leaving 224 bytes. The active linked scene has no other LWRAM user; its decompressed render scratch and CD cache remain in HWRAM. Current terminal behavior does not reuse the arena while pointers remain live.
- Inspected loader arithmetic: `used <= capacity` is maintained before each subtraction/addition, including alignment padding. Capacity and all declared sizes are checked before content reads. Pointers are cleared first and populated only after every read and CRC succeeds; late corruption cannot publish an earlier resource. The current boot caller always supplies a valid result/output and a real fixed-size aligned arena.
- Rebuilt and ran assets, identity, frontend, clock, and cart host tests with `-Wall -Wextra -Werror -fsanitize=address,undefined` using temporary binaries outside production. All passed.
- Ran the production ROM oracle independently against the external original ZIP. The verified interleaved ROM SHA-1 was `a726c46f350723ea73158bc78055d17d418b14e2`. All ten native raw-identity conversions matched original `$5DA0` execution and `$64CC`; nine confirmation fixtures, 1,500 palette writes and the explicit setup fixtures completed. Confirmation/setup records are research fixtures, not a differential claim that those owners have been integrated into native production.
- Ran Python test discovery: four passed, five graphics-recovery tests skipped because `WF_ROM` was not supplied to that separate suite. The new ROM oracle was run with its explicit `--rom` argument and passed.
- Ran the negative-disc generator on the current built ISO into a temporary directory and confirmed equal ISO sizes and exactly one changed byte (SELPAL payload, offset 565248). Reviewed the runtime negative branch: it requires checksum failure at index 6, platform error 21, state 0 and all seven resource pointers zero. The normal branch retains prior controller routes/clock checks, adds successful load and raw-identity checks, and rejects changed CD sector counters after scene entry.

## Limits

Mednafen runs and the new Saturn build remain the parent agent's independent validation; this reviewer did not rerun Docker or hardware. The existing SDK implementation of `cd_block_init` was not re-audited from source here; bounded transport loops were inspected in the preserved HW1 implementation. Host resource tests exercise size-callback failure rather than a successful callback returning a wrong size, and early read/checksum failures rather than every failure index; the inspected implementation handles these paths, while the runtime negative case specifically covers the final asset. No claim of full-game completion, complete selector lifecycle, Intro transition correctness, corrected Royal artwork, or physical-Saturn validation is made.
