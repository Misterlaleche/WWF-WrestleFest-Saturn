# wp1979 native gameplay audit — final ROM revision 4

## Final selection

The frozen source/input/asset tuple in `HANDOFF.md` completes one native tag match at **logic frame 7372**, with a real Hogan→Warrior exchange at529 and DiBiase→Perfect exchange at5768. The human team wins at the original11B6 endpoint; all four requested actors are present throughout and become legal participants. The complete CSV replay has zero post-boot ROM/sprite reads and no blocked gameplay owner. Its full role/position/health trace equals the generating run byte-for-byte.

Use `fixes/state6/src/wf_cpu_runtime.c`, `fixes/src/wf_r17_human_raw23.c`, `host/tag-rom-v4-input.csv`, `assets/HOTSPR-ROM-v4-32.BIN` and the unchanged211-page header. `assets/wp1979-gameplay-manifest.json` records the full final identity and counters. Earlier9980 and10675 routes are historical artifacts of earlier native source revisions, preserved unchanged.

## Inspected source base and architecture

Read-only repository snapshot: Misterlaleche/WWF-WrestleFest-Saturn-Private, branch runtime18/avances-20261028, commit d95b8c37f62402bce4dead13cf7c0567b8fc2439. The audit did not mutate the repository, previous work, native working copy or reference snapshot.

The current Saturn source uses native C gameplay, scheduler, collision, CPU, physics, animation and composition owners, with Yaul VDP1/VDP2 and native SCSP transport. Program ROM remains immutable table/composition data. The m68k shadow structure preserves original register semantics used by translated owners/RNG; it does not decode 68000 instructions during the Saturn game.114 production TUs are used by the host fixture; only two proposed source files differ from the pinned snapshot. The independent ROM executor belongs exclusively to tests.

WIP160 has substantial gameplay but provisional TAG, geometry and combat behavior. ZIP20/22 retain relevant blocked collision, CPU and graphics producer paths. The pinned current runtime is the strongest inspected base. The earlier detailed source/graphics/timing notes remain in `handoff/pre-correction-v2/GAMEPLAY-AUDIT.md`, with their original source scope.

## Bootstrap, control and legal participation

Existing direct_boot with human 0, partner 1, cabinet 0300, RNGseed 1085 selects slots 0/1/4/5 IDs 0/1/9/7: Hogan, Warrior, DiBiase, Perfect. Initial stamina 110/106/145/130. No post-boot edits are needed. The immutable input tape contains1761 ordinary controller changes, decimal zero-based frame and active-low byte; values persist and rowN precedes completed logic frameN+1.

Presence is actor 00.bit7. Legal participant is 33.bit0, exchanged by original tag_exchange_188bc. Human control 33.bit1 can transfer during a rescue without a legal TAG. Role 56.bit6 indicates reserve/temporary CPU role; 32.bit0 indicates apron mode. The human corner request uses a normal fresh button and a ready partner; raw 4C/4D are the genuine outgoing/entering TAG owners. This audit records all these roles separately and counts only legal swaps.

| TAG | Start | Legal exchange | Entry finishes |
|---|---:|---:|---:|
|Hogan→Warrior|476|529|556|
|DiBiase→Perfect|5715|5768|5795|

Final pin counts6994/7082/7170; endpoint7372; result4001/4000/8000/8000;169A0300;16A00015; original result wait131VBlanks. Nominal seconds use frame/60 and require measured presentation cadence/capture offset before locating them in video.

## Graphics residency

Final R18D32 atlas has6357 tiles and19720 unique chunks; all reconstructed tiles match canonical SPRITES.PAK. Map50856 bytes padded50880; dictionary631040; header16384; total file698304. Resident LWRAM631040 leaves150 cold fallback slots. The unchanged211-page header fits the209main+14extra allocation. The complete final tape performs zero runtime ROM or sprite reads. This proves residency of the captured route; it does not establish every possible interactive pose/path.

## ROM corrections and causal proof

The final CPU source includes exact1CCB4 tagged-partner copy and state 5/raw 3D reaction, original carry wiring to the existing1C5FA helper, and state6 live24CC register context plus original1EE36 return. The human source supplies EA42's original local clearedD3 while preserving saved D0–D3. The1C094 invalid-link guard remains original. No actor/state/RNG normalization or invented recovery is used.

The6127 downstream guard was traced to6065 human EA42: originalDE86 savesD0–D3, DEB2 clears local D3, and DFFC restores the caller. Native inherited D3=016B incorrectly changed probability. The correction reproduces original state 5/raw 3F and RNG 75EB7FA0 in the observed fixture.

The later6093 CPU difference was independently isolated: original1EDDC clearsB5.4, then1EDFE calls24CC with live D2–D7. Native state6 alone used a stale all-zero legacy cache, selected the opposite branch and setB5.4 again. Original seed 2F50D97B/B5=03 versus v3 A2397F08/13. The corrected live selector matches memory, RNG and all data registers; initialized state6 also obeys the original RTS instead of advancing another owner early. Instruction bytes and fixtures are in `fixes/state6/STATE6-CAUSAL.md`.

## Verification and boundaries

Earlier independent tests cover50 CPU/valid-action comparisons,4 retained invalid-link guards and33 humanEA42 RAM/RNG/register comparisons. State6 tests cover149 cases: all 30 progression/tier rows, multiple seeds, live D2–D7 variants, credited/timer guards and initialized/uninitialized paths. Baseline fails136; final passes149/149. All are compared against executed immutable original instructions, without patched routines or gameplay callback stubs.

Fresh984 full phases over5800..6127 (F18A,1C150,F4C2) match every byte of all four actor records against the original ROM initialized from each observed native snapshot. Actual6093 is also a separate full-phase red/green fixture, including the captured native legacy cache. Original three failing input variants now pass beyond816/6127/10533; the former6127 tape reaches a real human-loss endpoint9015. Five regenerated ordinary-pad routes complete under final code; the selected shortest route retains early human TAG and both teams tagging, plus32-byte resident graphics.

These phase fixtures do not establish end-to-end original-PCB equivalence. The verified scope is the demonstrated correction families and selected deterministic native routes. Arbitrary manual input, complete ROM equivalence, native scanout cadence and physical Saturn hardware are not certified by this host audit. The parent integration supplies final emulator/video, audio and BIN/CUE measurements.
