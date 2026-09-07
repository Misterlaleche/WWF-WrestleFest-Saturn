# WIP81 — exact victim prone category and state4 word lifecycle

- Added `arcade_prone_victim_category(target65,target70,&category)` for the ROM-EXACT victim half of the prone classifier:
  - 08/06 -> category 5
  - 09/07 + target70==0 -> category 6
  - 09/07 + target70!=0 -> category 7
- Added `arcade_state4_prone_family_word_step(...)` to apply the proven state-4 prone-family transitions to a full +$64/+65 word while preserving high-byte flags.
- Covers exact 05 critical-latch initialization, 05 contact -> 08/09, 2A -> 08, 21 -> 08/09 by target71, and 06/07 -> 08/09 on actor25==FE.
- The helper never invents contact37 or actor25; runtime must supply independently proven event predicates.
- Test366 PASS; Test367 PASS; directed Tests357/358/361/363 and WIP80 Test365 PASS; `check_host.py` compile PASS.
- No global live prone selection change yet because state4 shadow lifetime is not closed for every native knockdown producer.
- Source ZIP: `/WWF-Saturn/WWF-Saturn-Test255-WIP81-source.zip`
- SHA256: `54de8b6963215066211871994a9f5c4da921a519f0b8680fdbc16dbbff5e484c`
- No held release, no `tick==12` change, no BIN/CUE, no ROM files.
