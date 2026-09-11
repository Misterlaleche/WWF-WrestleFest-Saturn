# WWF WrestleFest arcade -> Sega Saturn

Private development repository for a native Sega Saturn reimplementation of Technos WWF WrestleFest (1991).

## ABSOLUTE AUTHORITY HIERARCHY

1. **SUPREME AUTHORITY: original arcade ROM set `wwfwfest.zip`.**
2. **Audited FINAL ROM-EXACT research / decompilation packages.**
3. **Saturn/Yaul technical authority.**
4. **Current Saturn code and test builds.**

If a lower source conflicts with a higher one, the higher authority wins. Unknown behavior remains OPEN/BLOCKED.

## AUDIO

WP24 transport is HARDWARE-PASS on real NTSC-U Saturn + Terraonion MODE.

Critical producer correction:
- low-stamina = `0x009088 -> 0x0B`, JSR `0x00908C`;
- `0x0263CC -> 0x0B` remains `REGISTERED_NOT_IDENTIFIED`.

Only producer rows classified `INTEGRATION_READY` may be wired directly.

## INTEGRATION MASTER R02

Branch: `integration-master-r01`.

Master-side platform integration currently provides:
- WP32 memory/owner contract;
- WP33 disc/CDFS/loading policy;
- VDP2 and CRAM capacity map;
- SCSP ownership map;
- fail-closed VDP1 policy;
- `wf_saturn_platform_memory.[ch]`;
- `wf_cdfs_runtime.[ch]`;
- `wf_cdfs_yaul.[ch]`;
- host tests for memory and CDFS loader;
- scene load preflight / overlap checks / atomic publication gate.

Validation:
- host PASS with `-Wall -Wextra -Werror`;
- SH-2 core compile PASS;
- SH-2 Yaul adapter compile PASS;
- no BIN/CUE generated;
- no hardware PASS claimed for this R02 layer.

The loader never reads files directly into renderer/audio-owned video/sound memories. Graphics are staged in HWRAM; LWRAM remains bulk residency only and is never an SCU-DMA endpoint.

Final VDP1 partition remains BLOCKED until authoritative runtime evidence proves simultaneous visible command and unique texture maxima.

Gameplay remains separately owned by `gameplay-runtime-r01`; do not duplicate State5/CPU/physics/collision/pin/TAG/Royal work here.

## MANDATORY READING

- `PROJECT-STATE.md`
- `HANDOFF.md`
- `docs/SATURN-PIPELINE.md`
- `docs/AUDIO-HARDWARE-VALIDATION.md`
- `docs/AUDIO-PRODUCER-CLEANR01-OWNER-MAP.md`
- `docs/SATURN-MEMORY-CDFS-INTEGRATION-R01.md`
- `docs/cdfs/CDFS-RUNTIME-LOADER-R02.md`
