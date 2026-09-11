# WWF WrestleFest arcade -> Sega Saturn

Private development repository for a native Sega Saturn reimplementation of Technos WWF WrestleFest (1991).

## ABSOLUTE AUTHORITY HIERARCHY

1. **SUPREME AUTHORITY: original arcade ROM set `wwfwfest.zip`.**
   - If code, notes, a previous build, a workpack or any other document disagrees with the original ROM, **the ROM wins**.
   - Main 68000 program authority used by the audited research: SHA1 `a726c46f350723ea73158bc78055d17d418b14e2`.
2. **Audited FINAL ROM-EXACT research / decompilation packages stored in the ChatGPT project sources.**
3. **Saturn/Yaul technical authority.**
4. **Current Saturn code and test builds.**

## FAIL-CLOSED IMPLEMENTATION RULE

Do not invent provisional gameplay, timings, state transitions, input behavior, RNG, collision rules, frontend logic, animation, graphics behavior, audio behavior or loops merely to make a build appear complete.

If a behavior is not demonstrated, leave it OPEN / incomplete until evidence exists.

## BLOCK WORKFLOW

Development is split into isolated hardware-testable BLOCKs. Final integration is performed from source modules and shared Saturn infrastructure. **BIN/CUE images are never concatenated.**

## AUDIO — CANONICAL CURRENT STATE

Transport is HARDWARE-PASS on real NTSC-U Saturn + Terraonion MODE. The producer matrix is ROM-audited; only producer rows marked `INTEGRATION_READY` may be inserted directly into runtime code.

Critical correction:
- low-stamina producer = `0x009088 -> 0x0B`, JSR `0x00908C`;
- `0x0263CC -> 0x0B` remains `REGISTERED_NOT_IDENTIFIED`.

Reusable backend: `src/audio/wp24/`.

## INTEGRATION MASTER R01

Branch: `integration-master-r01`.

This branch owns the shared master-side WP32/WP33 contract while gameplay continues independently.

Current additions:
- `docs/SATURN-MEMORY-CDFS-INTEGRATION-R01.md`
- `docs/INTEGRATION-MASTER-R01.md`
- `docs/memory/*.csv`
- `src/platform/wf_saturn_platform_memory.[ch]`
- `tests/platform/test_wf_saturn_platform_memory.c`

Validated locally:
- original main 68000 SHA1 reverified;
- host compile/test PASS with `-Wall -Wextra -Werror`;
- SH-2 object compile PASS;
- no BIN/CUE generated;
- no hardware PASS claimed for this new integration contract.

Renderer memory policy:
- VDP2/CRAM capacity contract is closed at Saturn technical-equivalence level from WP25/WP26 FINAL;
- final VDP1 command/texture partition remains BLOCKED pending demonstrated maximum simultaneous visible commands/unique textures;
- LWRAM is never an SCU DMA endpoint;
- WP24 owns all SCSP RAM;
- visible LOADING is not default and may exist only as a measured `SATURN PLATFORM ADAPTATION`.

Gameplay remains owned by `gameplay-runtime-r01`. Do not duplicate State5/CPU/physics/collision/pin/TAG/Royal work here.

## MANDATORY CROSS-CHAT HANDOFF

Before changing the port, read:
- `PROJECT-STATE.md`
- `HANDOFF.md`
- `docs/SATURN-PIPELINE.md`
- `docs/AUDIO-HARDWARE-VALIDATION.md`
- `docs/AUDIO-PRODUCER-CLEANR01-OWNER-MAP.md`
- `docs/SATURN-MEMORY-CDFS-INTEGRATION-R01.md`
