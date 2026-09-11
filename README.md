# WWF WrestleFest arcade -> Sega Saturn

Private development repository for a native Sega Saturn reimplementation of Technos WWF WrestleFest (1991).

## Non-negotiable authority hierarchy

1. SUPREME AUTHORITY: original arcade ROM set `wwfwfest.zip`.
   - If code, notes, a previous build, a workpack, or any other document disagrees with the original ROM, the ROM wins.
   - Main 68000 program authority used by the audited research: SHA1 `a726c46f350723ea73158bc78055d17d418b14e2`.
2. Audited FINAL ROM-EXACT research/decompilation packages available in the ChatGPT project sources.
   - In particular the project source archive `zips finales.zip` contains multiple audited FINAL workpacks.
   - These are high-authority engineering/reverse-engineering results, but remain subordinate to the original ROM.
3. Saturn/Yaul technical authority and audited Saturn implementation contracts.
4. Current Saturn implementation / test builds.

## Implementation rule

The project is not allowed to invent provisional gameplay, timings, state transitions, input behavior, RNG, collision rules, frontend logic, animation, graphics behavior, audio behavior, or loops merely to make a build appear complete.

If a behavior is not demonstrated, implementation must fail closed / remain incomplete until evidence is available.

## Development method

Development is split into isolated hardware-testable BLOCKs. A BLOCK may produce its own BIN/CUE harness, but final integration is performed from source modules and shared Saturn infrastructure; BIN/CUE images are not concatenated.

A BLOCK becomes integration-eligible only after ROM/audited-evidence traceability and real NTSC-U Saturn + MODE validation for hardware-dependent behavior.

See `AUTHORITY.md`, `PROJECT-STATE.md`, `HANDOFF.md`, and `docs/AUDIO-TEST-01.md`.
