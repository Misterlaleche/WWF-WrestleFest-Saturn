# TITLE handoff

The INTRO block ends only after the Technos/Tecmo plate completes fade level 0 for its normal three displayed frames.

Output contract:
- scene = TITLE_HANDOFF
- phase = HANDOFF
- display palette is black
- no INTRO audio command pending
- no local input consumed
- INTRO VRAM/CRAM resources remain owned until Renderer/WP32 reclaims them

The master must then enter the dedicated TITLE owner corresponding to arcade routine `$0081BA`. TITLE internals, including its documented 128+504+1480 synchronized periods, are deliberately not implemented by this block.
