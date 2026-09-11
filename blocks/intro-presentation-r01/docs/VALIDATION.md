# Validation

Host: PASS with `-Wall -Wextra -Werror` for FBI on/off route, exact 128+24 cadence, fade endpoints, RLE bounds, invalid-state fail-closed, and exact three-plate asset hashes.

SH-2/Yaul: compile PASS; ELF link PASS; unresolved symbols NONE. Yaul linker reports a LOAD segment RWX warning only.

Disc structural validation: PASS. ISO9660 PVD at LBA16, Saturn IP contains USA-capable area string, root payloads exact, A.BIN equals linked SH binary, raw single-track MODE1/2352 regenerated sector-for-sector with EDC/ECC, relative CUE.

Hardware: PENDING user test on Sega Saturn NTSC-U + Terraonion MODE. This remains R01-CANDIDATE.
