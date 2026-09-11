# Records / TOP RANKING R06

Authority: `WWF-WrestleFest-Saturn-Records-ROM-EXACT-FINAL-package(1).zip` + original ROM hierarchy.

Implemented master-side pure runtime core:
- exact 11x6 TAG and Royal table structures;
- exact factory defaults;
- descending sorter;
- `>=` qualification;
- NEW-before-OLD equal-value behavior;
- Royal `>100 -> 0` guard;
- TAG visible value `stored+1`;
- initials selector right/left wrap transforms;
- constants for 3-character blank, timeout `0x0700` and repeat threshold 8;
- table validation.

SATURN-SPECIFIC DESIGN, kept separate from ROM logic:
- WFRK v1 148-byte serializer/deserializer;
- exact 132-byte arcade payload preservation;
- big-endian header fields;
- CRC32 verification.

Not yet claimed:
- exact postgame runtime hook `$0065C0` in merged gameplay;
- full interactive initials controller/input edge/repeat choreography;
- pixel renderer `$69C8/$6A26`;
- Backup RAM Yaul adapter or physical persistence validation;
- attract ranking timing integration;
- hardware PASS.

No BIN/CUE generated.
