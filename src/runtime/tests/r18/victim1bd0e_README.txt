Victim1BD0E/raw54 counterpart, 2026-10-24. Stable for independent review.

Actual origin: human-tag-9b-actions-sweep/policy-002, HOST6733,
F4C2/dependency1BD0E, actor5 state8004/subtype001E/HP0. Input policy
3,4,2,8,4,24,1, no HP/result/RNG/actor-state writes or new combat probe.
All dump/report/source-archive hashes verified. HOST layout7664/actors384/
Dregs7340 verified against frozen headers. All9actors and D0-D7 byte-equal
between typed dump, separate dumps and raw export. Snapshot SHA256:
920545e585d000be858024e77ee1daea1983eddc584524744d2459222ca11621.
Exported represented globals/objects over explicit zero unrepresented RAM;
no A registers fabricated, no claim of original native/arcade trajectory.

Original1AFD4[1E] points1BCF6: callback1BD0E,kind1,count1,period16,pose172.
Local alternate1BD02 has identical kind/count/period,pose1AE. Its address
is absent from the generated catalog, so the new tick adapter uses the
existing primary tick rules and substitutes only its literal alternate
pose on a frame load. Original selector choice is passed explicitly.

Init clears own motion BYTE then EXG A0,A2 makes10B9A's source the
attacker, from victim.LINK.10B9E then loads recipient=attacker.LINK.
It need not equal the victim. All source XYZ copies occur before the
40/-1/20 translation, signed by attacker's BYTEfacing bit7. Self/third
actor aliases preserve that exact order. D1.W=FFFF/D2.W=0014 survive;
caller D0 is restored only on successful whole1C03E, as the original.

Latched non-FE returns immediately without any link read or alternate
selection. Latched BYTEphaseFE writes state4 and fullWORDsubtype100F.
Only init or FE then compare original BYTEselector61 with0F: equal keeps
1BCF6, otherwise selects1BD02. FullWORD animation-counter/index behavior
and original tail18/1A clearing remain, including FF00, WORDwrap and
high phase padding. No HP, result or RNG mutations belong to this owner.

Unsupported first-link addresses retain guard10B9E before its first read.
Unsupported second-link addresses retain guard10BA2 before first store.
Both keep own motionclear and D0.W0028/D1.WFFFF/D2.W0014 prefix. No
coordinate store or facing negation occurs before those guards. Latched
paths ignore invalid links lazily. Existing unrelated guards stay intact.

RED victim1bd0e-RED.log proves actual6733 blocked1BD0E before admission.
Final GREEN3397 whole intact1C03E calls+63 guards,60 complete ROM prefixes
10B9E/10BA2,3 retained invalid subtype table guards. RawRAM16KB,FG16KB,
D0-D7/globals/RNG and selected descriptor token match; all9 HP/results
and RNG explicitly checked unchanged. No ROM interception/patching.
ROM SHA1a726c46f350723ea73158bc78055d17d418b14e2 unchanged.
StrictC11/O2/-UNDEBUG/Wall/Wextra/Werror on freshly copied sources.

Descriptor observation is test-only C instrumentation: one trace write
at the copied state4_1bd0e_tick records the alternate argument actually
produced by the owner (1BD02 vs primary descriptor). It does not recompute
selection from fixture inputs, change production, or intercept ROM.
The explicit descriptor assertion also covers latched no-load branches.
Production/instrumented hashes are separately recorded in final JSON.

Matrices cover subtype flags, phase/latch/selector BYTE tests, timer/index
WORD wrapping, all attacker/recipient actor IDs and raw24-bit tagged
aliases/self/third-actor paths, coordinate wrap/facing padding, lazy bad
links, both descriptor poses, 16-tick sequences and exact tail effects.
An initial alias fixture inadvertently redirected its first hop to an
uninitialized third link and correctly hit retained10BA2; its log remains.
Only fixture links were initialized to represented actors; no guard or
production behavior was relaxed to pass it. Sequence tests execute only
this callback while its subtype remains1E, without invented physics.

Production source SHA256:
9bb5615b720983839803c862951e6beb19202711fe19244b4ea01c9bf7e59a1d.
Owner(block+admission,canonicalLF) SHA256:
dbced86ee59072b550ac8194e3f78f46bdc275211fea38ddf052fc0878923519.
Boundary proof compares previous source from the original probe archive:
only the new owner/tick and admission were added; all other normalized
text is identical. No actions/input/CPU/entry/Makefile edits for this task.
No new global probe, forced outcome or native timing claim. A/CCR/stack
and silent audio transport remain outside typed combat ABI.
