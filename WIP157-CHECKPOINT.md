# WIP157 CHECKPOINT — held Ground motion literal writer inventory

## Scope
Close the exact literal producer set for `+$5C/+$5E` and associated `+$01=2` activation sites inside reachable callback-held Ground actions. No live wiring.

## ROM-EXACT inventory
Twelve exact setup sites are now registered:

```text
raw $0B:  $13BE0 +$5C=$0700, $13BE6 +$5E=$0048, actor01=2 at $13C2C
raw $0E:  $13D8A +$5C=$0600, $13D90 +$5E=$0048, actor01=2 at $13DDE
           $13FB2 +$5C=$0700, $13FB8 +$5E=$0048, actor01=2 at $13FD8
raw $10:  $14210 +$5C=$0300, $14216 +$5E=$0048, actor01=2 at $142A0
           $1427C +$5C=$0300, $14282 +$5E=$0048, actor01=2 at $142A0
raw $13:  $14A46 +$5C=$0300, $14A4C +$5E=$0030, actor01=2 at $149FC
raw $14:  $14BA0 +$5C=$0500, $14BA6 +$5E=$0048, actor01=2 at $14BEA
raw $23:  $15DE2 +$5C=$0500, $15DE8 +$5E=$0048, actor01=2 at $15DF4
           $15E58 +$5C=$0600, $15E5E +$5E=$0048, actor01=2 at $15E7E
raw $35:  $177A4 +$5C=$0600, $177AA +$5E=$0048, actor01=2 at $177DA
raw $46/$47 shared callback:
           $1446E +$5C=$0600, $14474 +$5E=$0048, actor01=2 at $144B8
           $14688 +$5C=$0700, $1468E +$5E=$0048, actor01=2 at $146C2
```

Raw `$46/$47` share callback `$14458`, so those two branch-local setup sites are deliberately represented as shared, not guessed per raw.

## Native API
Added `arcade_prone_motion_writer_t`, `arcade_prone_motion_writer_count()` and `arcade_prone_motion_writer()`.

This is an event catalog only. It does not claim every site executes on every invocation; branch reachability remains controlled by the callback predicates.

## Regression
Test420 PASS. All 12 `MOVE.W #imm,+$5C`, `MOVE.W #imm,+$5E` and associated `MOVE.B #2,+$01` anchors were rechecked against the supplied main ROM.

Source ZIP: `WWF-Saturn-Test255-WIP157-source.zip`
SHA256: `1481972f435e444fe0b889e0e08534823bf597a2b9d6b1e2a514f6a07e7d4c89`

## Safety
- No fighter motion shadow is updated live.
- No branch predicate is guessed.
- No physical held release enabled.
- No 32-tick fallback removed.
- No tick12 migration.
- No BIN/CUE; no ROM bytes persisted.

## Next
Introduce validity-tagged fighter shadows for `+$01/+$5C/+$5E` and mirror only exact one-to-one callback events. This will permit WIP156 vertical integration to run only when provenance is known.
