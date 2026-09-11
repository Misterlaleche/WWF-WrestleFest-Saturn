# CDFS Runtime Loader R02 — validation

- Host compile/test: **PASS** (`-std=c11 -Wall -Wextra -Werror`).
- SH-2 platform memory compile: **PASS**.
- SH-2 CDFS core compile: **PASS**.
- SH-2 Yaul adapter compile: **PASS** against the supplied Yaul headers.
- Generated objects are 32-bit MSB Renesas SH relocatables.
- Object sizes:
  - `wf_saturn_platform_memory.o`: text 352, data 0, bss 0.
  - `wf_cdfs_runtime.o`: text 608, data 0, bss 0.
  - `wf_cdfs_yaul.o`: text 292, data 0, bss 0.
- Full Saturn ELF link: **NOT CLAIMED** until source-level merge with gameplay/frontend.
- Hardware PASS: **NOT CLAIMED**.
- BIN/CUE: **NOT GENERATED**.

Local cumulative package:
`WF-INTEGRATION-MASTER-CDFS-LOADER-R02-CANDIDATE.zip`

Package SHA256:
`8ff4f9a01b7c8442d792aa598a2bb6a2757e4c2b7acee8ee88695b3cbcba8c6b`
