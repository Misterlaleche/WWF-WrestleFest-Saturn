# WWF WrestleFest arcade -> Sega Saturn

Private development repository for a native ROM-EXACT Sega Saturn reimplementation of Technos WWF WrestleFest (1991).

## Authority

1. original `wwfwfest.zip`;
2. audited FINAL ROM-EXACT packages;
3. Saturn/Yaul technical authority;
4. current Saturn source/tests.

Unknown behavior remains OPEN/BLOCKED.

## Current validated blocks

- MODE+ROSTER TEST03: HARDWARE-PASS on Saturn NTSC-U + MODE.
- WP24 audio transport: HARDWARE-PASS.
- Integration memory/CDFS platform: host/SH-2 validated; no hardware-pass claim for new integration layer.

## Integration master

Branch: `integration-master-r01`.

Current master infrastructure:
- WP32/WP33 memory+CDFS contracts;
- CDFS runtime loader;
- HWRAM staging / LWRAM bulk residency guards;
- WP26 VDP2/CRAM map;
- WP24 SCSP map;
- R03 machine-readable final-memory manifest + fail-closed validator;
- R04 WP30 orphan ownership resolution;
- R05 fail-closed Scene Flow Master root router;\n- R06 Records/TOP RANKING core + Saturn save serializer;\n- R07 Ending Credits controller;\n- R08 Attract Biography controller/pack contract;\n- R09 Attract Demo 14-state/threshold/90-VBlank root contract.

R03 final-fit remains blocked by real unresolved inputs: VDP1 maxima, integrated ELF/link map, HWRAM scene peaks, final CDFS root count and hardware transfer timing.

R04 resolves project ownership:
- master owns Records, Ending Credits, Attract/Bio root, Scene Flow Master root;
- gameplay owns CPU-controller remainder and WP31 objects/effects/props.

## Important integration rules

- never replace CLEAN-R01/master `main.c` with isolated test harnesses;
- MODE+ROSTER behavior is transplanted from TEST03, not its architecture;
- WP24 is the only audio backend;
- gameplay branch remains owner of combat/CPU/objects and the WP17 `$1F7F0` CPU override/shared RNG service;
- no WIP160/175/176 as canonical gameplay base;
- attract demo never synthesizes joystick/button autoplay;\n- no BIN/CUE unless explicitly requested.

## Mandatory reading

- `PROJECT-STATE.md`
- `HANDOFF.md`
- `docs/MODE-ROSTER-HARDWARE-VALIDATION.md`
- `docs/AUDIO-RUNTIME-HANDOFF-R01.md`
- `docs/SATURN-MEMORY-CDFS-INTEGRATION-R01.md`
- `docs/cdfs/CDFS-RUNTIME-LOADER-R02.md`
- `docs/MEMORY-MANIFEST-R03.md`
- `docs/WP30-OWNERSHIP-RESOLUTION-R04.md`
- `docs/SCENE-FLOW-MASTER-R05.md`\n- `docs/RECORDS-R06.md`\n- `docs/CREDITS-R07.md`\n- `docs/ATTRACT-BIO-R08.md`\n- `docs/ATTRACT-DEMO-R09.md`
