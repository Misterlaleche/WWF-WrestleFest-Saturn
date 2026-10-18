# Native RUNTIME17 TAG $6E34

Authority: direct canonical `main68k.bin`, SHA1 `a726c46f350723ea73158bc78055d17d418b14e2`. The native source covers $6E34–$7364, payments through existing exact $055A consumer, full $10C-byte pair migration, reciprocal $26/$9C pointer repair, referee pointer replacement, immediate partner refill and delayed paid menu. Menu $51D2/$527E and palette $C1EA are native raw-write producers. Shared native helpers own $1079E/$782C/$7720/$75EE. The only external boundaries are input-word sampling and transactional raw video delivery.

Validation: strict C99 `-Wall -Wextra -Werror` build and `test_tag_6e34` pass. Initial test failed at credit consumption before implementation. `tools/verify_tag_rom.py` executes the unmodified canonical ROM with Unicorn MC68000, including every reached child, without game hooks. Twenty-eight independent fixtures compare all RAM `$1C0000..$1C1FFF`, all nine actor records, D0–D7, FG0 bytes, palette bytes, and every ordered raw write (address/value/width) within each video channel. All pass:

- Immediate refill; insufficient credits; partner refill.
- Pair relocation with reciprocal pointer repair.
- Delayed paid choice start, direction input, confirmation migration, confirmation refill.
- Lockout rejection; reserve character-4 exclusion.
- Delayed timeout with busy reserve; progression-4 immediate paid partner route.
- Migration through all four input slots with reversed source order.
- Four deterministic random seeds vary all eight incoming D registers and populate nonzero raw actor fields across all nine records; each seed runs refill, full migration and delayed direction selection.

Reproduce from repository root, with the test-only Unicorn package available:

```
cc -std=c99 -Wall -Wextra -Werror -Isrc/runtime/include \
 src/runtime/tests/r17/tag_oracle_driver.c \
 src/runtime/src/wf_r17_tag_6e34.c \
 src/runtime/src/wf_r17_participant_helpers.c \
 src/runtime/src/wf_r17_participant_native.c \
 src/runtime/src/wf_payment.c -o /tmp/tag-oracle
PYTHONPATH=<unicorn-package-directory> python3 src/runtime/tools/verify_tag_rom.py <canonical-main68k.bin> /tmp/tag-oracle
```

The owner uses static staging storage to avoid a large Saturn stack allocation; the frame scheduler must serialize calls. Mixed FG0/palette changes require one atomic commit callback; a missing sink or failed input/commit rolls back runtime state. The regression explicitly verifies rollback. This evidence establishes the native owner against the listed ROM fixtures, not hardware validation or whole-game completion.
