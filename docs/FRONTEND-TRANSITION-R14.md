# Frontend Transition Engine R14

Authority: WP15 FRONTEND TRANSITION ENGINE ROM-EXACT FINAL.

Implemented pure master-side transition primitives:
- common palette fade routine `$0264E2`: eight levels 7->0, three VBlanks each, 24 total;
- exact 16x8 4-bit fade table;
- stripe close `$008F50`: tiles `$4BE..$4C5`, palette bank F, three VBlanks each;
- stripe open `$008FD6`: `$4C5..$4BE`, three VBlanks each, then explicit FG0 clear token;
- generic exact VBlank wait primitive for proven waits such as 17/33/257;
- select horizontal slide: `$0140 <-> $0210` by 8 per update, exactly 26 updates.

The implementation deliberately does not collapse the arcade's distinct mechanisms into one generic visual fade. Full tile blank `$001FC0`, FG clears `$001F9E/$001FDE`, palette fade, stripe mask and selector slide remain distinct integration events/owners.

Important boundaries:
- no invented palette fade-in for title reveal;
- credited START title/attract -> mode remains direct clear/handoff, not common fade;
- select -> prematch retains exact 33-wait boundary;
- prematch -> gameplay remains direct teardown/handoff;
- Continue/Game Over internals remain WP14 authority;
- human labels for B608/BDA6 are not inferred;
- `$0C2000..$0C3FFF` semantic layer ownership remains unpromoted beyond proven writes.

Validation:
- host compile/test PASS with `-std=c11 -Wall -Wextra -Werror`;
- SH-2 freestanding runtime object compile PASS;
- runtime object text 1684 bytes, data/bss 0;
- no Saturn ELF link claimed for this isolated module;
- no hardware PASS claimed;
- no BIN/CUE generated.
