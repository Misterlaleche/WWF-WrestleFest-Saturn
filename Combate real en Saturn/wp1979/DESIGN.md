# WP1979: native Saturn TAG combat speed demonstration

## Authorized goal
Independent, hardware-testable Saturn program showing Hulk Hogan and Ultimate Warrior versus Ted DiBiase and Mr Perfect, using original WWF WrestleFest behavior and assets. Deliver actual emulator recording, raw MODE1 BIN/CUE and reproducible sources. Original archives and all existing repositories remain unchanged. The user authorized autonomous execution and delegated routine implementation choices.

## Selected architecture
Use a separate, pinned source snapshot of native runtime18 (`d95b8c37f62402bce4dead13cf7c0567b8fc2439`) as a reference. Its SH-2 C routines translate the arcade owners and preserve register side effects. Keep native game simulation, VDP1 sprites, VDP2 layers and resident assets. The older WIP160 contains provisional behavior and is unsuitable as gameplay authority. A graphics-trace replay alone would not establish native gameplay throughput.

The executable starts this single TAG fixture directly, using the original setup and a disclosed deterministic diagnostic seed selecting the required opponents. Controller input and a reproducible demonstration input schedule both feed the real game input owner; no recorded positions or rendered video are used as the game.

## Acceptance evidence
- ROM checksums, exact four actor identities and original setup owners.
- Independent per-tick displacement and animation-clock checks against ROM tables.
- Sustained completed native game frames and displayed frames per Saturn VBlank, with no emulator overclock or speed-up in the supplied recording.
- Report MAME's 7 MHz/(448*272) = 57.444852941 Hz as the emulator reference; original PCB crystal/total timing is not independently measured here.
- Real emulator boot, visible scene inspection, valid CD bootstrap/layout, bounded RAM/VRAM and no undefined dependencies during the recorded combat.
- Physical Saturn validation is reserved for the user's test, never inferred from emulation.

## Scope decisions
Use the default cartridge-free NTSC path first. Keep initialization and loading outside the measured fight. If a native unsupported branch occurs, preserve its diagnostic, investigate ROM evidence, and fix only this independent copy. Preserve source attribution and include all limitations in delivery notes.
