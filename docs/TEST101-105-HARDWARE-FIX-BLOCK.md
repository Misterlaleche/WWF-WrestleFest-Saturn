# Test101-105 hardware-feedback correction block

Source baseline: Test100 / V0.115, validated on real NTSC-U Saturn + Terraonion MODE.

- Test101 / V0.116: provisional difficulty-aware CPU offensive pacing after hardware Test100 showed disproportionate aggressiveness.
- Test102 / V0.117: provisional native damage retune for values that were already non-exact; exact ROM stamina maxima/tier/display logic and exact recovered finisher damage remain untouched.
- Test103 / V0.118: normal human TAG uses arcade-style move-toward-partner/corner + physical P. Synthetic INPUT_TAG remains diagnostic compatibility only.
- Test104 / V0.119: native referee follows live action instead of remaining parked at ring centre; exact path remains provisional.
- Test105 / V0.120: renderer exposes live 1/2/3 pin count near referee; exact arcade glyph art/placement can replace temporary text when recovered.

Validation: 101 host binaries PASS split 50/51; targeted Tests101-105 ASan+UBSan PASS.

No BIN/CUE generated at this checkpoint by user request. Next hardware image is deferred until a materially larger integration milestone or a Saturn-only blocker.
