# Native render record D6/D7 contract — independent ROM differential

Canonical ROM SHA1: `a726c46f350723ea73158bc78055d17d418b14e2`.

`tools/verify_render_shadow_rom.py` executes unmodified `$D1FC`, including `$DCA0` and all reached rendering children, with Unicorn MC68000. No instructions or child calls are replaced. A read-only code observer checks that `$DCA0` accepts the actual record and `$D212` starts emission. A2 starts at `$C2000`.

The independently compiled `tests/r17/render_shadow_oracle_driver.c` loads actual records using the canonical character/pose tables and calls `wf_r17_render_record_shadow`. The oracle compares both resulting D6/D7 values. It varies characters 0/12/14, poses 0/2/10, both mirror states, in-bounds coordinates, X/Y clipping, unsigned wrap, randomized screen positions and randomized incoming D0–D7.

Result: **144 admitted fixtures pass**. All four expected D6 results occur: `0`, `4`, `5`, `$1C1610`; D7 is zero. Actual character 0 pose 2 has header `$4206` (kind 2), outside the admitted kind0/kind1 contract; **18 negative fixtures** verify rejection and unchanged D6/D7. This test does not claim native kind2/kind3 or linked-record support.

Reproduce from repository root:

```
cc -std=c99 -Wall -Wextra -Werror -Isrc/runtime/include \
 src/runtime/tests/r17/render_shadow_oracle_driver.c \
 src/runtime/src/wf_r17_render_contract.c -o /tmp/render-shadow-oracle
PYTHONPATH=<unicorn-package-directory> python3 \
 src/runtime/tools/verify_render_shadow_rom.py <canonical-main68k.bin> /tmp/render-shadow-oracle
```

No production source changes were needed for this validation.
