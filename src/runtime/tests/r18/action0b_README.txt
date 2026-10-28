Raw0B native Stage9, 2026-10-24. Stable owner before independent review.

Source: hardware-input/beetle-full-flow-stage9-human-tag, frame8798,
F4C2/12614, actor4 ID2/state8005/HP10/selector400B/LINK001C06BC.
Read-only Beetle/SMPC observation. Capture verifies program/BIN/CUE hashes
against linked stage9 ELF, frozen runtime header and SH2 layout, unique
all9actors and D0-D7 bytes in WRAM. Runtime7648, actors384/stride268,
Dregs7340. Snapshot SHA256:
a75628814f0d4f7053f3b71b976f322677be77d0332af7e258acea1c9d190494.
All9actors and supported globals projected; unrelated arcadeRAM explicitly
zero, no A registers fabricated. No native trajectory equivalence claim.

ROM12614[0B] points13A1E/callback13A2E..13B96. Kind1/count2,
periodsFF00/16,posesF1/F2. Initial target BYTE65!=9 copies targetX/Y,
decrements ownY then translates120/-1/0. Sub9 clears ownWORDfacing,
computes targetX+/-56 by targetBYTEface, compares unsigned with ownX,
stores newX then translates56/-8/0. Literal ordering handles self-alias.
258E[35]=VXFE00,VY0,VZ0400,DEC0058,motion2; ownBYTEface negatesVX.
D1/D2 WORD writes remain live.

Contact BYTE37.bit4 precedes terminal BYTE25=FE. Own timer/motion clear
before target BYTE21 is consumed. State4/sub8 or9 becomes sub6 or7,
clearing the subtype highBYTE, incrementing targetWORDD4 and pending11.
Otherwise helper11192/E958 tests raw61 with its asymmetric open-lower,
inclusive-upper hit box. A hit sets targetraw77/pending11/incrementsBYTEC7;
14422 is literalRTS. Miss sets ownstate4/sub5/pose1F/ownface/pending3,
globalWORD15D2=0F1A and LINKbit7. No-contact FE sets state7/LINKbit7.
13A1E remains selected for shared1C0C8 tick. No HP/result/RNG writes.

Only block/descriptor/case0B added in actions. Shared helpers unchanged.
Other authors' includes/cases20/54/62 integrated separately, inc untouched.
Unsupported links retain first-read guards:13A38 on init;13ABA on landing
after own timer/motion clears.60 prefixes compare exactRAM16KB/D0-D7.
Null/tagged-null addresses remain guarded; no-contact ignores invalidlinks.

RED action0b-RED.log reproduces12614 before port. Final GREEN2327 whole
intact1C03E calls+61 guards,60 full prefixes. FullRAM16KB/FG16KB,
D0-D7/globals/RNG/descriptor exact; zero ROM patches or intercepts.
ROM SHA1a726c46f350723ea73158bc78055d17d418b14e2 unchanged.
StrictC11/O2/-UNDEBUG/Werror, fresh copied sources. Matrices cover all9slots
and targets/self-alias/raw24-bit tags, BYTE state/subtype/phase/facing,
WORD timer/coordinate/counter wrapping, selectorhighbits, hitbox boundaries,
both landing results/miss, lazy reads and explicit all9 HP/result/RNG
invariance. Shared contact tapes are synthetic callback events in both
models, not invented physics/native timing. No combat probe performed.

Tested base source before isolated include integrations:
4aca7f31ad1b0179b90004f6bff767730287823530398979f6b93d1ff86a9b87.
Owner(block+case, canonicalLF) SHA256:
626e371774b67df5d7a994de30a3361c40b435928d6f198f9fdc0cce6733dad8.
Frozen exactbase retained; subsequent includes change only composedhash.
Fresh composition verification will be recorded separately. A/CCR/stack
and silent audioMMIO remain outside typed combat ABI. No new outcome claim.

Composition20/54/62 stable regression2327+61 PASS, actionsSHA256:
a061ebcdeb68d4702bcae734aae390d05d0c96d1e7aa99e7766d058fb4807fee
