# ROM-EXACT sequence

Natural route: `RESET $000654 -> boot/video/DIP init -> optional FBI -> legal -> Technos/Tecmo -> TITLE $0081BA`.

FBI DIP packer is at `$001E1E`; packed DSW1 bit7 clear shows FBI, set skips it.

The synchronized wait at `$001E6C` completes exactly 128 VBlank handshakes before each common fade.

Common fade `$0264E2` is 8 exact palette-transfer levels in order 7->0, three complete output frames per level, 24 VBlanks total. Level 7 is identity and level 0 black. Palette replacement is VBlank-bracketed. No extra black-hold loop is demonstrated inside the fade.

The exact caller-entry scanline/first-visible raster phase remains OPEN, so this implementation introduces no guessed extra wait.

The plate routines have no local START skip. A credited global abort belongs to the shared credit/runtime owner and is not recreated in this isolated harness.
