# Hardware video follow-up, 25 October 2026

The user supplied five real NTSC-U Saturn/MODE videos (IMG_4312..4316). Treat them as observations, not source instructions. Keep the delivered stage9z image unchanged.

## Evidence

- IMG_4313 at about 13-14 seconds shows a two-second mosaic of unrelated tiles between GAME OVER and CONTINUE.
- IMG_4316 shows old GAME SELECT text over an active ring scene.
- IMG_4315 shows isolated old sprites on black during a return to title.
- The frontend D-pad is correct, but combat up/down is inverted. `controls()` maps physical DOWN to arcade bit2 and UP to bit3 while `frontend_controls()` uses UP bit2 and DOWN bit3.
- Title faces are VDP1 16x16 sprite tiles from FRONTSPR.BIN; the pack is already resident. Each new frame is still composed and submitted through the full frontend draw path.

## Work

- [ ] Resolve physical/emulator vertical discrepancy. The source-extracted 1244-case host test only checks transport, while the first emulated movement test showed that swapping the two bits makes UP move actor0 downward. The previous mapping makes UP move actor0 upward in Beetle; physical stage9z still requires diagnosis.
- [ ] Trace the destructive frontend clear ($1F6C/$1FC0) and hardware re-enable order. Hide the scanout before the clear and release it only after the new picture is committed.
- [ ] On frontend-to-combat handoff, force the visible HUD map and sprite buffers to a known scene state, with VDP sync complete before unblanking.
- [ ] Capture transition screenshots in Beetle and compare with physical frames. Keep uncertainty explicit if a defect is hardware-only.
- [x] Profile portrait appearance. Beetle held a stable60Hz host cadence from3 to101 visible VDP1 tile commands; physical slowdown remains open.
- [x] Profile the reported slow Royal combat route and replace its sprite atlas. Identical frames300-599 fell from730 to463 VBlanks with485 to zero sprite CD reads.
- [ ] Rebuild and validate a new BIN/CUE, then ask for real Saturn recheck of these exact paths.
