# AUDIO PRODUCER MATRIX AUDIT — 2026-09-11

A complete producer matrix was rebuilt from the audited Audio Complete Integration package and then checked directly against the original arcade program reconstructed from `wwfwfest.zip`.

## Direct-ROM result

- 173 total producer rows.
- 163 immediate `MOVE.W #imm,D0` producers: 163/163 byte-verified.
- 10 dynamic/dataflow producers: 10/10 `JSR $00002052` sites verified.
- 138 context records joined.
- Base evidence: 12 `RECOVERED_ROM`, 161 `REGISTERED_NOT_IDENTIFIED`.
- Seven finisher producer rows are independently upgraded to `RECOVERED_ROM_USAGE` only through `finisher_audio_evidence.csv`.

The 68000 program reconstruction used:
- even bytes: `31e14-0.ic18`;
- odd bytes: `31e13-0.ic19`.

Immediate rows were accepted only when:
1. source bytes equal `30 3C <raw_word>`;
2. `call_address == source_address + 4`;
3. call bytes equal `4E B9 00 00 20 52`;
4. `effective_command == raw_operand & 0xFF`.

## Critical correction

The previous bridge incorrectly promoted `0x0263CC -> 0x0B` as the low-stamina warning producer.

The audited source evidence identifies the recovered low-stamina path as:
- `0x009088: MOVE.W #$310B,D0`;
- `0x00908C: JSR $00002052`;
- active actor `+$66 < $0018`;
- latch `$1C0166` prevents every-frame replay.

The complete manifest also contains `0x0263CC -> 0x0B`, but that row remains `REGISTERED_NOT_IDENTIFIED`. Equal command identity does not prove equal gameplay semantics. The runtime bridge now rejects `0x0263CC` as a low-stamina producer.

## Conservative CLEAN-R01 ownership

- recovered low-stamina producer -> WP10; backend WP24;
- recovered ring-out/count-out producers -> WP08; backend WP24;
- five recovered finisher moves / seven producers -> WP01 primary; WP22/WP10 supporting; backend WP24;
- all other producer ownership remains unresolved until demonstrated.

No semantic owner is inferred from audible similarity, command identity, or nearby state alone.

The full 173-row matrix is retained in the audited external work artifact generated from the source packages.
