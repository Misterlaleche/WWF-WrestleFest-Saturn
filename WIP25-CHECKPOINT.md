# Test255 WIP25 — ground composition identity closure

Classification: Test255/V0.270 WIP. Test254/V0.269 remains canonical.

## ROM-EXACT changes
- Ground/PICK-UP composition indexing is now selection/logical -> arcade actor/internal composition id:
  `[0,7,2,8,6,9,1,3,10,11,4,5]`.
- Rebuilt all 132 non-self `Gxy.PAK` matchup packs for the 12-wrestler roster from exact program/gfx2 composition data.
- Exact partner-dependent ground/PICK-UP coverage increases to 1265 compositions for the established pose families.
- Perfect logical character 7 now resolves PICK UP poses 206..210 through actor/internal set 3, closing the hardware-observed CHECK324/raw206 hole without fallback.
- Universal category-$05 prone strike pair phases 204/205 are present for every wrestler/opponent matchup.

## Fallback policy
- Ground actions (`ST_GROUND_ATTACK`, `ST_GROUND_PICKUP`) no longer use the HWFIX7 visual fallback.
- `RING14/OBJ15` HWFIX7 fallback remains temporarily enabled until their already host-validated streaming loader is connected to gameplay.

## Validation
- Test238 updated: exact Perfect PICK-UP + universal prone strike role coverage.
- Test251 updated: 1265 exact partner-dependent ground/PICK-UP compositions.
- Test276 updated: HWFIX7 ground fallback is closed; only ring/object remain provisional.
- Test289 added: actor/internal composition identity and full targeted ground coverage.
- Directed host passes: 214, 229, 231, 238, 245, 247, 251, 276, 289.

## Still pending in ground block
- Decode/implement callback-held completion for STOMP raw `$23` and FIST DROP raw `$26`.
- Replace remaining semantic/native ground-drop dispatch with the exact prone action producer/callback where proven.
- Exhaustive reachable-state audit must reach missing ground routes = 0 before the ground block is declared Arcade Pixel Perfect.

Library source checkpoint: `/WWF-Saturn/WWF-Saturn-Test255-WIP25-source.zip`
SHA256: `41c7b6e94da9e34bb118a2491f29bb8c0596b87275ec44fac4c54ad4640e62aa`

No arcade ROM bytes are stored in this checkpoint.
