# New-chat handoff — WWF WrestleFest Saturn

Use this private repository and persistent ChatGPT Library as source of truth.

## Current start point

1. Read `PROJECT-STATE.md` first.
2. Current canonical validated source snapshot:
   - `/WWF-Saturn/WWF-Saturn-Test200-V0215-source.zip`
   - SHA256 `d4e55dc0ddc388b5842118ed459aa5cd35841888e219679b06cf9c530e59b9b2`
3. **Test200 / V0.215 has been genuinely reached.** Do not resume from Test171 or earlier.
4. Complete host regression: **192/192 PASS**.
5. Targeted ASan/UBSan for Tests172-200: **29/29 PASS**.
6. Tests172-199 are ROM-audit/test/documentation-only. Test200 validates existing native clear-stage stamina runtime against newly pinned ROM `$90D6` semantics.
7. Test147/V0.162 remains the latest SH-2/Yaul compile+link PASS.
8. No new BIN/CUE was generated after Test144.

## Hardware state

Test144/V0.159 remains the current pending hardware-validation image. Test100/V0.115 remains the latest image physically confirmed on NTSC-U Saturn + Terraonion MODE until Test144 is physically tested.

## Critical Test200 closure

- Linked-actor cleanup around the `+$109` producer is substantially tightened through `$21358/$213A6`, including TAG/Royal role flags and links, but `+$109` still has no defensible human-readable gameplay semantic.
- Earthquake raw action `$47` is now mechanically bounded through phase, contact branches, fallback and completion. Its trademark identity remains **CANDIDATE** and is not promoted by inference.
- Exact impact/callback operands were additionally pinned for Warrior, Jake, Boss Man, Smash, Perfect, Slaughter and DiBiase.
- `$90D6` is now closed as a four-record clear-stage stamina restore loop: skip `+$FF bit0`, exact `{24,32,12,0}` table from config bits `$6000`, add to live `+$66` and queued `+$6A`, clamp live stamina to `+$72` maximum. Test200 confirms native TAG runtime alignment.

## Fidelity rules remain mandatory

Preserve evidence labels: **ROM-EXACT**, **PROVISIONAL**, **REGISTERED BUT NOT IMPLEMENTED**. Never promote move identity, damage totals, AI behavior, geometry, timing or presentation detail to arcade-exact from plausibility or memory.

## Post-Test200 work if requested

The final Test200 report and remaining work are in `PROJECT-STATE.md`. The next logical phase is not arbitrary Test201 numbering: either (a) generate and validate a post-Test200 Saturn integration build, or (b) continue targeted fidelity closure on the still-registered systems before a new hardware image, according to the user's next instruction.