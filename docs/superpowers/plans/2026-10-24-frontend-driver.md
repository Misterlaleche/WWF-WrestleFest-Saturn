# Original frontend driver implementation plan

> Execute inline in frontend_inventory; no task threads, SDK/backend, entry or Makefile edits.

**Goal:** Compose closed original-ROM owners from654 through title/credited START/mode/roster toAC0 with explicit hardware events.

**Architecture:** One borrowed raw16KB RAM and video shadow, own D/A/control cursors. Mandatory platform ROM/MMIO/wait/world/submit/commit callbacks; existing native IO/map/board/leaf/fade/IRQ/coin own effects. Generation token abandons the interrupted scene after8AE.

**Tech stack:** Portable C11, existing runtime18 modules, host C harness and local original-ROM differential tests.

- [x] Publish frontend_driver.h callback/ownership API; document source scope and bus routes.
- [x] Add failing host composition harness: cold reset, real level tape, coin qualification, credited START, mode/roster handoff; unknown dependency must fail.
- [x] Implement driver dispatcher and event bridges in frontend_driver.c, no SDK dependency.
- [x] Compose reset/boot/title/modeplate/mode/roster cursors, preserving RAM/D/A and diversion generation.
- [x] Verify host tape paths and strict host compilation; compose already-qualified original-ROM modules; preserve state/IO traces and limits.
- [x] Publish hashes/API/evidence for root independent review and Saturn backend integration.
