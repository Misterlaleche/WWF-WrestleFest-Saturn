# Test255 WIP27 — prone producer closure

Status: source checkpoint only; no BIN/CUE generated.

## ROM-EXACT closed in this checkpoint

- Test292 proves the linked-target raw `+$65` subtype producer used by prone callbacks raw `$0A/$0B/$10`.
- The literal callbacks contain `SUBI.B #8,+$65(A2)` followed by `ADDI.B #6,+$65(A2)`, yielding `8->6` and `9->7` on the demonstrated branch.
- Test293 proves ROM `$24EC2` computes target raw `+$70` from live stamina `+$66` and maximum `+$72`.
- The result is exactly the already recovered `arcade_stamina_tier()` value: `0=healthy`, `1=weakened`, `2=critical`.
- Therefore prone category `$06` vs `$07` can use the native Saturn stamina model without a provisional surrogate.
- Existing Test290/Test291 ground raw/composition coverage remains clean.

## Still open before ground is declared Arcade Perfect

- Initial raw `+$65` subtype producer on every native knockdown path must be mapped into the Saturn state machine rather than guessed.
- Callback-held `$FF00` phases in reachable prone actions still need their exact release producer(s), especially raws `$0B,$0E,$10,$13,$14,$23,$35,$46,$47`.
- No fixed timer may replace an `$FF00` hold.

## Validation

PASS: Test75 prone classifier.
PASS: Test290 live prone raw-action routing.
PASS: Test291 complete Gxy visual coverage for all reachable non-pin `$05/$06/$07` actions.
PASS: Test292 exact `+$65` retype producer.
PASS: Test293 exact `+$70` stamina producer.

Library source ZIP: `/WWF-Saturn/WWF-Saturn-Test255-WIP27-source.zip`
SHA256: `f1dc83f29ab506ed84728e33ffc623a8927722c8d4377a1b244721a8ea46a9f7`
