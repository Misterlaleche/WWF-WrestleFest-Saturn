# Test144 / V0.159 — Saturn hardware validation checklist

Target: Sega Saturn NTSC-U + Terraonion MODE.

## Build provenance

- Canonical source: `/WWF-Saturn/WWF-Saturn-Test144-V0159-source.zip`
- Source SHA256: `6ea546f185d71951596ca19419c7fbd50a2def0d24e0346ec91ca36350960eee`
- Hardware package: `/WWF-Saturn/WWF-WrestleFest-Test144-V0159-BIN-CUE.zip`
- Package SHA256: `b61ad9a53a3d159dbac9c34e4111f60f2aa6b11af5b98706fcd9f5c83f42646e`
- Raw BIN SHA256: `7610c9dfc015baa0573742286f2381d5b7b63bd2ccccfcb61844bd3c5199ac46`
- CUE SHA256: `bf558f9489edcd0809da6f884beddb070b1d4896eb7de4164d65f02c3400b1cd`
- SH-2 compile/link, Saturn/ISO structural checks, MODE1/2352 sector verification and independent libcdio payload read: **PASS**.
- Hardware status before physical test: **PENDING**. Test100/V0.115 remains the latest image confirmed on real hardware.

## Visible checks

1. Boot and reach title/mode flow without regression.
2. Normal combat, TAG and Royal remain playable relative to Test100.
3. Knockdown rise presents the recovered 13+13+17 descriptor chain without corruption.
4. On completed TAG/Royal matches, the ring remains visible and provisional winner/loser staging occurs instead of the old large centre result plate.
5. Record any crash, sprite corruption, missing asset, CD stall, palette issue or input regression with mode, wrestler(s) and exact moment.

## Do not infer from this build

- No arcade-semantic name is yet claimed for raw actor `+$109`, `+$60` or `+$61`.
- Test142 post-match coordinates/speeds remain **PROVISIONAL**.
- Exact victory clap, exact defeated/sad pose and exact arcade result typography remain **REGISTERED BUT NOT IMPLEMENTED**.
- The ten current finisher routes do not establish that **All Super Throws / Signature Finisher Moves** are fully ROM-exact; their individual evidence labels remain authoritative.
