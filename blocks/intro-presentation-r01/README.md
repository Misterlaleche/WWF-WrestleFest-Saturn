# BLOCK-INTRO-PRESENTATION-01-R01-CANDIDATE

Isolated ROM-EXACT pre-TITLE presentation block for WWF WrestleFest Saturn.

Owned route: `optional FBI -> legal/copyright -> Technos/Tecmo -> TITLE handoff`.

Authority: original `wwfwfest.zip` (main 68000 SHA1 `a726c46f350723ea73158bc78055d17d418b14e2`) > audited Presentation FINAL > CLEAN-R01/Saturn authority > implementation.

Closed behavior:
- FBI gate is an explicit input; packed DSW1 bit7 clear = shown, set = skipped.
- Each emitted plate completes 128 VBlank synchronizations before fade.
- Common fade `$0264E2`: 8 exact transfer levels, runtime 7->0, 3 complete displayed frames each = 24 VBlanks.
- Level 7 identity; level 0 black.
- No local START skip.
- No pre-TITLE audio command is emitted because no producer is demonstrated in this owned route.
- After Technos/Tecmo black, emit TITLE handoff. TITLE itself is a separate owner.
- Invalid internal states fail closed.

Validation:
- host tests: PASS with -Wall -Wextra -Werror
- SH-2 compile: PASS
- Saturn ELF link: PASS; no unresolved symbols
- MODE1/2352 structural disc verification: PASS
- real NTSC-U Saturn + MODE: PENDING

The isolated harness is not the master main loop. No WIP160/175/176 runtime base was adopted. Protected `INTRO.RLE` is not committed; local build requires the exact asset SHA256 `16452b1bfd5559ff6ea33ec265e60c794763deba61f7492de121147afcffd3dc`.
