# TAG campaign validation, 2026-11-13

## Scope and evidence

`tests/test_tag_campaign.py` compares the native post-TAG cursor from `$19BA`
with the intact canonical ROM through the next match `$AC0`, game over `$6FC`,
or credits `$1B4E`. The 60 cases cover every progression value 0–9, victory,
already-resolved continuation, abandonment, the skip-round DIP, and two-team
participation. Presentation dependencies execute original ROM instructions.
All RAM/video regions, D0–D7/A0–A6, dependency requests, and endpoints match.
The stage-four history reset and final-stage credits transfer are included.

These are terminal-state fixtures. They do not prove a full campaign played by
physical inputs or the complete physical credit/continue-input loop. A separate
credits test owns the pages after `$1B4E`. The mutation changing progression
increment from one to two failed at RAM `$1C0163`, demonstrating that the
comparison detects an incorrect next round.

Evidence: `build/tag-campaign-20261113/rom-summary.json` and `mutation.log`.

`tests/run_tag_campaign_main.py` executes the real application main, frontend,
post-match adapter, and subsequent real `$F8C`. The new harness observes all
entry/result/scene events. It requires credited TAG entries and the requested
stage, so a loss followed by attract mode cannot masquerade as progression.
Optional test policies supply physical pad buttons while reading runtime state;
they do not write actors, progression, timers, RNG, results, or credits.

The original diagnostic tape lost stage zero. A long exploratory run then
entered attract mode repeatedly; it is not campaign acceptance. The first
starter policy also lost. Later policy-search evidence is maintained separately
under `build/tag-policy-search` by the policy owner.

## Arena coverage

The ROM's existing ten-stage arena tables match the native tables:

| Route | Stage 0–9 scene IDs |
|---|---|
| Match | 0, 5, 1, 5, 0, 5, 0, 1, 5, 0 |
| Exit | 2, 6, 0, 6, 2, 6, 2, 2, 6, 2 |
| Return | 0, 5, 0, 5, 0, 5, 0, 0, 5, 0 |

Scene 3 is the menu and scene 4 is the presenter. Scene 1 is the initial indoor
world for stages two and seven. Scenes 2 and 6 are the two exteriors.

`tests/run_tag_campaign_scenes.py` drives the actual resident frontend scene
binding for the menu/presenter and every stage's match/exit/return route. It
compares canonical packed cells and pixels against the actual Saturn character
and name tables at three scrolling/page-boundary views. The 32 bindings cover
all seven scenes and 14,745,600 pixels, with no CD reads or sprite-cache changes.
This checks background transport; it does not certify final VDP1/VDP2 console
composition or console frame timing.

Evidence: `build/tag-campaign-20261113/scenes/summary.json` and `run.log`.

## Reproduced tagged-link CPU fault

A normal physical-input policy reached credited TAG frame 1816 in exterior
scene 2. CPU owner `$1C150` blocked in victim state `$1DA7E` at `$1DB30`.
Actor five had link `$80000000`. On the 68000's 24-bit bus, the high tag byte
is discarded: the original code reads ROM byte `$21 = FF`, fails the state-five
comparison, clears B9 bit 1, and returns. The native code previously accepted
only an actor pointer for this read and blocked.

The correction reuses the existing immutable ROM-zero actor view for this
read-only link. It preserves the original nonzero-LONG guard and ordinary
actor-pointer handling. It neither clears the link nor suppresses the owner.

The preserved real snapshot is `tests/fixtures/tag_cpu_link`. Its manifest
records hashes and provenance; it was captured after the blocked read, with
an idempotent B9 clear already applied. `tests/test_tag_campaign_cpu.py`
compares intact original `$1DA7E` with production `wf_cpu_actor_step`:

- The unchanged observed snapshot reproduces the original block before the fix.
- Thirty explicit variants cover modes 0/1/2, victim subtypes 8/9, physical null,
  two tagged nulls, ordinary actor links, and tagged actor links.
