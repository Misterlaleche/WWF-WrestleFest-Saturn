# Fluidity and Saturn save design, 25 October 2026

The user is testing stage9z on an NTSC-U Saturn with MODE. Continue work in the existing runtime18 checkout and preserve that delivered disc. Prioritize sustained combat speed and eliminate unnecessary resident assets. Keep arcade combat behavior intact while adding missing native transport and save behavior.

## Combat

Verify Royal's resident sprite pack against the measured tile manifest before comparing timing. The delivered stage9z pack already matches the regenerated pack byte for byte. Measure logical frames per VBlank and CD sector reads on the same scripted Royal route. Preserve a bounded cold fallback and demonstrate every sprite command refers to a valid tile. Diagnose the reproducible Royal frame6374 block against the original ROM before implementing the next movement branch. Keep TAG coverage.

## Save

Use Saturn internal backup memory through the Yaul BUP BIOS wrapper. A single versioned game save holds both arcade ranking tables (132 bytes at RAM offsets C2..145) and a small, explicit options area for future menu work. On first run, the ROM's factory ranking tables are the default. Load saved rankings immediately after the reset owner initializes factory rankings and before the title renders. Save after a completed result/ranking route; create an initial save on first boot if memory is available. Validate magic, version, length and checksum; damaged or unsupported data must never be copied into game RAM. Avoid backup calls in the combat frame loop. Never format internal memory automatically.

## Evidence

Host tests prove save encoding, validation, defaults, and ranking-table round trip. The Saturn link and HWRAM budget must pass. Emulator testing can measure speed and flow, but the user provides final physical MODE evidence.
