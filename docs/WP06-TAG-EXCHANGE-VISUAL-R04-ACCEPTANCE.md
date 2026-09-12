# WP06 TAG Exchange Visual R04 — master acceptance

External checkpoint:
`WP06-TAG-EXCHANGE-VISUAL-R04-AUDITED(1).zip`

SHA256:
`bcb5c35c1d404b88b4bafc3124dd6797b62487d6edffe4ec0073c13932b7807b`

Embedded branch:
`wp06-tag-exchange-visual-r04-audited`

Embedded HEAD:
`6cb8de6721b9fff01ecb013ebc37287934bd7286`

The branch is not currently published on the remote. Embedded Git fsck passes. ZIP extraction drops executable bits on two tracked files, but their bytes are identical to HEAD.

## Independent re-audit

- core source/evidence hashes: 11/11 PASS;
- preview hashes: 4/4 PASS;
- canonical `program.bin` is byte-identical to reconstructed main 68000 from `wwfwfest.zip`;
- main SHA1: `a726c46f350723ea73158bc78055d17d418b14e2`;
- HOST strict test: PASS;
- SH-2 GCC 13.2.0 independent compile: PASS;
- ROM anchor test: PASS;
- WP25 metasprite re-audit: PASS;
- previews regenerated directly from ROM/WP25: byte-exact PASS.

## Visual contract

Normal TAG exchange visual chain:
- raw `$4C`: poses `$0060 $0061 $0066 $0067`, 13 ticks each;
- raw `$4D`: `$0062 $0063 $0000(HOLD) $0066 $0067`, with a real callback-controlled HOLD;
- raw `$4E`: `$0066 $0067`, 17 ticks each;
- raw `$66`: pair-dependent `$004B $004C`, 17 ticks each.

Exact exchange anchors:
- `$4C/$66` side0: X=$0218 Y=$0193 Z=$0140;
- `$4C/$66` side1: X=$02F6 Y=$0121 Z=$0140;
- `$4D` side0: X=$01C8 Y=$0192 Z=$0140;
- `$4D` side1: X=$034C Y=$0120 Z=$0140.

Exact legal-role handoff owner:
`$188BC`.

This is not equivalent to a late high-level `swap_fighters()` after synthetic traversal.

## Warrior R04 correction

R02 preview clipping and R03 global reverse-painter hypothesis are both superseded.

Ultimate Warrior combat graphics source is `$01`.

Exact emitted-cell counts:
- `$0060`: 28;
- `$0061`: 27;
- `$0062`: 23;
- `$0063`: 23;
- `$0066`: 23;
- `$0067`: 22.

All referenced kind-3 subcompositions and chain cells are present.

The apparent gap in isolated Warrior `$0062/$0063` is reproduced from ROM itself. These are interaction poses and must be judged together with the simultaneous `$4C` actor and side-dependent facing. **No synthetic arm tile or redrawn pixels are authorized.**

## Integration consequence

The historical WIP160 path is explicitly non-ROM-exact:
- `ST_PUNCH` temporary contact pose;
- synthetic 18-tick slap;
- generic 48-tick rope crossing;
- late `swap_fighters()`;
- hard-forced reserve facing;
- provisional ±16 px apron correction.

These must not be promoted.

Gameplay owns legal/reserve eligibility, callback-visible state, exact `$188BC` condition and player-control ownership. TAG visual owns only the exact pose/world-coordinate/facing presentation contract.

## Fail-closed

- exact finite duration of the `$4D` HOLD is not guessed;
- no fabricated single "palm touches palm" frame token;
- no universal invented APRON enum;
- no invented 2P-vs-4P visual branch.

Verdict: **TAG EXCHANGE VISUAL CONTRACT ROM-EXACT CLOSED.**
Gameplay integration: **PENDING OWNER CONSUMPTION**.
Hardware: **NOT TESTED**.