- All 16 KiB of game RAM and all eight owned D registers match; ROM bytes remain
  unchanged. The native import/export round trip must also match before a step.

Evidence: `build/tag-campaign-20261113/cpu-link-red.log`,
`cpu-link-green.log`, and `cpu-link/summary.json` (31 cases). The original
frozen C24 source was also retested separately in `cpu-link-baseline-red/`,
retaining its failing native binary and RAM/register outputs beside the oracle.

The actual default adaptive policy was then replayed through current production
main. It entered exterior scene 2, passed the former frame-1816 fault, and reached
a normal human loss at frame 3218. The real post adapter completed `$11B6` and
returned to game-over `$6FC`. There were no core faults, source changes, decode
errors, or combat backing reads. The runner correctly returned bounded status 3
because this policy lost before stage one. This verifies removal of the specific
fault, not a successful campaign. Evidence: `build/tag-campaign-20261113/cpu-replay/`;
binary SHA256 `57d9bc413bd5ce624b0e1c4c5d6be5925a17b59641eddb29d7bfccbc6172200f`.

Run focused tests from the repository root with the installed machine68k Python:

```sh
/work/venv/bin/python tests/test_tag_campaign.py --report build/tag-campaign-20261113/rom-summary.json
/work/venv/bin/python tests/test_tag_campaign_cpu.py
/work/venv/bin/python tests/run_tag_campaign_scenes.py
```

Each main/scenes runner verifies runtime build input/object hashes, preserves
its log and binary identity, and reports any source changes during a run.
Use fresh output paths for new acceptance evidence; preserve prior failures.

## Royal state-nine probability branch

Royal policy grid1 trial039 reached frame 8151 with CPU actor 7 (character 11) in
state 9, targeting a state 5 actor with selector 4A. The previous translation
explicitly blocked at $24CC for characters 04/07/0B on this route. Original $1EF42
selects from the 40/60 row at $1EF90. Choice zero selects action 1 and sets B5 bit 4;
choice one keeps the earlier table action and clears bit 4. Both enter $1DFE2.
The native prefix also now preserves the original D0=id assignment before the
RNG call. No probability or RNG state is substituted.

`tests/test_royal_cpu_state9.py` compares 163 cases from the preserved real
snapshot and explicit ID/RNG/target/wait variants. Intact ROM executes from $1EEA0
through $1DFE2 (or its early return); only the next native state 5 owner is bounded
at the same handoff. All 16 KiB RAM and D0–D7 match. Both probability branches,
all 12 character IDs, target 7B/61/ground cases, byte-counter wrap and partner 89
are covered. This is a bounded owner proof; the physical-input replay verifies
its subsequent real state5 integration separately.

Evidence: `build/royal-cpu-state9-20261113/red-verified.log`, `red/`, `green/`,
and `green.log`. An earlier setup build was rejected for concurrent referee
source changes and is retained as `red.log`; it is not the semantic red result.
The prior tagged-null suite also passes 31/31 with this CPU source, under
`link-regression/`. These changes are ROM-fidelity fixes, separate from the
frozen C26 performance image and its cadence evidence.

The identical physical policy (period 6, spacing 16, variant 4, pin-health 255)
subsequently passed the original frame 8151 failure and completed a normal loss
at frame 8948 with no core fault or decode/backing error. The policy owner retains
that fresh executable/input identity under
`build/royal-pin-release/cpu24cc-replay/trial000`.

The ordinary native frontend Royal/TAG controls still end at 14409/10397.
Against frozen C26, 224 files match byte for byte, including command/pixel traces
and every state snapshot. The two run logs and missing worker receipts are
explicitly excluded: this check used serial host assets, while C26 used the
deferred host worker. It is not a worker-equivalence claim. Full comparison:
`build/royal-cpu-state9-20261113/ordinary-native/c26-comparison.json`.
