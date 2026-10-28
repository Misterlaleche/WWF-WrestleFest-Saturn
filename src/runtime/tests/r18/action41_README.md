# Raw action 41: descriptor 1848C / callback 18498

The production owner is the new `raw41` block and admission case 41 in
`wf_r17_actions_r11.c`. Initial entry clears LONG 92 only. Latched entries call
the existing exact 10FC6 rope helper before selector/profile/link checks. A
qualified state 4 / subtype 1D peer receives reciprocal links and the original
handshake pose/timer/motion writes. FE uses the low animation BYTE and tags
the link. The single 14-tick pose 1A4 descriptor remains selected through all
state changes. No other guard or shared helper was changed.

The source pointer in 92 is copied in full, including high aliases; lookup uses
the existing 24-bit actor resolver. A tagged null reads ROM 21=FF and fails the
peer-state comparison as in the original. Unsupported non-actor relation
pointers still guard at 184CE after the original executed prefix. Unread
relations remain untouched on initial/selector-bit7 branches.

`action41_capture_beetle.py` reproducibly freezes the read-only Beetle guard
from frame 320 / slot 4. All nine actor blocks uniquely match the byte-swapped
2 MiB WorkRAM; D0-D7 come from the report. Typed global-prefix offsets match
the supplied SH2 observer actors/RNG layout and observed frame 320. Supported
globals are projected into RAM 8 KiB and unrepresented arcade bytes explicitly
remain zero. This is a natural typed-model observation, not complete original
arcade RAM or an independently matched HOST 7/2 route. Capture/ROM/program/disc/
schedule hashes and this limitation are recorded in `action41_manifest.json`.

The missing-owner RED reproduced 12614. Final differential PASS 11,317 plus four
unknown-pointer controls compares full RAM 8 KiB, FG 16 KiB, D0-D7, unchanged
RNG and tick descriptor against intact ROM 1C03E. It covers initial/latched,
WORD/BYTE/index/timer boundaries, profile/handshake gates, self/peer/tagged
links, all contact bits, low-BYTE rope guards and combined branch priority.
Only 2052 sound output is suppressed. Native sources are frozen before compiling
so concurrent unrelated owners cannot alter the tested translation unit.

Independent inventory-owner review repeated 11,317+4 and verified the ROM,
snapshot provenance and owner hash without blocking findings. Evidence:
`validation/local-20261024/action41-{red.log,final-green.json/log}` and
`action41-independent-review.txt`. The proof excludes the full scheduler,
physical audio, hardware cadence and global/free-combat equivalence.
