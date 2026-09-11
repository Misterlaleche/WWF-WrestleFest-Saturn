# Gameplay HUD Core R16

Authority: WP18 HUD/SCORE/TIMER/COUNTERS/MESSAGES ROM-EXACT FINAL.

Implemented pure master-side model:
- exact four top-HUD FG0 anchors;
- exact player-indicator tile bases;
- exact raw 12-entry portrait descriptor table;
- queued stamina delta to signed visual accumulator;
- maximum two-unit displayed-stamina convergence per eligible update;
- clamp displayed stamina to [0,max] and clear accumulator on clamp;
- exact six-column stamina quantization/tile families;
- exact BCD timer seed 30:01 with immediate first decrement to visible 30:00;
- one BCD decrement every four eligible loop calls;
- seconds borrow to 59 and BCD minute decrement;
- proven five-minute event command byte 0x0B when decremented minute reaches 05;
- time-up clamps 00:00 and exposes transition-gate value 3.

No wall-clock timer is used.

Fail-closed boundaries preserved:
- no wrestler names;
- no generic score;
- no visible Royal elimination counter from +0xC4;
- no 0xC498 runtime path;
- no human strings assigned to message selectors 0x17..0x26;
- no universal physical x/y invented from FG offsets before gameplay scroll/camera integration.

Remaining R16 integration:
- raw message-stream metadata / 0xC710 selector logic;
- gameplay credit renderer/cache binding;
- referee/outside numeric renderer binding;
- renderer-specific tilemap writes;
- merge with gameplay phase ordering (0x262D2 at 0x1018, stamina 0x7548 at 0x10B0, messages 0xC710 at 0x10D0);
- real hardware validation.

Host + SH-2 compile PASS. No BIN/CUE.
