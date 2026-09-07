# WIP92 — invalidate exact raw ownership across native reversals

## Runtime
- Adds `grapple_raw_ownership_valid`.
- The flag becomes true only when the recovered `$16582` terminal actually emits attacker `$0C` / target `$FF` ownership.
- Native HOLD reversals explicitly clear this validity before role swap continues.
- Exact category `$09` dispatch now requires both the `$0C/FF` topology and valid provenance.

## Why
- The current reversal window/role swap remains Saturn-native/provisional.
- Keeping the old `$0C/FF` shadow valid after that native role swap would falsely treat a provisional reversal as ROM-EXACT ownership.
- Raw bytes may remain as telemetry, but they cannot authorize exact category-$09 behavior after a native reversal.

## Tests
- Test378 PASS.
- Test379 PASS.
- Test380 PASS.
- `python3 -m py_compile tools/check_host.py` PASS.

## Source
- `/WWF-Saturn/WWF-Saturn-Test255-WIP92-source.zip`
- SHA256 `600b845a48aac6154ac14be1b169b9d78fe6bb28b61194e4cbdb0a6aec8453d6`

## Next
- Close raw `$15/$17` descriptor/callback profiles without inventing semantic move names; only then decide whether their native semantic bridges can be replaced.
