Raw8D HOST7856, 2026-10-24. Stable owner, no combat probe.

Origin: human-tag-input-correct-sweep/policy-002; actual corrected-input HOST
policy 2,4,2,8,4,0,1. F4C2/12614 at7856 actor0 ID0 HP0 selector008D.
No HP, result, RNG, actor state or input edits were made to that source run.
Capture bridge pins dump/report/archive hashes, frozen layout7664/384/7340,
all9actors and D0-D7 byte identity; no A-register capture exists.
Snapshot SHA256 ccc95b00b2082fbd4b5a6b5326e8ce22ecf952da3e691b80fd1aba8fd59bccb1.
Exported represented raw state lies over zero only for unrepresented data;
this is explicitly a typed HOST replay, not a native SH2 trajectory claim.
The actual captured FG16KB is supplied for the natural differential call.

ROM table12614[8D] points17F9A; callback17FB2..18020. Kind1,4 frames;
durations6/FF00/FF00/FF00, posesB2/B3/1F/12. Initial258E[12] sets
VX=-256 (negated by BYTE2E.bit7),VY=0,VZ=0,VZDEC=28hex,motion2.
Latched callback calls10FC6 and IGNORES carry, preserving literal rope
WORD/LOW-BYTE writes and contact3 state4/sub17hex effects. Phase is BYTE25:
phase1 plus BYTE37.bit4 calls1110E(sound device preservesD0),10B62(+80),
clearsWORD22 then258E[3] setsVX/VY0,VZ300hex,VZDEC38hex,motion2.
Phase2 plus same contact clearsmotion/timer and setsBYTE60.bit7.
Common1C0C8 retains17F9A even if callback changes state/selector. Its full
WORD frame/timer rules, includingFF00 latch and padded high bytes, remain.
No actor link/team pointer is consumed; no HP/result/RNG write is owned.

Only new raw8D block/descriptor/case in actions_r11.c; shared helpers and
owners unchanged. Parent's separate action87.inc include/case integrated
as explicitly requested; its contents were not edited by this agent.
World low3bits7 with FLAG32.bit5 clear remains guarded at280DC BEFORE
entering resolver; original10B66 displacement80 and10B6A D0.W=contact
are retained, corrections untouched at that boundary. FLAG32.bit5 early
return and lazy non-phase1 calls do not invoke the unsupported world.
Existing resolver/other owner guards remain. No replacement OK paths.

RED action8d-RED.log proves natural7856 blocked12614 before port.
Final GREEN: 3975 whole intact1C03E calls +37 guards, including36 complete
ROM-entry prefixes280DC. RAM16KB, FG16KB, D0-D7, globals/RNG, descriptor
byte-exact; ROM SHA1a726c46f350723ea73158bc78055d17d418b14e2 unchanged;
no ROM patches or intercepted callbacks. StrictC11/O2/-UNDEBUG/Werror.
Matrices cover all9slots, BYTE latch/phase/facing, WORD timer/selector
padding, all owned world routes0..6, resolver coordinate wrapping,
rope priority and live WORD/LOW-BYTE flag distinctions, invalid links
ignored lazily, HP/results untouched, and shared contact event sequences.
The60 sequential calls use an explicit shared synthetic contact tape;
these are callback sequence tests, not invented physics/native timing.

Limits: no new whole-combat/native run and no trajectory/WIN claim.
A registers, CCR, stack and MMIO sound output outside typed combat ABI.
Unsupported resolver world7 remains a fatal dependency at its exact
compared entry prefix. Parent/root performs independent review/integration.

Production composed SHA256 b865753974040c61b2a931a7b1a6c16053ec6b562ffceacd391150945cb8db24
Owner(block+case) SHA256 16c952329c838a5549e24a347dc287ef8738ed9060424de262745a8ecc6bfdda
