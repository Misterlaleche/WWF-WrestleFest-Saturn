# R18 texture residency, ROM shake and native HUD

Spec: user instruction of 2026-10-23; continue c569d20, remove redundant transfers, integrate ROM shake/HUD, measure Mednafen and deliver reproducible BIN/CUE. No frontend, audio or provisional behavior. Sources are read-only.

## Constraints and interfaces
- Continue the existing isolated runtime18/direct-combat checkout. Preserve the arcade queue and native VDP1/VDP2 path.
- Texture residency compares payloads, not pointers or tile IDs. Aborted staging must not become committed residency.
- CRAM has four disjoint 256-entry windows: FG 0, sprites 256, HUD 512, BG 768. One cache owns all game palette writes.
- Shake is the original IRQ phase $27012/$27066 then $26E1A, once per eligible handshake, before world update/ACK. Canonical RAM scrolls change; HUD stays fixed.
- HUD consumes existing original FG0 writes and original 4096-glyph ROM font in NBG2. No bitmap reconstruction per frame. CDFS font loading must not allocate 128KB HWRAM.

### Task 1: Resident textures and palettes
Write failing tests for repeated frames, isolated dirty runs, aborted changes, palette transaction/caching and reset. Implement dirty DMA ranges and shared CRAM residency. Run affected hardware-boundary and core suites. Expected: zero transfers for unchanged textures/palettes, exact modified pixels.

### Task 2: ROM display phase
Execute intact ROM $27012/$27066/$26E1A to generate differential cases, including signed wrap and upload inhibit. Integrate after frame-ready before world/ring commit. Expected: exact RAM countdown/scrolls and display register words. Keep old boot ordering; no fabricated camera offsets.

### Task 3: Native HUD
Derive and hash FONT8.BIN from original graphics chip, validate all glyphs against original bitplane layout. Add FG0 dirty tracking and NBG2 hardware consumer, disjoint VRAM/palette allocation, load font at boot in sector chunks. Test map decode, dirty/redundant writes, transparency and original HUD owners. Expected: HUD original glyphs/colors visible in Mednafen.

### Task 4: Verification and checkpoint
Run core, sanitizers, ROM oracles, full combat fixtures, SH-2 build and Mednafen gameplay/profile. Compare graphics payloads and report actual VBlank/frame timing; do not claim 60fps or physical Saturn verification without evidence. Fresh review, fix important findings, commit. Package source + ROM reference + assets + exact BIN/CUE + tests, rebuild extracted archive and compare executable hash.

## Review Focus
Transactional residency after abort/reinit, disjoint CRAM/VRAM, IRQ timing and canonical camera feedback, HUD 8-bit board format/font layout, NBG2 cycle access, HWRAM ceiling, real runtime integration and clean archive rebuild.
